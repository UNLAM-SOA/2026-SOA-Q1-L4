#include "Conectividad.h"
#include "header.h"
#include <Preferences.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <WiFiClientSecure.h>

static const char* WIFI_SSID = "aero"; 
static const char* WIFI_PASS = "12345678";

static WiFiClientSecure   espClient;
static PubSubClient client(espClient);

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[16] = {0};
  memcpy(msg, payload, min((unsigned int)(sizeof(msg) - 1), length));

  if (strcmp(topic, TOPIC_COMMAND) == 0) {
    Evento comando;
    if      (strcmp(msg, "ARM")    == 0) comando = PRENDER;
    else if (strcmp(msg, "DISARM") == 0) comando = APAGAR;
    else return;
    Serial.println();
    Serial.print("Message: ");
    Serial.print(msg);
    if (xSemaphoreTake(xMqttComandoPendienteMutex, 0) == pdTRUE) {
      mqttComandoPendiente     = true;
      mqttComandoPendienteTipo = comando;
      xSemaphoreGive(xMqttComandoPendienteMutex);
    }
    return;
  }

  if (strcmp(topic, TOPIC_MELODY) == 0) {
    uint8_t idx;
    if      (strcmp(msg, "STARWARS") == 0) idx = 0;
    else if (strcmp(msg, "NOKIA")    == 0) idx = 1;
    else if (strcmp(msg, "MARIO")    == 0) idx = 2;
    else if (strcmp(msg, "TETRIS")   == 0) idx = 3;
    else return;
    Serial.print("[MQTT] Melodia seleccionada: ");
    Serial.println(msg);
    if (xSemaphoreTake(xMelodiaSeleccionadaMutex, 0) == pdTRUE) {
      melodiaSeleccionada = idx;
      xSemaphoreGive(xMelodiaSeleccionadaMutex);
    }
    return;
  }

  if (strcmp(topic, TOPIC_SENSITIVITY) == 0) {
    float val = atof(msg);
    if (val < 0.1f || val > 15.0f) return;
    if (xSemaphoreTake(xUmbralMovimientoMutex, 0) == pdTRUE) {
      umbralMovimiento = val;
      xSemaphoreGive(xUmbralMovimientoMutex);
    }
    Preferences prefs;
    prefs.begin("alarm", false);
    prefs.putFloat("umbral", val);
    prefs.end();
    Serial.print("[MQTT] Umbral movimiento: ");
    Serial.println(val);
  }
}

static void tareaConectividad(void* pvParameters) {
  Serial.println("[WiFi] Esperando conexion...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    vTaskDelay(pdMS_TO_TICKS(500));
  }
  Serial.println();
  Serial.print("WiFi conectado: ");
  Serial.println(WiFi.localIP());

  while (1) {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[WiFi] Reconectando...");
      WiFi.begin(WIFI_SSID, WIFI_PASS);
      while (WiFi.status() != WL_CONNECTED) {
        vTaskDelay(pdMS_TO_TICKS(500));
      }
      Serial.println("[WiFi] Reconectado");
    }

    if (!client.connected()) {
      Serial.print("[MQTT] Conectando...");
      if (client.connect(MQTT_CLIENT_ID, MQTT_USER, MQTT_PASS)) {
        Serial.println(" conectado");
        client.subscribe(TOPIC_COMMAND);
        client.subscribe(TOPIC_MELODY);
        client.subscribe(TOPIC_SENSITIVITY);
        encolarMqtt(TOPIC_STATE, "APAGADO");
      } else {
        Serial.print(" fallo rc=");
        Serial.println(client.state());
        vTaskDelay(pdMS_TO_TICKS(5000));
        continue;
      }
    }

    // incoming MQTT messages
    client.loop();

    MqttMessage outMsg;
    while (xQueueReceive(xMqttOutQueue, &outMsg, 0) == pdTRUE) {
      client.publish(outMsg.topic, outMsg.payload);
    }

    // Publicar data de acelerometrocada 2 segundos
    static uint32_t lastAccelPublish = 0;
    uint32_t now = millis();
    if (now - lastAccelPublish >= 2000) {
      lastAccelPublish = now;
      char accelPayload[64];
      snprintf(accelPayload, sizeof(accelPayload),
               "{\"x\":%.2f,\"y\":%.2f,\"z\":%.2f}",
               currentX, currentY, currentZ);
      client.publish(TOPIC_ACCEL, accelPayload);
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void encolarMqtt(const char* topic, const char* payload) {
  if (xMqttOutQueue == nullptr) return;
  MqttMessage msg;
  strncpy(msg.topic,   topic,   sizeof(msg.topic)   - 1);
  strncpy(msg.payload, payload, sizeof(msg.payload) - 1);
  msg.topic[sizeof(msg.topic)   - 1] = '\0';
  msg.payload[sizeof(msg.payload) - 1] = '\0';
  xQueueSend(xMqttOutQueue, &msg, 0); 
}

void initConectividad() {
  xMqttOutQueue = xQueueCreate(MQTT_QUEUE_SIZE, sizeof(MqttMessage));

  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  espClient.setInsecure();

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(mqttCallback);

  xTaskCreate(tareaConectividad, "tareaConectividad", 8192, nullptr, PRIORIDAD, nullptr);
}