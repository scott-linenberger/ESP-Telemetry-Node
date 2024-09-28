#include <Arduino.h>
#include <TelemetryNode.h>

const TelemetryNodeConfig TELEM_CONFIG = {
  true,// ------------------------------- isDebugging, when true, outputs Serial
  115200, // ---------------------------- Serial baud rate
  "wifiSSID", // ------------------------ Wifi SSID
  "wifiPassword", // -------------------- Wifi Password
  "0.0.0.0", // ------------------------- MQTT Broker IP Address
  1883, // ------------------------------ MQTT Broker Port
  "uname", // --------------------------- MQTT username
  "password", // ------------------------ MQTT password
  "<mqtt-device-id>", // ---------------- MQTT device ID
  "Adafruit Feather HUZZAH ESP8266", // - MQTT device type 
  false, // ----------------------------- MQTT clean session flag
  5, // --------------------------------- MQTT connection retries
  30000, // ----------------------------- MQTT reconnect timeout between connection tries
  60000, // ----------------------------- MQTT reconnect delay time before restarting after max failed attempts
  R"json({
    id: <device-id>,
    type: <device-type>,
    online: 0,
    event: "DEVICE_LAST_WILL",
    msg: "He's dead, Jim.",
  })json", // --------------------------- MQTT last will JSON string
  true, // ------------------------------ MQTT last will retain
  1, // --------------------------------- MQTT last will QOS
  "topic/telemetry", // ----------------- MQTT Topic telemetry broadcasts
  "topic/actions", // ------------------- MQTT Topic incoming actions
  300000, // ---------------------------- Keep alive timeout 5 min as ms
  900000, // ---------------------------- Heartbeat timeout 15 min as ms
};


