#include "TelemetryNode.h"

/** Returns a user readable representation */
const char* telemEventToString(TelemetryEventType eventType) {
    switch(eventType) {
        case TELEM_EVENT_DEVICE_ONLINE:
            return "EVENT_DEVICE_ONLINE";

        case TELEM_EVENT_DEVICE_HEARTBEAT:
            return "EVENT_DEVICE_HEARTBEAT";

        case TELEM_EVENT_GPIO:
            return "EVENT_DEVICE_GPIO";

        case EVENT_TELEM_REQUEST_RESPONSE:
            return "EVENT_TELEM_REQUEST_RESPONSE";

        default: 
            return "";
    }
}

void TelemetryNode::begin() {
    /* start the debug logger + Serial */
    log->begin(telemConfig.serial_baud_rate);
}

void TelemetryNode::connect() {
    if (ledStatus != nullptr) {
        /* clear LEDs */
        ledStatus->off();
        ledStatus->run();
    }

    _connectToWiFi();
    _connectToMqttHost(0);

    if (ledStatus != nullptr) {
        /* connected, flash LEDs */
        ledStatus->flashTimes(3, 250);
    }
}

void TelemetryNode::run() {
    yield();
    mqttClient->poll(); // poll the MQTT client to keep the connection alive

    yield();
    if (ledStatus != nullptr) {
        ledStatus->run(); // run LEDs to ensure animations work
         yield();
    }
  

    if (millis() - tsLastKeepAlive >= telemConfig.timeout_keep_alive) {
        _keepAlive();
        tsLastKeepAlive = millis();
        return; // return to shorten loop time and prevent keepAlive and heartbeat from happening in the same loop
    }

    if (millis() - tsLastHeartbeat >= telemConfig.timeout_telemetry_heartbeat) {
        _publishHeartbeat();
        tsLastHeartbeat = millis();
    }
    yield();
}

void TelemetryNode::_connectToWiFi() {
    log->print("[TelemetryNode]: attempting WiFi connection to SSID: ");
    log->println(telemConfig.wifi_ssid);

    // WiFi.mode(WIFI_STA); // may be needed for ESP32s
    WiFi.begin(telemConfig.wifi_ssid, telemConfig.wifi_password);

    uint8_t msDelay = 150;

    // set connection LED flashing
    if (ledStatus != nullptr) {
        ledStatus->flashIndefinitely(msDelay);
    }

    unsigned long tsDotLast = msDelay;

    // wait for WiFi to connect
    while (WiFi.status() != WL_CONNECTED) {
        if (ledStatus != nullptr) {
            ledStatus->run();
        }

        if (millis() - tsDotLast >= msDelay) {
            log->print(".");
            tsDotLast = millis();
        }
    }

    log->println("\n[TelemetryNode]: WiFi connected!");

    if (ledStatus != nullptr) {
        ledStatus->off();
        ledStatus->run();
    }
}

void TelemetryNode::_sendMqttWill() {
    // LWT ---- start the last-will-and-testament for sudden deaths
    mqttClient->beginWill(
        telemConfig.topic_device_telemetry,
        telemConfig.mqtt_last_will_msg.length(),
        telemConfig.mqtt_last_will_retain
    );

    // print the last will message
    mqttClient->print(telemConfig.mqtt_last_will_msg);
    mqttClient->endWill();  // lwt is ready!
}

void TelemetryNode::_connectToMqttHost(uint8_t attemptNumber) {
    if (attemptNumber > telemConfig.mqtt_connect_reconnect_tries) {
        log->println("[TelemetryNode]: max retries reached! RESTARTING!");
        delay(telemConfig.mqtt_connect_reconnect_delay); // wait for the specified delay, then restart
        ESP.restart();
    }

    log->print("[TelemetryNode]: attempting to connect to MQTT host IP ->");
    log->print(telemConfig.mqtt_broker_ip_addr);
    log->print(" & port -> ");
    log->println(telemConfig.mqtt_broker_port);

    log->println("[TelemetryNode]: sending LWT");
    _sendMqttWill();
    
    log->println("[TelemetryNode]: setting connection vars..");
    // setup connection information
    mqttClient->setCleanSession(telemConfig.mqtt_use_clean_session);

    // set node ID, username and password
    mqttClient->setId(telemConfig.mqtt_device_id);
    mqttClient->setUsernamePassword(telemConfig.mqtt_uname, telemConfig.mqtt_pass);

    log->print("[TelemetryNode]: Connecting to MQTT broker with ID -> ");
    log->println(telemConfig.mqtt_device_id);

    // if the connection failed
    if (!mqttClient->connect(telemConfig.mqtt_broker_ip_addr, telemConfig.mqtt_broker_port)) {
        ledStatus->flashIndefinitely(50);
        log->print("[TelemetryNode]: MQTT broker connection FAILED! connection error -> ");
        log->println(mqttClient->connectError());

        log->println("[TelemetryNodel]: waiting to re-attempt MQTT connection...");
        // wait for the connection timeout
        tsLastMqttConnAttempt = millis();
        while(millis() - tsLastMqttConnAttempt < telemConfig.mqtt_connect_reconnect_timeout) {
            yield();
        }

        _connectToMqttHost(attemptNumber++); // try connecting again
    }

    log->println("[TelemetryNode]: MQTT broker connection SUCCESSFUL!");
    /* broadcast telemetry event - ONLINE */
    JsonDocument jsonPayload;

    /* publish telmetry information */
    jsonPayload["event"] = telemEventToString(TELEM_EVENT_DEVICE_ONLINE);

    yield();
    /* publish telmetry information */
    publishTelmetryInfo(jsonPayload);
}

void TelemetryNode::_keepAlive() {
    yield();
    log->println("[TelemetryNode]: running keep alive logic");

    yield();
    log->println("[TelemetryNode]: managing MQTT broker connection, checking if connected");

    if (mqttClient->connected()) {
        yield();
        log->println("[TelemetryNode]: MQTT client connection OK");
        return;
    }

    log->println("[TelemetryNode]: MQTT client NOT CONNECTED! Attempting reconnect...");
     _connectToMqttHost(0); // attempt to connect to the MQTT broker

    if (mqttClient->connected()) {
        yield();
        log->println("[TelemetryNode]: MQTT client reconnection SUCCESS");
        return;
    }

    /* MQTT re-connect failed.. */
    log->println("[TelemetryNode]: MQTT client reconnect UNSUCCESSFUL:[MAX RECONNECT ATTEMPTS REACHED], performing HARD RESET!");
    ESP.restart();
    yield();
}

char* getTimeFromMillis() {
    unsigned long milliseconds = millis();
    static char timeString[9]; // Buffer for "HH:MM:SS\0"

    // Calculate hours, minutes, and seconds
    unsigned long totalSeconds = milliseconds / 1000;
    int hours = totalSeconds / 3600;
    int minutes = (totalSeconds % 3600) / 60;
    int seconds = totalSeconds % 60;

    // Format the time as "HH:MM:SS"
    snprintf(timeString, sizeof(timeString), "%02d:%02d:%02d", hours, minutes, seconds);

    return timeString;
}

void TelemetryNode::publishTelmetryInfo(JsonDocument jsonPayload) {
    yield();

    // add WiFi connection information to outgoing telemetry data
    char* timeAlive = getTimeFromMillis();
    int8_t rssi = WiFi.RSSI();

    jsonPayload["id"] = telemConfig.mqtt_device_id;

    /* get chip info from the board */
    #ifdef ESP8266
        jsonPayload["type"] = "ESP8266-" + String(ESP.getCoreVersion());
    #elif defined(ESP32)
        jsonPayload["type"] = String(ESP.getChipModel()) + "-" + String(ESP.getCoreVersion());
    #else
        jsonPayload["type"] = "uknown";
    #endif

    jsonPayload["ver"] = "1.0.0-beta";
    jsonPayload["wifiSignal"] = rssi;
    
    String event = jsonPayload["event"];

    if (event == telemEventToString(TELEM_EVENT_DEVICE_ONLINE)) {
        #ifdef ESP8266
            jsonPayload["resetReason"] = ESP.getResetReason();
        #elif defined(ESP32)
            jsonPayload["resetReason"] = esp_reset_reason_t();
        #else
            jsonPayload["resetReason"] = "uknown";
        #endif

        // include action path on EVENT_DEVICE_ONLINE events
        jsonPayload["actions"] = telemConfig.topic_device_actions;
    } else {
        jsonPayload["timeAlive"] = timeAlive;
        jsonPayload["memAvailable"] = ESP.getFreeHeap();
    }
   
    // publish the telemetry info
    mqttClient->beginMessage(
        telemConfig.topic_device_telemetry,
        telemConfig.mqtt_last_will_retain,
        telemConfig.mqtt_last_will_qos
    );

    serializeJson(jsonPayload, *mqttClient);
    mqttClient->endMessage();
    mqttClient->flush();

    // flash LEDs
    if (ledStatus != nullptr) {
        ledStatus->flashTimes(1, 250);
    }

    log->println("[TelemetryNode]: device telemetry published!");
    
    log->print("[TelemetryNode]: telemetry - time alive: ");
    log->println(timeAlive);
    
    log->print("[TelemetryNode]: telemetry - WiFi signal strength -> ");
    log->println(rssi);

    log->print("[TelemetryNode]: event -> ");
    log->println(event);

    yield();
    yield();
}

void TelemetryNode::_publishHeartbeat() {
    JsonDocument jsonPayload;
    jsonPayload["event"] = telemEventToString(TELEM_EVENT_DEVICE_HEARTBEAT);
    publishTelmetryInfo(jsonPayload);
}

JsonDocument TelemetryNode::incomingMqttMessagetoJson(int _messageSize) {
  // we received a message, print out the topic and contents
  log->println("[TelemetryNode]: <-INCOMING-MQTT-MESSAGE->");
  
  log->print("  [Topic]: ");
  log->println(mqttClient->messageTopic());

  // convert the message into a string
  char payloadString[_messageSize];

  // use the Stream interface to print the contents
  for (int i = 0; i < _messageSize; i++) {
    payloadString[i] = (char)mqttClient->read();
  }

  // parse the string into JSON
  JsonDocument json;
  deserializeJson(json, payloadString);

  return json;
}

void TelemetryNode::setDebugging(bool _isDebugging) {
    log->setLogging(_isDebugging);
}
