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

const char *wifiStatusName(wl_status_t status) {
  switch (status) {
    case WL_IDLE_STATUS: return "idle";
    case WL_NO_SSID_AVAIL: return "SSID not found";
    case WL_SCAN_COMPLETED: return "scan completed";
    case WL_CONNECTED: return "connected";
    case WL_CONNECT_FAILED: return "connection failed (check password/authentication)";
    case WL_CONNECTION_LOST: return "connection lost";
    case WL_DISCONNECTED: return "disconnected";
    default: return "unknown";
  }
}

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

bool connectToWifi() {
  Serial.println("Wi-Fi: starting connection (check SSID/password in include/secrets.h)");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  const unsigned long startTime = millis();
  unsigned long lastStatusTime = 0;
  while (WiFi.status() != WL_CONNECTED && millis() - startTime < 20000) {
    if (millis() - lastStatusTime >= 1000) {
      lastStatusTime = millis();
      Serial.print("Wi-Fi: ");
      Serial.print(wifiStatusName(WiFi.status()));
      Serial.print(" (status ");
      Serial.print(static_cast<int>(WiFi.status()));
      Serial.println(")");
    }
    delay(50);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.print("Wi-Fi: failed after 20 seconds; final status: ");
    Serial.print(wifiStatusName(WiFi.status()));
    Serial.print(" (");
    Serial.print(static_cast<int>(WiFi.status()));
    Serial.println("). Check SSID, password, and 2.4 GHz network.");
    return false;
  }

  Serial.println("Wi-Fi: connected");
  Serial.print("  IP:      ");
  Serial.println(WiFi.localIP());
  Serial.print("  Gateway: ");
  Serial.println(WiFi.gatewayIP());
  Serial.print("  DNS:     ");
  Serial.println(WiFi.dnsIP());
  Serial.print("  Signal:  ");
  Serial.print(WiFi.RSSI());
  Serial.println(" dBm");

  IPAddress brokerIp;
  Serial.print("Internet check: resolving ");
  Serial.print(MQTT_BROKER_HOST);
  Serial.print("... ");
  if (!WiFi.hostByName(MQTT_BROKER_HOST, brokerIp)) {
    Serial.println("DNS failed");
    return true;
  }
  Serial.println(brokerIp);

  WiFiClient probeClient;
  Serial.print("Internet check: TCP port ");
  Serial.print(MQTT_BROKER_PORT);
  Serial.print("... ");
  if (!probeClient.connect(brokerIp, MQTT_BROKER_PORT)) {
    Serial.println("failed (Wi-Fi works, but broker/port is unreachable)");
    return true;
  }
  Serial.println("connected");
  probeClient.stop();
  return true;
}

void setup() {
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println("ESP32 booted; serial output is working.");
  pinMode(LED1_PIN, OUTPUT);

  mqttClient.setServer(MQTT_BROKER_HOST, MQTT_BROKER_PORT);
  mqttClient.setCallback(onMqttMessage);
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    if (!connectToWifi()) {
      delay(5000);
      return;
    }
  }
  if (!mqttClient.connected()) {
    connectToMqtt();
  }
  mqttClient.loop();

  static unsigned long lastHeartbeat = 0;
  if (millis() - lastHeartbeat >= 5000) {
    lastHeartbeat = millis();
    Serial.print("Heartbeat: Wi-Fi ");
    Serial.print(wifiStatusName(WiFi.status()));
    Serial.print(", MQTT ");
    Serial.println(mqttClient.connected() ? "connected" : "disconnected");
  }
}
