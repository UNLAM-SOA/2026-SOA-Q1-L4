#include "Conectividad.h"
#include "header.h"
#include <WiFi.h>
#include <PubSubClient.h>

static const char* WIFI_SSID = "SO Avanzados";
static const char* WIFI_PASS = "SOA.2019";

static WiFiClient   espClient;
static PubSubClient client(espClient);

static void mqttCallback(char* topic, byte* payload, unsigned int length) {
  char msg[16] = {0};
  memcpy(msg, payload, min((unsigned int)(sizeof(msg) - 1), length));

  if (strcmp(topic, TOPIC_COMMAND) != 0) return;

  Evento comando;
  if      (strcmp(msg, "ARM")    == 0) comando = PRENDER;
  else if (strcmp(msg, "DISARM") == 0) comando = APAGAR;
  else return;

  if (xSemaphoreTake(xMqttComandoPendienteMutex, 0) == pdTRUE) {
    mqttComandoPendiente     = true;
    mqttComandoPendienteTipo = comando;
    xSemaphoreGive(xMqttComandoPendienteMutex);
  }
}

static void tareaConectividad(void* pvParameters) {
  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(pdMS_TO_TICKS(500));
  }
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

  client.setServer(MQTT_BROKER, MQTT_PORT);
  client.setCallback(mqttCallback);

  xTaskCreate(tareaConectividad, "tareaConectividad", 4096, nullptr, PRIORIDAD, nullptr);
}