#ifndef TELEMETRY_NODE_H
#define TELEMETRY_NODE_H

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <RunnableLed.h>

/* import WiFi */
#ifdef ESP8266
#include <ESP8266WiFi.h>  // Include ESP8266-specific header
#elif defined(ESP32)
#include <WiFi.h>         // Include ESP32-specific header
#else
#error "Unsupported platform"
#endif

/* Enum for device events */
enum TelemetryEventType {
    TELEM_EVENT_DEVICE_ONLINE,
    TELEM_EVENT_DEVICE_HEARTBEAT,
    TELEM_EVENT_GPIO,
    EVENT_TELEM_REQUEST_RESPONSE
};

const char* telemEventToString(TelemetryEventType eventType);

struct TelemetryNodeConfig {
  bool     isDebugging;
  char     wifi_ssid[100];
  char     wifi_password[100];
  char     mqtt_broker_ip_addr[100];
  int      mqtt_broker_port;
  char     mqtt_uname[100];
  char     mqtt_pass[100];
  String   mqtt_device_id;
  String   mqtt_device_type;
  bool     mqtt_use_clean_session;
  uint16_t mqtt_connect_reconnect_tries;
  int      mqtt_connect_reconnect_timeout;
  uint16_t mqtt_connect_reconnect_delay;
  String   mqtt_last_will_msg;
  bool     mqtt_last_will_retain; 
  int      mqtt_last_will_qos;
  char     topic_device_telemetry[500];
  char     topic_device_actions[500];
  int      timeout_keep_alive;
  const long timeout_telemetry_heartbeat;
};

class TelemetryNode {
    private:
        /* debugging flags */
        bool isDebugging = true;
        
        /* configuration struct */
        TelemetryNodeConfig telemConfig;

        /* status indicators */
        RunnableLed *ledStatus;

        /* connection variables */
        WiFiClient wiFiClient;
        MqttClient *mqttClient;

        /* methods */
        void _connectToWiFi();
        void _connectToMqttHost(uint8_t attemptNumber);
        void _sendMqttWill();
        void _keepAlive();
        String _getNodeMqttId();
        void _publishHeartbeat();
        void _log(char _message);
        void _logLn(char _message);

        /* timestamps */
        unsigned long tsLastKeepAlive;
        unsigned long tsLastHeartbeat;
        unsigned long tsLastMqttConnAttempt;

    public:
        TelemetryNode(
            WiFiClient _wiFiClient, 
            MqttClient &_mqttClient,
            RunnableLed &_ledStatus,
            TelemetryNodeConfig _telemConfig
        ): wiFiClient(_wiFiClient), mqttClient(&_mqttClient), ledStatus(&_ledStatus), telemConfig(_telemConfig){};
        void connect();
        void run();   
        void publishTelmetryInfo(JsonDocument jsonPayload);
        JsonDocument incomingMqttMessagetoJson(int _messageSize);
        void setDebugging(bool _isDebugging);
};

#endif