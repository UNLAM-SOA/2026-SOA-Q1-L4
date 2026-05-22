#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>

#define SPEAKER_PIN             0
#define LED_PIN                 1
#define PIN_HALL                3
#define ACCELEROMETER_SCL       8
#define ACCELEROMETER_SDA       9
#define PIN_BUTTON              2
#define MAX_TYPE_EVENTS         4
#define TOTAL_EVENTOS           5
#define TOTAL_ESTADOS           4
#define PRIORIDAD               1
#define TAM_PILA                2048
#define SERIAL_BAUD             115200
#define UMBRAL_MOVIMIENTO       2.5
#define UMBRAL_TOUCH            500
#define DEBOUNCE_MS             50
#define BITS_READ_RESOLUTION    12
#define TIMEOUT_ADVERTENCIA_MS  6000
#define UMBRAL_ADVERTENCIAS     3

enum Estado {
  APAGADO,
  ACTIVO,
  ADVERTENCIA_MOVIMIENTO,
  ALERTA
};

enum Evento {
  APAGAR,
  PRENDER,
  MOV_DETECTADO,
  TOUCH_DETECTADO,
  TIMEOUT_ADVERTENCIA
};

typedef bool (*EventDetector)();
typedef void (*Accion)();

// --- Variables globales compartidas ---
Estado           estadoActual       = APAGADO;
Evento           newEvent;
short            lastIndexTypeSensor = 0;
Adafruit_MPU6050 mpu;
float            lastX, lastY, lastZ;
int              hallBaseline = 0;
static SemaphoreHandle_t        xBotonSemaphore      = nullptr;
static SemaphoreHandle_t        xBotonEventoPendienteMutex        = nullptr;
static SemaphoreHandle_t        xTimeoutAdvertenciaPendienteMutex = nullptr;
static volatile bool     botonEventoPendiente = false;
static volatile bool     timeoutAdvertenciaPendiente = false;
// Timeout de advertencia: timeout con FreeRTOS
static TimerHandle_t     xTimerAdvertencia          = nullptr;
TaskHandle_t             xTareaAdvertenciaHandle;
int                      contadorAdvertencias = 0;
bool                     timerReiniciadoEnAdvertencia = false;

static void isrBoton() {
  xSemaphoreGiveFromISR(xBotonSemaphore, nullptr);
}

static void tareaBoton(void* pvParameters) {
  while (1) {
    xSemaphoreTake(xBotonSemaphore, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
    if (digitalRead(PIN_BUTTON) == LOW) {
      if (xSemaphoreTake(xBotonEventoPendienteMutex, portMAX_DELAY) == pdTRUE) {
        botonEventoPendiente = true;
        xSemaphoreGive(xBotonEventoPendienteMutex);
      }
    }
  }
}

static bool detectaBotonEvent() {
  bool result = false;
  if (xSemaphoreTake(xBotonEventoPendienteMutex, portMAX_DELAY) == pdTRUE) {
    if (botonEventoPendiente) {
      botonEventoPendiente = false;
      newEvent = (estadoActual == APAGADO) ? PRENDER : APAGAR;
      result = true;
    }
    xSemaphoreGive(xBotonEventoPendienteMutex);
  }
  return result;
}

// Movimiento: MPU6050 (acelerometro)
static bool detectaMovimientoBrusco() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  float diffX = abs(a.acceleration.x - lastX);
  float diffY = abs(a.acceleration.y - lastY);
  float diffZ = abs(a.acceleration.z - lastZ);
  bool condition = (diffX > UMBRAL_MOVIMIENTO || diffY > UMBRAL_MOVIMIENTO || diffZ > UMBRAL_MOVIMIENTO);

  if (condition) {
    Serial.println("Valor x");
    Serial.println(a.acceleration.x);

    Serial.println("Valor y");
    Serial.println(a.acceleration.y);

    Serial.println("Valor z");
    Serial.println(a.acceleration.z);
  }

  return condition;
}

static bool detectaMovimientoEvent() {
  if ((estadoActual == ACTIVO || estadoActual == ADVERTENCIA_MOVIMIENTO) && detectaMovimientoBrusco()) {
    newEvent = MOV_DETECTADO;
    return true;
  }
  return false;
}

// Touch: potenciómetro/sensor Hall
static bool detectaTouchEvent() {
  int hallActual = analogRead(PIN_HALL);
  bool trampaHall = abs(hallActual - hallBaseline) > UMBRAL_TOUCH;
  if (((estadoActual == ACTIVO || estadoActual == ADVERTENCIA_MOVIMIENTO)) && trampaHall) {
    Serial.println("Valor hall: ");
    Serial.println(hallActual);
    newEvent = TOUCH_DETECTADO;
    return true;
  }
  return false;
}

static bool detectaTimeoutAdvertencia() {
  bool result = false;
  if (xSemaphoreTake(xTimeoutAdvertenciaPendienteMutex, portMAX_DELAY) == pdTRUE) {
    if (timeoutAdvertenciaPendiente) {
      timeoutAdvertenciaPendiente = false;
      newEvent = TIMEOUT_ADVERTENCIA;
      result = true;
    }
    xSemaphoreGive(xTimeoutAdvertenciaPendienteMutex);
  }
  return result;
}

void iniciarTimeoutAdvertencia() {
  xTimerReset(xTimerAdvertencia, 0);
}

void cancelarTimeoutAdvertencia() {
  if (xSemaphoreTake(xTimeoutAdvertenciaPendienteMutex, portMAX_DELAY) == pdTRUE) {
    xTimerStop(xTimerAdvertencia, 0);
    timeoutAdvertenciaPendiente = false;
    xSemaphoreGive(xTimeoutAdvertenciaPendienteMutex);
  }
}

void setearTimeoutAdvertencia(void *parameter)
{
  while(1){
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (xSemaphoreTake(xTimeoutAdvertenciaPendienteMutex, portMAX_DELAY) == pdTRUE) {
      timeoutAdvertenciaPendiente = true;
      xSemaphoreGive(xTimeoutAdvertenciaPendienteMutex);
    }
  }
}

void callbackTemporizador(TimerHandle_t xTimer)
{
  xTaskNotifyGive(xTareaAdvertenciaHandle);
}

// Acciones
static void guardaPosicion() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  lastX = a.acceleration.x;
  lastY = a.acceleration.y;
  lastZ = a.acceleration.z;
}

static void prender() {
  Serial.println(">>> SISTEMA ARMADO");
  guardaPosicion();
  estadoActual = ACTIVO;
}

static void advertirMovimiento() {
  Serial.println(">>> ADVERTENCIA: Movimiento detectado");
  contadorAdvertencias++;
  timerReiniciadoEnAdvertencia = false;
  digitalWrite(LED_PIN, HIGH);
  iniciarTimeoutAdvertencia();
  estadoActual = ADVERTENCIA_MOVIMIENTO;
}

static void alertar() {
  Serial.println("¡ALERTA! Movimiento Y contacto detectados");
  cancelarTimeoutAdvertencia();
  digitalWrite(LED_PIN, HIGH);
  tone(SPEAKER_PIN, 3136);
  estadoActual = ALERTA;
}

static void manejarMovDuranteAdvertencia() {
  if (!timerReiniciadoEnAdvertencia) {
    Serial.println(">>> Movimiento reiterado: extendiendo advertencia");
    timerReiniciadoEnAdvertencia = true;
    iniciarTimeoutAdvertencia();
  } else {
    Serial.println(">>> Movimiento persistente: escalando a ALERTA");
    alertar();
  }
}

static void apagar() {
  Serial.println(">>> SISTEMA DESARMADO");
  contadorAdvertencias = 0;
  cancelarTimeoutAdvertencia();
  if (estadoActual == ALERTA) noTone(SPEAKER_PIN);
  digitalWrite(LED_PIN, LOW);
  estadoActual = APAGADO;
}

static void reiniciarAdvertencia() {
  if (contadorAdvertencias >= UMBRAL_ADVERTENCIAS) {
    Serial.println(">>> Advertencias repetidas: escalando a ALERTA");
    alertar();
    return;
  }
  Serial.println(">>> Timeout: volviendo a ACTIVO");

  int hallActual = analogRead(PIN_HALL);
  Serial.println("Valor hall: ");
  Serial.println(hallActual);

  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);
  Serial.println("Valor x");
  Serial.println(a.acceleration.x);

  Serial.println("Valor y");
  Serial.println(a.acceleration.y);

  Serial.println("Valor z");
  Serial.println(a.acceleration.z);

  contadorAdvertencias = 0;
  digitalWrite(LED_PIN, LOW);
  estadoActual = ACTIVO;
}

static void errorTransicion() {
  Serial.println("Transicion invalida");
}

// Matriz de transición (FSM)
// Eventos: APAGAR, PRENDER, MOV_DETECTADO, TOUCH_DETECTADO, TIMEOUT_ADVERTENCIA
static const Accion MATRIZ_TRANSICION[TOTAL_ESTADOS][TOTAL_EVENTOS] = {
  { errorTransicion, prender,          errorTransicion,    errorTransicion,     errorTransicion      }, // APAGADO
  { apagar,          errorTransicion,  advertirMovimiento, alertar,             errorTransicion      }, // ACTIVO
  { apagar,          errorTransicion,  manejarMovDuranteAdvertencia, alertar, reiniciarAdvertencia }, // ADVERTENCIA_MOVIMIENTO
  { apagar,          errorTransicion,  errorTransicion,    errorTransicion,     errorTransicion      }  // ALERTA
};

// Tabla de detectores
EventDetector eventType[MAX_TYPE_EVENTS] = {
  detectaBotonEvent,
  detectaMovimientoEvent,
  detectaTouchEvent,
  detectaTimeoutAdvertencia
};

// Inicializar tareas de freertos
void initEventDetectors() {
  xBotonSemaphore                   = xSemaphoreCreateBinary();
  xBotonEventoPendienteMutex         = xSemaphoreCreateMutex();
  xTimeoutAdvertenciaPendienteMutex  = xSemaphoreCreateMutex();

  xTaskCreate(tareaBoton, "tareaBoton", TAM_PILA, nullptr, PRIORIDAD, nullptr);
  xTaskCreate(setearTimeoutAdvertencia, "setearTimeoutAdvertencia", TAM_PILA, nullptr, PRIORIDAD, &xTareaAdvertenciaHandle);

  xTimerAdvertencia = xTimerCreate("tAdvertencia", pdMS_TO_TICKS(TIMEOUT_ADVERTENCIA_MS),
                                   pdFALSE, nullptr,
                                   callbackTemporizador);
  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), isrBoton, FALLING);
}

// Inicializar sensores y actuadores
void initHardware() {
  Wire.begin(ACCELEROMETER_SDA, ACCELEROMETER_SCL);
  if (!mpu.begin()) {
    Serial.println("No se encontro el sensor en los pines SDA y SCL");
  }
  Serial.println("¡MPU6050 conectado en los pines!");
  pinMode(PIN_BUTTON,   INPUT_PULLUP);
  pinMode(SPEAKER_PIN,  OUTPUT);
  pinMode(LED_PIN,      OUTPUT);
  pinMode(PIN_HALL, INPUT);

  Serial.println("Valor hall: ");
  Serial.println(analogRead(PIN_HALL));
}

bool getNuevoEvento() {
  short index = lastIndexTypeSensor % MAX_TYPE_EVENTS;
  lastIndexTypeSensor++;
  return eventType[index]();
}

void procesarEvento() {
  if (getNuevoEvento()) {
    if ((newEvent >= 0) && (newEvent < TOTAL_EVENTOS) &&
        (estadoActual >= 0) && (estadoActual < TOTAL_ESTADOS)) {
      MATRIZ_TRANSICION[estadoActual][newEvent]();
    }
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  analogReadResolution(BITS_READ_RESOLUTION);
  analogSetAttenuation(ADC_11db);
  initHardware();
  initEventDetectors();
}

void loop() {
  procesarEvento();
  vTaskDelay(pdMS_TO_TICKS(200));
}