#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

#define LED1_PIN 5

#define MQTT_BROKER_URI "mqtt://broker.hivemq.com"
#define MQTT_BROKER_HOST "broker.hivemq.com"
#define MQTT_BROKER_PORT 1883

const char *mqttTopics[] = {
  "EMB1/teacher/bme280/temp",
  "EMB1/teacher/bme280/hum",
  "EMB1/teacher/bme280/pres",
  "test/topic/"
};

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

void onMqttMessage(char *topic, byte *payload, unsigned int length) {
  Serial.print(topic);
  Serial.print(": ");
  Serial.write(payload, length);
  Serial.println();
}

void connectToMqtt() {
  while (!mqttClient.connected()) {
    const String clientId = "ESP32S2-" + String((uint32_t)ESP.getEfuseMac(), HEX);
    Serial.print("Connecting to ");
    Serial.print(MQTT_BROKER_URI);
    Serial.print("... ");

    if (mqttClient.connect(clientId.c_str())) {
      Serial.println("connected");
      for (const char *topic : mqttTopics) {
        if (mqttClient.subscribe(topic)) {
          Serial.print("Subscribed to ");
          Serial.println(topic);
        } else {
          Serial.print("Failed to subscribe to ");
          Serial.println(topic);
        }
      }
    } else {
      Serial.print("failed, state=");
      Serial.print(mqttClient.state());
      Serial.println("; retrying in 2 seconds");
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(LED1_PIN, OUTPUT);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("Wi-Fi connected, IP: ");
  Serial.println(WiFi.localIP());

  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient.setCallback(onMqttMessage);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    WiFi.reconnect();
    delay(500);
    return;
  }
  if (!mqttClient.connected()) {
    connectToMqtt();
  }
  mqttClient.loop();
}
