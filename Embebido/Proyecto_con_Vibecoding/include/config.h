/**
 * config.h
 * -----------------------------------------------------------------------------
 * Configuracion centralizada del dispositivo IoT antirrobos (ESP32-C3).
 *
 * Aqui se definen TODAS las constantes del proyecto (pines, umbrales, tiempos,
 * credenciales y topicos MQTT). El objetivo es no tener "numeros magicos"
 * dispersos por el codigo: si hay que ajustar algo, se cambia en un solo lugar.
 *
 * Pines elegidos para el ESP32-C3 evitando los pines de "strapping" (2, 8, 9)
 * y los reservados a la flash/USB, para que el firmware funcione tanto en el
 * simulador Wokwi como en una placa fisica sin conflictos de arranque.
 * -----------------------------------------------------------------------------
 */
#ifndef CONFIG_H
#define CONFIG_H

// ---------------------------------------------------------------------------
// Pines (numeracion GPIO del ESP32-C3)
// ---------------------------------------------------------------------------
static const int PIN_CONTACTO_ADC = 3;   // Sensor de contacto/magnetico (potenciometro en Wokwi) -> ADC1_CH3
static const int PIN_BOTON        = 4;   // Pulsador de armado/desarmado (INPUT_PULLUP + interrupcion)
static const int PIN_I2C_SDA      = 5;   // Bus I2C - dato (MPU6050)
static const int PIN_I2C_SCL      = 6;   // Bus I2C - reloj (MPU6050)
static const int PIN_LED          = 7;   // LED rojo (indicador visual)
static const int PIN_BUZZER       = 10;  // Buzzer (alarma sonora, via PWM/LEDC)

// ---------------------------------------------------------------------------
// Buzzer (LEDC / PWM) - API del arduino-esp32 core 2.0.x
// ---------------------------------------------------------------------------
static const int      BUZZER_CANAL_LEDC   = 0;     // Canal LEDC usado por el buzzer
static const int      BUZZER_RESOLUCION   = 10;    // Resolucion del PWM en bits
static const uint32_t BUZZER_FREC_BASE_HZ = 2000;  // Frecuencia base de configuracion del canal
static const uint32_t BUZZER_FREC_ALARMA  = 880;   // Tono de la alarma (La5 = 880 Hz)

// ---------------------------------------------------------------------------
// MPU6050 (acelerometro/giroscopo)
// ---------------------------------------------------------------------------
static const uint8_t MPU6050_DIRECCION_I2C = 0x68; // Direccion I2C por defecto

// ---------------------------------------------------------------------------
// Umbrales de deteccion
// ---------------------------------------------------------------------------
// Movimiento: diferencia (en m/s^2) entre el modulo de aceleracion actual y el
// modulo de reposo capturado al armar. Si se supera, hay "movimiento brusco".
static const float UMBRAL_MOVIMIENTO_MS2 = 2.5f;

// Contacto/magnetico: el ADC del ESP32-C3 entrega 0..4095. Por encima de este
// valor se considera que se manipulo/abrio el cierre del bolso.
static const int UMBRAL_CONTACTO_ADC = 2000;

// ---------------------------------------------------------------------------
// Tiempos (en milisegundos) - NO se usan delays bloqueantes
// ---------------------------------------------------------------------------
static const uint32_t PERIODO_SENSORES_MS   = 100;   // Lectura de sensores (10 Hz)
static const uint32_t PERIODO_TELEMETRIA_MS = 1000;  // Publicacion periodica de aceleracion
static const uint32_t VENTANA_ADVERTENCIA_MS= 6000;  // Tiempo de "alta vigilancia" en ADVERTENCIA
static const uint32_t ANTIRREBOTE_BOTON_MS  = 50;    // Filtro anti-rebote del pulsador
static const uint32_t PERIODO_RECONEXION_MS = 5000;  // Reintento de conexion WiFi/MQTT

// ---------------------------------------------------------------------------
// FreeRTOS - tamanos de pila (bytes) y prioridades
// ---------------------------------------------------------------------------
static const uint32_t STACK_TAREA_SENSORES = 4096;
static const uint32_t STACK_TAREA_CONTROL  = 4096;
static const uint32_t STACK_TAREA_RED      = 8192;  // WiFi+MQTT necesitan mas pila
static const UBaseType_t PRIO_TAREA_RED      = 1;
static const UBaseType_t PRIO_TAREA_SENSORES = 2;
static const UBaseType_t PRIO_TAREA_CONTROL  = 3;   // El control es lo mas prioritario
static const UBaseType_t LARGO_COLA_EVENTOS  = 16;  // Capacidad de la cola de eventos

// ---------------------------------------------------------------------------
// WiFi
// ---------------------------------------------------------------------------
// Wokwi provee la red abierta "Wokwi-GUEST" (password vacia). En una placa
// fisica, reemplazar por las credenciales de la red real.
static const char* const WIFI_SSID     = "Wokwi-GUEST";
static const char* const WIFI_PASSWORD = "";

// ---------------------------------------------------------------------------
// MQTT
// ---------------------------------------------------------------------------
// En Wokwi (VS Code) el broker del host se alcanza con "host.wokwi.internal".
// En una placa fisica, reemplazar por la IP de la PC que corre Mosquitto
// (por ej. "192.168.0.10").
static const char* const MQTT_HOST     = "host.wokwi.internal";
static const uint16_t    MQTT_PUERTO   = 1883;
static const char* const MQTT_USUARIO  = "claromio";
static const char* const MQTT_PASSWORD = "RiverBest";
static const char* const MQTT_CLIENT_ID= "esp32-alarma";

// Topicos (coinciden con Node-RED y Telegraf de la carpeta Infraestructura)
static const char* const TOPIC_ESTADO  = "alarma/estado";   // ESP32 -> mundo
static const char* const TOPIC_ACEL    = "alarma/acel";     // ESP32 -> mundo
static const char* const TOPIC_EVENTO  = "alarma/evento";   // ESP32 -> mundo
static const char* const TOPIC_COMANDO = "alarma/comando";  // mundo -> ESP32

#endif // CONFIG_H
