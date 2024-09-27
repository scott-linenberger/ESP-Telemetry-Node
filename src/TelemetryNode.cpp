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

void TelemetryNode::connect() {
    /* clear LEDs */
    ledStatus->off();
    ledStatus->run();

    _connectToWiFi();
    _connectToMqttHost(0);

    /* connected, flash LEDs */
    ledStatus->flashTimes(3, 250);
}

void TelemetryNode::run() {
    yield();
    mqttClient->poll(); // poll the MQTT client to keep the connection alive

    yield();
    ledStatus->run(); // run LEDs to ensure animations work
    yield();

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
    Serial.print("[TelemetryNode]: attempting WiFi connection to SSID: ");
    Serial.println(telemConfig.wifi_ssid);

    // WiFi.mode(WIFI_STA); // may be needed for ESP32s
    WiFi.begin(telemConfig.wifi_ssid, telemConfig.wifi_password);

    // set connection LED flashing
    uint8_t msDelay = 150;
    ledStatus->flashIndefinitely(msDelay);
    unsigned long tsDotLast = msDelay;

    // wait for WiFi to connect
    while (WiFi.status() != WL_CONNECTED) {
        ledStatus->run();

        if (millis() - tsDotLast >= msDelay) {
            Serial.print(".");
            tsDotLast = millis();
        }
    }

    Serial.println("\n[TelemetryNode]: WiFi connected!");
    ledStatus->off();
    ledStatus->run();
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
        Serial.println("[TelemetryNode]: max retries reached! RESTARTING!");
        delay(telemConfig.mqtt_connect_reconnect_delay); // wait for the specified delay, then restart
        ESP.restart();
    }

    Serial.print("[TelemetryNode]: attempting to connect to MQTT host IP ->");
    Serial.print(telemConfig.mqtt_broker_ip_addr);
    Serial.print(" & port -> ");
    Serial.println(telemConfig.mqtt_broker_port);

    Serial.println("[TelemetryNode]: sending LWT");
    _sendMqttWill();
    
    Serial.println("[TelemetryNode]: setting connection vars..");
    // setup connection information
    mqttClient->setCleanSession(telemConfig.mqtt_use_clean_session);

    // convert MQTT ID to char array
    String fullNodeId = _getNodeMqttId();
    uint8_t strLenFullNodeId = fullNodeId.length() + 1;
    char mqttNodeId[strLenFullNodeId];
    fullNodeId.toCharArray(mqttNodeId, strLenFullNodeId);

    // set node ID, username and password
    mqttClient->setId(mqttNodeId);
    mqttClient->setUsernamePassword(telemConfig.mqtt_uname, telemConfig.mqtt_pass);

    Serial.print("[TelemetryNode]: Connecting to MQTT broker with ID -> ");
    Serial.println(mqttNodeId);

    // if the connection failed
    if (!mqttClient->connect(telemConfig.mqtt_broker_ip_addr, telemConfig.mqtt_broker_port)) {
        ledStatus->flashIndefinitely(50);
        Serial.print("[TelemetryNode]: MQTT broker connection FAILED! connection error -> ");
        Serial.println(mqttClient->connectError());

        Serial.println("[TelemetryNodel]: waiting to re-attempt MQTT connection...");
        // wait for the connection timeout
        tsLastMqttConnAttempt = millis();
        while(millis() - tsLastMqttConnAttempt < telemConfig.mqtt_connect_reconnect_timeout) {
            yield();
        }

        _connectToMqttHost(attemptNumber++); // try connecting again
    }

    Serial.println("[TelemetryNode]: MQTT broker connection SUCCESSFUL!");
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
    Serial.println("[TelemetryNode]: running keep alive logic");

    yield();
    Serial.println("[TelemetryNode]: managing MQTT broker connection, checking if connected");

    if (mqttClient->connected()) {
        yield();
        Serial.println("[TelemetryNode]: MQTT client connection OK");
        return;
    }

    Serial.println("[TelemetryNode]: MQTT client NOT CONNECTED! Attempting reconnect...");
     _connectToMqttHost(0); // attempt to connect to the MQTT broker

    if (mqttClient->connected()) {
        yield();
        Serial.println("[TelemetryNode]: MQTT client reconnection SUCCESS");
        return;
    }

    /* MQTT re-connect failed.. */
    Serial.println("[TelemetryNode]: MQTT client reconnect UNSUCCESSFUL:[MAX RECONNECT ATTEMPTS REACHED], performing HARD RESET!");
    ESP.restart();
    yield();
}

String TelemetryNode::_getNodeMqttId() {
  return String(telemConfig.mqtt_device_type + "-" + telemConfig.mqtt_device_id);
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
    jsonPayload["type"] = telemConfig.mqtt_device_type;
    jsonPayload["ver"] = "1.0.0-beta";
    jsonPayload["wifiSignal"] = rssi;
    
    String event = jsonPayload["event"];

    if (event == telemEventToString(TELEM_EVENT_DEVICE_ONLINE)) {
        jsonPayload["resetReason"] = ESP.getResetReason();
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
    ledStatus->flashTimes(1, 250);

    Serial.println("[TelemetryNode]: device telemetry published!");
    
    Serial.print("[TelemetryNode]: telemetry - time alive: ");
    Serial.println(timeAlive);
    
    Serial.print("[TelemetryNode]: telemetry - WiFi signal strength -> ");
    Serial.println(rssi);

    Serial.print("[TelemetryNode]: event -> ");
    Serial.println(event);

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
  Serial.println("[TelemetryNode]: <-INCOMING-MQTT-MESSAGE->");
  
  Serial.print("  [Topic]: ");
  Serial.println(mqttClient->messageTopic());

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

void TelemetryNode::_log(char _msg) {
    if (!isDebugging) {
        return;
    }

    Serial.print(_msg);
}

void TelemetryNode::_logLn(char _msg) {
    if (!isDebugging) {
        return;
    }

    Serial.println(_msg);
}

void TelemetryNode::setDebugging(bool _isDebugging) {
    isDebugging = _isDebugging;
}