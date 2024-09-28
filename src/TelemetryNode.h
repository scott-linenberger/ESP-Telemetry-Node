#ifndef TELEMETRY_NODE_H
#define TELEMETRY_NODE_H

#include <Arduino.h>
#include <ArduinoMqttClient.h>
#include <ArduinoJson.h>
#include <RunnableLed.h>
#include <DebugLogger.h>

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
    EVENT_DEVICE_ONLINE,
    EVENT_DEVICE_RECONNECT,
    EVENT_DEVICE_HEARTBEAT,
    EVENT_TELEMETRY,
    EVENT_TELEM_REQUEST_RESPONSE,
};

/* convenience method for user-friendly enum strings */
const char* telemEventToString(TelemetryEventType eventType);

struct DeviceConfig {
    unsigned long serial_baud_rate;
    bool is_logging;
    bool is_broadcast_heartbeat;
    bool retain_heartbeat;
    uint8_t qos_heartbeat;
    bool is_broadcast_reset_reason;
    bool retain_reset_reason;
    uint8_t qos_reset_reason;
    bool is_broadcast_time_alive;
    bool retain_time_alive;
    uint8_t qos_time_alive;
    bool is_broadcast_wifi_signal;
    bool retain_wifi_signal;
    uint8_t qos_wifi_signal;
    bool is_broadcast_memory_available;
    bool retain_memory_available;
    uint8_t qos_memory_available;
};

struct ConnectionConfig {
    char          wifi_ssid[100];
    char          wifi_password[100];
    char          mqtt_broker_ip_addr[100];
    int           mqtt_broker_port;
    char          mqtt_uname[100];
    char          mqtt_pass[100];
    String        mqtt_client_id;
    bool          mqtt_use_clean_session;
    uint16_t      mqtt_connect_reconnect_tries;
    String        mqtt_last_will_msg;
    bool          mqtt_last_will_retain; 
    int           mqtt_last_will_qos;
};

struct TopicConfig {
    char incoming_actions[200];
    char telemetry[200];
    char device_events[200];
    char device_reset_reason[200];
    char time_alive[200];
    char wifi_signal[200];
    char memory_available[200];
};

struct TimeoutConfig {
    long     keep_alive;
    long     telemetry_heartbeat;
    long     mqtt_reconnect_try;
    uint16_t mqtt_failed_connect_restart_delay;
};

struct TelemetryNodeConfig {
  ConnectionConfig connection;
  DeviceConfig     device;
  TimeoutConfig    timeout;
  TopicConfig      topic;
};

class TelemetryNode {
    private:        
        /* configuration struct */
        TelemetryNodeConfig telemConfig;

        /* deubg logger */
        DebugLogger *log;

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
        void _publishHeartbeat();
        void _log(char _message);
        void _logLn(char _message);
        void _publishDeviceEvent(TelemetryEventType eventType);
        void _publishDeviceResetReason();

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
        ): wiFiClient(_wiFiClient), mqttClient(&_mqttClient), ledStatus(&_ledStatus), telemConfig(_telemConfig){
            /* init the debug logger */
            log = new DebugLogger(telemConfig.device.is_logging);
        };
        TelemetryNode(
            WiFiClient _wiFiClient, 
            MqttClient &_mqttClient,
            TelemetryNodeConfig _telemConfig
        ): wiFiClient(_wiFiClient), mqttClient(&_mqttClient), ledStatus(nullptr), telemConfig(_telemConfig){
            /* init the debug logger */
            log = new DebugLogger(telemConfig.device.is_logging);
        };
        void begin();
        void connect();
        void run();   
        void publishTelmetryInfo(JsonDocument jsonPayload);
        JsonDocument incomingMqttMessagetoJson(int _messageSize);
        void setDebugging(bool _isDebugging);
        void publishWifiSignal();
        void publishMemoryAvailable();
        void publishTimeAlive();
};

#endif