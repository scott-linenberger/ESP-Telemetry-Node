#include <Arduino.h>
#include <TelemetryNode.h>


const TelemetryNodeConfig TELEM_CONFIG = {
  /* CONNECTION */ 
  {
    "wifiSSID", // -------------------------- Wifi SSID
    "wifiPassword", // ---------------------- Wifi Password
      "0.0.0.0", // ------------------------- MQTT Broker IP Address
      1883, // ------------------------------ MQTT Broker Port
      "uname", // --------------------------- MQTT username
      "password", // ------------------------ MQTT password
      "<mqtt-client-id>", // ---------------- MQTT client ID
      false, // ----------------------------- MQTT clean session flag
      5, // --------------------------------- MQTT connection retries before reboot
      {
        true, // ---------------------------- MQTT send last will message
        "EVENT_DEVICE_OFFLINE", // --------------------------- MQTT last will JSON string
        true, // ------------------------------ MQTT last will retain
        1, // --------------------------------- MQTT last will QOS
      }
  },
  /* DEVICE */
  {
    115200, // ---------------------------- Serial baud rate
    true, // ------------------------------ logs Serial output when true, does NOT when false
    /* MQTT Message Settings - Reset Reason */
    true, // ----------------------------- retain reset reason
    0, // --------------------------------- qos reset reason
    true, // ------------------------------ enable heartbeats !! IF HEARTBEATS DISABLED, NO DEVICE METRICS WILL BE PUBLISHED !!
    { /* METRIC - Time Alive */
      true, // ------------------------------ send device time alive with heartbeat
      true, // ------------------------------ retain device time alive messages
      0 // --------------------------------- qos device time alive messages
    },
    { /* METRIC - WiFi Signal */
      true, // ------------------------------ send wifi signal strength when heartbeat
      false, // ----------------------------- retain wifi signal messages
      0 // --------------------------------- qos wifi signal messages
    },
    { /* METRIC - Free Heap Memory */
      true, // ------------------------------ send available memory heap when heartbeat
      false, // ----------------------------- retain available memory heap messages
      0 // --------------------------------- qos available memory messages
    }
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
    "<mqtt-client-id>/actions", // ------------------------------- incoming actions topic
    "<mqtt-client-id>/telemetry", // ------------------------------- telemtry topic
    "<mqtt-client-id>/device/events", // ------------------------------- device events topic
    "<mqtt-client-id>/device/reset", // ------------------------------- device reset reason topic
    "<mqtt-client-id>/device/alive-time", // ------------------------------- device time alive
    "<mqtt-client-id>/device/wifi", // ------------------------------- wifi signal topic
    "<mqtt-client-id>/device/heap", // ------------------------------- memory available topic
  }
};
