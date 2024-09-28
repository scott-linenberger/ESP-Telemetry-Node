/**
 * COMPATIBLE WITH ESP8266 & ESP32
 * ....................................
 * Tested with:
 * - Adafruit Feather Huzzah ESP8266
 * ....................................
 */
#include <Arduino.h>
#include <TelemetryNode.h>
#include <RunnableLed.h>
/* Configure the telemetry node */
#include "TELEM_CONFIG.h"

/* import WiFi based on device type */
#ifdef ESP8266
#include <ESP8266WiFi.h>  // Include ESP8266-specific header
#elif defined(ESP32)
#include <WiFi.h>         // Include ESP32-specific header
#else
#error "Unsupported platform"
#endif

/* Runnable LED (we'll use the onboard LED for this example)*/
RunnableLed ledOnboard = RunnableLed(
  LED_BUILTIN, // pin
  LOW // "on" pin state
);

/* Connections */
WiFiClient wiFiClient;
MqttClient mqttClient(wiFiClient);

/* Telemetry Node */
TelemetryNode telemNode = TelemetryNode(
  wiFiClient,
  mqttClient,
  ledOnboard,
  TELEM_CONFIG
);

void setup() {
    /* set pinMode or LED won't work */
    pinMode(LED_BUILTIN, OUTPUT);

    /* begin the telemetry node */
    telemNode.begin();

    /* connect TelemetryNode to network & MQTT */
    telemNode.connect();

    // do MQTT pub/sub
    mqttClient.onMessage(onMqttOnMessage);
    mqttClient.subscribe(TELEM_CONFIG.topic.incoming_actions, 1); //subscribe to actions
    mqttClient.subscribe("/mqtt-channel/i-want-to-subscribe-to/telmNode");
}

void onMqttOnMessage(int messageSize) {
  ledOnboard.flashTimes(3, 50);
}

void loop() {
    telemNode.run();
}