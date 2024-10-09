/**
 * COMPATIBLE WITH ESP8266 & ESP32
 * ....................................
 * Tested Device List:
 * - Adafruit Feather Huzzah ESP8266
 * - Xiao ESP32C3
 * - Xiao ESP32C6
 * - Sparkfun Thing Plus ESP32-C6
 * ....................................
 */
#include <Arduino.h>
#include <TelemetryNode.h>
/* Configure the telemetry node */
#include "TELEM_CONFIG.h"

/* Connections */
WiFiClient wiFiClient;
MqttClient mqttClient(wiFiClient);

/* Telemetry Node */
TelemetryNode telemNode = TelemetryNode(
  wiFiClient,
  mqttClient,
  TELEM_CONFIG
);

void setup() {
    /* begin the telemetry node */
    telemNode.begin();

    /* connect TelemetryNode to network & MQTT */
    telemNode.connect();

    // do MQTT pub/sub
    mqttClient.onMessage(onMqttOnMessage);
    /* subscribe to telemetry node actions to respond to action requests */
    mqttClient.subscribe(TELEM_CONFIG.topic.incoming_actions, 1);
}

void onMqttOnMessage(int messageSize) {
  /* process incoming messages so telemetry node can respond to action requests */
  JsonDocument json = telemNode.processIncomingMessage(messageSize);
}

void loop() {
    telemNode.run();
}