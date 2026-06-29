/**
 * main.cpp  -  Dispositivo IoT Antirrobos (ESP32-C3)
 * -----------------------------------------------------------------------------
 * Sistema embebido que protege un equipaje personal. Vigila golpes (MPU6050) y
 * la apertura del cierre (sensor de contacto/magnetico), y reacciona con un LED
 * y un buzzer segun una maquina de estados (ver AlarmFSM.h y el diagrama).
 *
 * Se comunica por WiFi/MQTT con un broker local (Mosquitto) que a su vez alimenta
 * Node-RED (tablero + historial) y Telegraf/InfluxDB (series de tiempo). Desde la
 * app Android se envian comandos (alarma/comando) y se reciben estado/eventos.
 *
 * Arquitectura de software:
 *   - Patron Maquina de Estados (AlarmFSM, C++ puro y testeado aparte).
 *   - FreeRTOS con 3 tareas + 1 timer de software (sin funciones bloqueantes):
 *       * tareaSensores : lee MPU6050 y contacto, genera eventos, publica acel.
 *       * tareaControl  : consume eventos, corre la FSM y maneja los actuadores.
 *       * tareaRed      : mantiene WiFi/MQTT y traduce comandos a eventos.
 *   - Cola de eventos (xQueue) para comunicar las tareas de forma segura.
 *   - Interrupcion + antirrebote por software para el pulsador.
 *   - Timer de FreeRTOS para la ventana de la ADVERTENCIA (no se usa delay()).
 * -----------------------------------------------------------------------------
 */
#include <Arduino.h>
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#include "config.h"
#include "AlarmFSM.h"

// ---------------------------------------------------------------------------
// Tipos de evento internos (lo que viaja por la cola hacia la tarea de control)
// ---------------------------------------------------------------------------
// Se distinguen de los Evento de la FSM porque algunos dependen del contexto
// (p. ej. el boton: en APAGADO significa PRENDER y en cualquier otro estado,
// APAGAR). La traduccion final a Evento de la FSM la hace la tarea de control.
enum class EventoInterno : uint8_t {
    BOTON,        // Pulsador fisico (toggle armar/desarmar)
    MOVIMIENTO,   // Golpe detectado por el acelerometro
    MAGNETICO,    // Apertura detectada por el sensor de contacto
    TIMEOUT_MOV,  // Vencio la ventana de advertencia
    CMD_ARMAR,    // Comando remoto: armar
    CMD_DESARMAR, // Comando remoto: desarmar
    CMD_PANICO    // Comando remoto: disparar alarma inmediata
};

// ---------------------------------------------------------------------------
// Objetos globales
// ---------------------------------------------------------------------------
static AlarmFSM           fsm;
static Adafruit_MPU6050   mpu;
static WiFiClient         wifiClient;
static PubSubClient       mqtt(wifiClient);

static QueueHandle_t      colaEventos = nullptr;   // EventoInterno
static TimerHandle_t      timerAdvertencia = nullptr;

// Modulo de aceleracion de reposo, capturado al armar (estado ACTIVO).
static volatile float     acelReferencia = 0.0f;
static bool               mpuDisponible  = false;

// Antirrebote del pulsador (se actualiza en la ISR y se consulta en la ISR).
static volatile uint32_t  ultimoFlancoBotonMs = 0;

// ---------------------------------------------------------------------------
// Utilidades de actuadores
// ---------------------------------------------------------------------------
static void buzzerEncender() {
    ledcWriteTone(BUZZER_CANAL_LEDC, BUZZER_FREC_ALARMA);
}
static void buzzerApagar() {
    ledcWriteTone(BUZZER_CANAL_LEDC, 0);
    ledcWrite(BUZZER_CANAL_LEDC, 0);
}
static void ledEncender() { digitalWrite(PIN_LED, HIGH); }
static void ledApagar()   { digitalWrite(PIN_LED, LOW); }

// ---------------------------------------------------------------------------
// Publicaciones MQTT
// ---------------------------------------------------------------------------
static void publicarEstado(Estado estado) {
    // JSON compatible con telegraf.conf (json_string_fields = estado, modo).
    // ArduinoJson v7: se usa JsonDocument (StaticJsonDocument quedo obsoleto).
    JsonDocument doc;
    doc["estado"] = nombreEstado(estado);
    // "modo" resume si el sistema esta armado y en que nivel de vigilancia.
    const char* modo = "DESARMADO";
    if (estado == Estado::ACTIVO)      modo = "ARMADO";
    else if (estado == Estado::ADVERTENCIA) modo = "VIGILANDO";
    else if (estado == Estado::ALERTADO)    modo = "ALARMA";
    doc["modo"] = modo;
    doc["codigo"] = static_cast<int>(estado);
    char buffer[128];
    size_t n = serializeJson(doc, buffer);
    mqtt.publish(TOPIC_ESTADO, reinterpret_cast<const uint8_t*>(buffer), n, true /*retain*/);
}

static void publicarEvento(const char* evento, const char* detalle) {
    JsonDocument doc;
    doc["evento"]  = evento;
    doc["detalle"] = detalle;
    char buffer[128];
    size_t n = serializeJson(doc, buffer);
    mqtt.publish(TOPIC_EVENTO, reinterpret_cast<const uint8_t*>(buffer), n);
}

static void publicarAcel(float magnitud, float x, float y, float z) {
    JsonDocument doc;
    doc["magnitud"]   = magnitud;
    doc["x"] = x; doc["y"] = y; doc["z"] = z;
    doc["referencia"] = acelReferencia;
    char buffer[160];
    size_t n = serializeJson(doc, buffer);
    mqtt.publish(TOPIC_ACEL, reinterpret_cast<const uint8_t*>(buffer), n);
}

// ---------------------------------------------------------------------------
// ISR del pulsador: solo aplica antirrebote y encola el evento BOTON.
// ---------------------------------------------------------------------------
static void IRAM_ATTR isrBoton() {
    uint32_t ahora = millis();
    if (ahora - ultimoFlancoBotonMs < ANTIRREBOTE_BOTON_MS) return; // rebote
    ultimoFlancoBotonMs = ahora;
    EventoInterno ev = EventoInterno::BOTON;
    BaseType_t hpw = pdFALSE;
    xQueueSendFromISR(colaEventos, &ev, &hpw);
    if (hpw == pdTRUE) portYIELD_FROM_ISR();
}

// ---------------------------------------------------------------------------
// Callback del timer de ADVERTENCIA: encola el TIMEOUT.
// ---------------------------------------------------------------------------
static void onTimerAdvertencia(TimerHandle_t) {
    EventoInterno ev = EventoInterno::TIMEOUT_MOV;
    xQueueSend(colaEventos, &ev, 0);
}

// ---------------------------------------------------------------------------
// Aplica las acciones fisicas asociadas a ENTRAR a un estado.
// ---------------------------------------------------------------------------
static void aplicarAccionesDeEstado(Estado estado) {
    switch (estado) {
        case Estado::APAGADO:
            ledApagar(); buzzerApagar();
            xTimerStop(timerAdvertencia, 0);
            break;
        case Estado::ACTIVO:
            ledApagar(); buzzerApagar();
            xTimerStop(timerAdvertencia, 0);
            // Se recalibra la referencia de reposo en la proxima lectura.
            acelReferencia = 0.0f;
            break;
        case Estado::ADVERTENCIA:
            ledEncender(); buzzerApagar();
            // Arranca/reinicia la ventana de alta vigilancia (timer de FreeRTOS).
            xTimerChangePeriod(timerAdvertencia, pdMS_TO_TICKS(VENTANA_ADVERTENCIA_MS), 0);
            xTimerStart(timerAdvertencia, 0);
            break;
        case Estado::ALERTADO:
            ledEncender(); buzzerEncender();
            xTimerStop(timerAdvertencia, 0);
            break;
    }
}

// ---------------------------------------------------------------------------
// Callback de MQTT: traduce comandos entrantes a eventos internos.
// Acepta texto plano ("ARM","DISARM","PANIC") o JSON {"cmd":"ARM"}.
// ---------------------------------------------------------------------------
static void onMensajeMqtt(char* topic, byte* payload, unsigned int length) {
    if (String(topic) != TOPIC_COMANDO) return;

    char buf[64];
    unsigned int n = length < sizeof(buf) - 1 ? length : sizeof(buf) - 1;
    memcpy(buf, payload, n);
    buf[n] = '\0';

    String cmd = String(buf);
    cmd.trim();
    // Si viene como JSON, extraer el campo "cmd".
    if (cmd.startsWith("{")) {
        JsonDocument doc;
        if (deserializeJson(doc, buf) == DeserializationError::Ok) {
            cmd = String((const char*)(doc["cmd"] | ""));
        }
    }
    cmd.toUpperCase();

    EventoInterno ev;
    bool valido = true;
    if (cmd == "ARM" || cmd == "ARMAR")          ev = EventoInterno::CMD_ARMAR;
    else if (cmd == "DISARM" || cmd == "DESARMAR") ev = EventoInterno::CMD_DESARMAR;
    else if (cmd == "PANIC" || cmd == "PANICO")    ev = EventoInterno::CMD_PANICO;
    else valido = false;

    if (valido) xQueueSend(colaEventos, &ev, 0);
}

// ---------------------------------------------------------------------------
// TAREA: sensores
// ---------------------------------------------------------------------------
static void tareaSensores(void*) {
    TickType_t ultimaTelemetria = 0;
    bool movimientoYaReportado = false;
    bool contactoYaReportado   = false;

    for (;;) {
        float magnitud = 0.0f, ax = 0.0f, ay = 0.0f, az = 0.0f;

        if (mpuDisponible) {
            sensors_event_t a, g, temp;
            mpu.getEvent(&a, &g, &temp);
            ax = a.acceleration.x; ay = a.acceleration.y; az = a.acceleration.z;
            magnitud = sqrtf(ax * ax + ay * ay + az * az);

            // Al armar, la referencia es 0: se toma la primera lectura como reposo.
            if (acelReferencia == 0.0f && magnitud > 0.0f) {
                acelReferencia = magnitud;
            }

            float desvio = fabsf(magnitud - acelReferencia);
            if (desvio > UMBRAL_MOVIMIENTO_MS2) {
                if (!movimientoYaReportado) {
                    EventoInterno ev = EventoInterno::MOVIMIENTO;
                    xQueueSend(colaEventos, &ev, 0);
                    movimientoYaReportado = true;
                }
            } else {
                movimientoYaReportado = false; // rearmar el flanco
            }
        }

        // Sensor de contacto/magnetico (ADC).
        int lecturaContacto = analogRead(PIN_CONTACTO_ADC);
        if (lecturaContacto > UMBRAL_CONTACTO_ADC) {
            if (!contactoYaReportado) {
                EventoInterno ev = EventoInterno::MAGNETICO;
                xQueueSend(colaEventos, &ev, 0);
                contactoYaReportado = true;
            }
        } else {
            contactoYaReportado = false;
        }

        // Telemetria periodica de aceleracion (no bloqueante).
        TickType_t ahora = xTaskGetTickCount();
        if (mqtt.connected() &&
            (ahora - ultimaTelemetria) >= pdMS_TO_TICKS(PERIODO_TELEMETRIA_MS)) {
            publicarAcel(magnitud, ax, ay, az);
            ultimaTelemetria = ahora;
        }

        vTaskDelay(pdMS_TO_TICKS(PERIODO_SENSORES_MS));
    }
}

// ---------------------------------------------------------------------------
// TAREA: control (corre la FSM)
// ---------------------------------------------------------------------------
static void tareaControl(void*) {
    EventoInterno ev;
    for (;;) {
        if (xQueueReceive(colaEventos, &ev, portMAX_DELAY) == pdTRUE) {
            Estado anterior = fsm.estado();
            Estado nuevo = anterior;
            const char* nombreEv = "";

            switch (ev) {
                case EventoInterno::BOTON:
                    // El boton arma si esta apagado; en cualquier otro caso, apaga.
                    nombreEv = "BOTON";
                    if (anterior == Estado::APAGADO) nuevo = fsm.procesar(Evento::PRENDER);
                    else                              nuevo = fsm.procesar(Evento::APAGAR);
                    break;
                case EventoInterno::CMD_ARMAR:
                    nombreEv = "CMD_ARMAR";
                    nuevo = fsm.procesar(Evento::PRENDER);
                    break;
                case EventoInterno::CMD_DESARMAR:
                    nombreEv = "CMD_DESARMAR";
                    nuevo = fsm.procesar(Evento::APAGAR);
                    break;
                case EventoInterno::CMD_PANICO:
                    // Panico: solo tiene sentido si el sistema esta armado.
                    nombreEv = "CMD_PANICO";
                    if (anterior != Estado::APAGADO) nuevo = fsm.procesar(Evento::MAGNETICO);
                    break;
                case EventoInterno::MOVIMIENTO:
                    nombreEv = "MOVIMIENTO";
                    nuevo = fsm.procesar(Evento::MOVIMIENTO);
                    break;
                case EventoInterno::MAGNETICO:
                    nombreEv = "MAGNETICO";
                    nuevo = fsm.procesar(Evento::MAGNETICO);
                    break;
                case EventoInterno::TIMEOUT_MOV:
                    nombreEv = "TIMEOUT_MOV";
                    nuevo = fsm.procesar(Evento::TIMEOUT_MOV);
                    break;
            }

            if (nuevo != anterior) {
                aplicarAccionesDeEstado(nuevo);
                publicarEstado(nuevo);
                publicarEvento(nombreEv, nombreEstado(nuevo));
                Serial.printf("[FSM] %s --%s--> %s\n",
                              nombreEstado(anterior), nombreEv, nombreEstado(nuevo));
            }
        }
    }
}

// ---------------------------------------------------------------------------
// Conexion WiFi / MQTT (no bloqueante: reintenta dentro de la tarea de red)
// ---------------------------------------------------------------------------
static void asegurarWiFi() {
    if (WiFi.status() == WL_CONNECTED) return;
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    Serial.print("[WiFi] Conectando");
    uint32_t inicio = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - inicio < PERIODO_RECONEXION_MS) {
        vTaskDelay(pdMS_TO_TICKS(200));
        Serial.print(".");
    }
    if (WiFi.status() == WL_CONNECTED) {
        Serial.printf("\n[WiFi] Conectado. IP: %s\n", WiFi.localIP().toString().c_str());
    } else {
        Serial.println("\n[WiFi] Sin conexion, se reintentara.");
    }
}

static void asegurarMqtt() {
    if (mqtt.connected()) return;
    if (WiFi.status() != WL_CONNECTED) return;
    Serial.print("[MQTT] Conectando al broker... ");
    if (mqtt.connect(MQTT_CLIENT_ID, MQTT_USUARIO, MQTT_PASSWORD)) {
        Serial.println("OK");
        mqtt.subscribe(TOPIC_COMANDO);
        publicarEstado(fsm.estado()); // estado inicial al reconectar
    } else {
        Serial.printf("fallo, rc=%d\n", mqtt.state());
    }
}

// ---------------------------------------------------------------------------
// TAREA: red (WiFi + MQTT)
// ---------------------------------------------------------------------------
static void tareaRed(void*) {
    mqtt.setServer(MQTT_HOST, MQTT_PUERTO);
    mqtt.setCallback(onMensajeMqtt);
    TickType_t ultimoIntento = 0;

    for (;;) {
        if (!mqtt.connected()) {
            TickType_t ahora = xTaskGetTickCount();
            if ((ahora - ultimoIntento) >= pdMS_TO_TICKS(PERIODO_RECONEXION_MS)) {
                asegurarWiFi();
                asegurarMqtt();
                ultimoIntento = ahora;
            }
        } else {
            mqtt.loop(); // procesa entrantes/salientes y mantiene el keepalive
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

// ---------------------------------------------------------------------------
// setup / loop
// ---------------------------------------------------------------------------
void setup() {
    Serial.begin(115200);
    delay(200); // breve espera para el monitor serie (solo en arranque)

    // Actuadores
    pinMode(PIN_LED, OUTPUT);
    ledApagar();
    ledcSetup(BUZZER_CANAL_LEDC, BUZZER_FREC_BASE_HZ, BUZZER_RESOLUCION);
    ledcAttachPin(PIN_BUZZER, BUZZER_CANAL_LEDC);
    buzzerApagar();

    // Pulsador con interrupcion por flanco de bajada
    pinMode(PIN_BOTON, INPUT_PULLUP);

    // ADC del sensor de contacto
    analogReadResolution(12); // 0..4095

    // I2C + MPU6050
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
    mpuDisponible = mpu.begin(MPU6050_DIRECCION_I2C, &Wire);
    if (mpuDisponible) {
        mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
        mpu.setGyroRange(MPU6050_RANGE_500_DEG);
        mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
        Serial.println("[MPU6050] Inicializado.");
    } else {
        Serial.println("[MPU6050] NO detectado (se continua sin acelerometro).");
    }

    // Infraestructura de FreeRTOS
    colaEventos = xQueueCreate(LARGO_COLA_EVENTOS, sizeof(EventoInterno));
    timerAdvertencia = xTimerCreate("advertencia",
                                    pdMS_TO_TICKS(VENTANA_ADVERTENCIA_MS),
                                    pdFALSE /*one-shot*/, nullptr, onTimerAdvertencia);

    attachInterrupt(digitalPinToInterrupt(PIN_BOTON), isrBoton, FALLING);

    // Tareas (ESP32-C3 es de un solo nucleo: se usa xTaskCreate sin afinidad)
    xTaskCreate(tareaRed,      "tareaRed",      STACK_TAREA_RED,      nullptr, PRIO_TAREA_RED,      nullptr);
    xTaskCreate(tareaSensores, "tareaSensores", STACK_TAREA_SENSORES, nullptr, PRIO_TAREA_SENSORES, nullptr);
    xTaskCreate(tareaControl,  "tareaControl",  STACK_TAREA_CONTROL,  nullptr, PRIO_TAREA_CONTROL,  nullptr);

    // Estado inicial coherente con los actuadores
    aplicarAccionesDeEstado(fsm.estado());
    Serial.println("[SISTEMA] Iniciado en estado APAGADO.");
}

void loop() {
    // Toda la logica vive en las tareas de FreeRTOS. El loop de Arduino queda
    // libre; se cede el CPU para no consumir ciclos inutilmente.
    vTaskDelay(pdMS_TO_TICKS(1000));
}
