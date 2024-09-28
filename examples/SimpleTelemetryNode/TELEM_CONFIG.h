#include <Arduino.h>
#include <TelemetryNode.h>


const TelemetryNodeConfig TELEM_CONFIG = {
  /* CONNECTION */ 
  {
    "wifiSSID", // ------------------------ Wifi SSID
    "wifiPassword", // -------------------- Wifi Password
      "0.0.0.0", // ------------------------- MQTT Broker IP Address
      1883, // ------------------------------ MQTT Broker Port
      "uname", // --------------------------- MQTT username
      "password", // ------------------------ MQTT password
      "<mqtt-client-id>", // ---------------- MQTT client ID
      false, // ----------------------------- MQTT clean session flag
      5, // --------------------------------- MQTT connection retries before reboot
      R"json({
        id: <mqtt-device-id>,
        type: <device-type>,
        online: 0,
        event: "DEVICE_LAST_WILL",
        msg: "He's dead, Jim.",
      })json", // --------------------------- MQTT last will JSON string
      true, // ------------------------------ MQTT last will retain
      1, // --------------------------------- MQTT last will QOS
  },
  /* DEVICE */
  {
    115200, // ---------------------------- Serial baud rate
    true, // ------------------------------ logs Serial output when true, does NOT when false
    true, // ------------------------------ enable heartbeats
    false, // ----------------------------- retain heartbeat messages
    0, // --------------------------------- qos hertbeat messages
    true, // ------------------------------ send device reset reason
    true, // ------------------------------ retain device reset reason
    0, // --------------------------------- qos device device reset reason messages
    true, // ------------------------------ send device time alive when heartbeat
    true, // ------------------------------ retain device time alive
    0, // --------------------------------- qos device time alive messages
    true, // ------------------------------ send wifi signal strength when heartbeat
    false, // ----------------------------- retain wifi signal messages
    0, // --------------------------------- qos wifi signal messages
    true, // ------------------------------ send available memory heap when heartbeat
    false, // ----------------------------- retain available memory heap messages
    0, // --------------------------------- qos available memory messages
  },
  /* TIMEOUTS CONFIGURATION */
  {
    300000, // ---------------------------- Keep alive timeout (5 min as ms)
    900000, // ---------------------------- Heartbeat timeout  (15 min as ms)
    30000, // ----------------------------- Time to wait between MQTT connection retry attempts
    60000, // ----------------------------- Time to wait before restarting the device after too many failed connect attempts
  },
  /* TOPIC CONFIGURATION */
  {
    "", // ------------------------------- incoming actions topic
    "", // ------------------------------- telemtry topic
    "", // ------------------------------- device events topic
    "", // ------------------------------- device reset reason topic
    "", // ------------------------------- device time alive
    "", // ------------------------------- wifi signal topic
    "", // ------------------------------- memory available topic
  }
};
