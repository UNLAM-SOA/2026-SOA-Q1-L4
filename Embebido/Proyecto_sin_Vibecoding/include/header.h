#ifndef HEADER_H
#define HEADER_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/timers.h>
#include <freertos/queue.h>

// ── Pines ─────────────────────────────────────────────────────────────────
#define SPEAKER_PIN             0
#define LED_PIN                 1
#define PIN_HALL                3
#define PIN_BUTTON              2
#define ACCELEROMETER_SCL       8
#define ACCELEROMETER_SDA       9

// ── Tamaños de la FSM ─────────────────────────────────────────────────────
#define MAX_TYPE_EVENTS         5
#define TOTAL_EVENTOS           5
#define TOTAL_ESTADOS           4

// ── FreeRTOS ──────────────────────────────────────────────────────────────
#define PRIORIDAD               1
#define TAM_PILA                2048
#define SERIAL_BAUD             115200

// ── Umbrales ──────────────────────────────────────────────────────────────
#define UMBRAL_MOVIMIENTO       2.5
#define UMBRAL_TOUCH            500
#define DEBOUNCE_MS             50
#define TIMEOUT_ADVERTENCIA_MS  6000
#define UMBRAL_ADVERTENCIAS     3

#define BITS_READ_RESOLUTION    12

// ── MQTT ──────────────────────────────────────────────────────────────────
#define MQTT_BROKER             "0.tcp.sa.ngrok.io"
#define MQTT_PORT               29395
#define MQTT_USER               "claromio"
#define MQTT_PASS               "RiverBest"
#define MQTT_CLIENT_ID          "ESP32AlarmClient"

#define TOPIC_COMMAND           "alarm/command"
#define TOPIC_STATE             "alarm/state"
#define TOPIC_ACCEL             "alarm/accel"
#define TOPIC_EVENT             "alarm/event"

#define MQTT_QUEUE_SIZE         8

struct MqttMessage {
  char topic[32];
  char payload[64];
};

// ── Tipos compartidos entre módulos ───────────────────────────────────────
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

// ── Variables globales ────────────────────────────────────────────────────
extern Estado            estadoActual;
extern Evento            newEvent;
extern short             lastIndexTypeSensor;

extern Adafruit_MPU6050  mpu;
extern float             lastX, lastY, lastZ;
extern int               hallBaseline;

extern SemaphoreHandle_t xBotonSemaphore;
extern SemaphoreHandle_t xBotonEventoPendienteMutex;
extern SemaphoreHandle_t xTimeoutAdvertenciaPendienteMutex;

extern volatile bool     botonEventoPendiente;
extern volatile bool     timeoutAdvertenciaPendiente;

extern TimerHandle_t     xTimerAdvertencia;
extern TaskHandle_t      xTareaAdvertenciaHandle;
extern TaskHandle_t      xAlarmaTask;

extern int               contadorAdvertencias;
extern bool              timerReiniciadoEnAdvertencia;

extern QueueHandle_t             xMqttOutQueue;
extern SemaphoreHandle_t         xMqttComandoPendienteMutex;
extern volatile bool             mqttComandoPendiente;
extern volatile Evento           mqttComandoPendienteTipo;

#endif
