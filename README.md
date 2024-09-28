# ESP-Telemetry-Node

Turn your ESP8266 or ESP32 into an MQTT Telemetry Reporter with keep-alive and heartbeat.

## Features

### Connection Management, Keep Alive & Recovery

ESP-Telemetry-Node or "Telemetry Node" manages your `Serail`, `WiFi` and `MqttClient` connections to make getting online easy.

- MQTT connect & reconnect logic
  - Gracefully recovers

## Dependencies

| Library            | By                   |
| ------------------ | -------------------- |
| ArduinoMqttClient  | By Arduino           |
| ArduinoJson        | Benoit Blanchon      |
| RunnableLed        | By Scott Linenberger |
| ArduinoDebugLogger | By Scott Linenberger |

## Quick Start

```
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
}

void onMqttOnMessage(int messageSize) {
  ledOnboard.flashTimes(3, 50);
}

void loop() {
    telemNode.run();
}
```
