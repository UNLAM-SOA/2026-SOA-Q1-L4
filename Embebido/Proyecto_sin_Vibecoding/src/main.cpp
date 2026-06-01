#include "header.h"
#include <Melodias.h>
#include <Sensores.h>
#include <FSM.h>

// ── Definiciones de variables globales compartidas ────────────────────────
Estado           estadoActual        = APAGADO;
Evento           newEvent;
short            lastIndexTypeSensor = 0;
Adafruit_MPU6050 mpu;
float            lastX, lastY, lastZ;
int              hallBaseline        = 0;
SemaphoreHandle_t xBotonSemaphore               = nullptr;
SemaphoreHandle_t xBotonEventoPendienteMutex     = nullptr;
SemaphoreHandle_t xTimeoutAdvertenciaPendienteMutex = nullptr;
volatile bool    botonEventoPendiente            = false;
volatile bool    timeoutAdvertenciaPendiente      = false;
TimerHandle_t    xTimerAdvertencia               = nullptr;
TaskHandle_t     xTareaAdvertenciaHandle;
TaskHandle_t     xAlarmaTask                     = nullptr;
int              contadorAdvertencias            = 0;
bool             timerReiniciadoEnAdvertencia    = false;

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