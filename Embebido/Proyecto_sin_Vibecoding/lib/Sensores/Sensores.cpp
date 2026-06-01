#include "Sensores.h"

// ── ISR ───────────────────────────────────────────────────────────────────
static void isrBoton() {
  xSemaphoreGiveFromISR(xBotonSemaphore, nullptr);
}

// ── Tareas FreeRTOS ───────────────────────────────────────────────────────
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

void setearTimeoutAdvertencia(void* parameter) {
  while (1) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    if (xSemaphoreTake(xTimeoutAdvertenciaPendienteMutex, portMAX_DELAY) == pdTRUE) {
      timeoutAdvertenciaPendiente = true;
      xSemaphoreGive(xTimeoutAdvertenciaPendienteMutex);
    }
  }
}

// ── Callback del timer ────────────────────────────────────────────────────
void callbackTemporizador(TimerHandle_t xTimer) {
  xTaskNotifyGive(xTareaAdvertenciaHandle);
}

// ── Timeout helpers ───────────────────────────────────────────────────────
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

// ── Detección de eventos ──────────────────────────────────────────────────
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

static bool detectaTouchEvent() {
  int hallActual = analogRead(PIN_HALL);
  bool trampaHall = abs(hallActual - hallBaseline) > UMBRAL_TOUCH;
  if ((estadoActual == ACTIVO || estadoActual == ADVERTENCIA_MOVIMIENTO) && trampaHall) {
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

// ── Tabla de detectores (usada por FSM) ──────────────────────────────────
EventDetector eventType[MAX_TYPE_EVENTS] = {
  detectaBotonEvent,
  detectaMovimientoEvent,
  detectaTouchEvent,
  detectaTimeoutAdvertencia
};

// ── Init ──────────────────────────────────────────────────────────────────
void initHardware() {
  Wire.begin(ACCELEROMETER_SDA, ACCELEROMETER_SCL);
  if (!mpu.begin()) {
    Serial.println("No se encontro el sensor en los pines SDA y SCL");
  }
  Serial.println("¡MPU6050 conectado en los pines!");
  pinMode(PIN_BUTTON,  INPUT_PULLUP);
  pinMode(SPEAKER_PIN, OUTPUT);
  pinMode(LED_PIN,     OUTPUT);
  pinMode(PIN_HALL,    INPUT);

  Serial.println("Valor hall: ");
  Serial.println(analogRead(PIN_HALL));
}

void initEventDetectors() {
  xBotonSemaphore                  = xSemaphoreCreateBinary();
  xBotonEventoPendienteMutex        = xSemaphoreCreateMutex();
  xTimeoutAdvertenciaPendienteMutex = xSemaphoreCreateMutex();

  xTaskCreate(tareaBoton, "tareaBoton", TAM_PILA, nullptr, PRIORIDAD, nullptr);
  xTaskCreate(setearTimeoutAdvertencia, "setearTimeoutAdvertencia", TAM_PILA, nullptr, PRIORIDAD, &xTareaAdvertenciaHandle);

  xTimerAdvertencia = xTimerCreate("tAdvertencia", pdMS_TO_TICKS(TIMEOUT_ADVERTENCIA_MS),
                                   pdFALSE, nullptr, callbackTemporizador);

  attachInterrupt(digitalPinToInterrupt(PIN_BUTTON), isrBoton, FALLING);
}
