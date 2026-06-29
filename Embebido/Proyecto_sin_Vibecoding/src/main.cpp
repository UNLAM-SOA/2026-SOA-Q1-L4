#include "header.h"
#include <Preferences.h>
#include <Melodias.h>
#include <Sensores.h>
#include <FSM.h>
#include <Conectividad.h>

// ── Definiciones de variables globales compartidas ────────────────────────
Estado           estadoActual        = APAGADO;
Evento           newEvent;
short            lastIndexTypeSensor = 0;
Adafruit_MPU6050 mpu;
float            lastX, lastY, lastZ;
int              hallBaseline        = 2500;
QueueHandle_t             xMqttOutQueue                     = nullptr;
SemaphoreHandle_t         xMqttComandoPendienteMutex        = nullptr;
SemaphoreHandle_t         xBotonSemaphore                   = nullptr;
SemaphoreHandle_t         xBotonEventoPendienteMutex        = nullptr;
SemaphoreHandle_t         xTimeoutAdvertenciaPendienteMutex = nullptr;
volatile Evento           mqttComandoPendienteTipo          = APAGAR;
volatile bool             mqttComandoPendiente              = false;
volatile uint8_t          melodiaSeleccionada               = 0;
SemaphoreHandle_t         xMelodiaSeleccionadaMutex         = nullptr;
volatile bool             botonEventoPendiente              = false;
volatile bool             timeoutAdvertenciaPendiente       = false;
TimerHandle_t             xTimerAdvertencia                 = nullptr;
TaskHandle_t              xTareaAdvertenciaHandle;
TaskHandle_t              xAlarmaTask                       = nullptr;
int                       contadorAdvertencias              = 0;
bool                      timerReiniciadoEnAdvertencia      = false;
float                     currentX                     = 0;
float                     currentY                     = 0;
float                     currentZ                     = 0;
volatile float            umbralMovimiento             = 2.5f;
SemaphoreHandle_t         xUmbralMovimientoMutex       = nullptr;

void setup() {
  Serial.begin(SERIAL_BAUD);
  analogReadResolution(BITS_READ_RESOLUTION);
  analogSetAttenuation(ADC_11db);
  xMelodiaSeleccionadaMutex = xSemaphoreCreateMutex();
  xUmbralMovimientoMutex = xSemaphoreCreateMutex();
  {
    Preferences prefs;
    prefs.begin("alarm", true);
    umbralMovimiento = prefs.getFloat("umbral", 2.5f);
    prefs.end();
  }
  initHardware();
  initEventDetectors();
  initConectividad();
}

void loop() {
  procesarEvento();
  vTaskDelay(pdMS_TO_TICKS(200));
}
