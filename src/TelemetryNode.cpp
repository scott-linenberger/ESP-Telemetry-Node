#include "TelemetryNode.h"

/** Returns a user readable representation */
const char* telemEventToString(TelemetryEventType eventType) {
    switch(eventType) {
        case EVENT_DEVICE_ONLINE:
            return "EVENT_DEVICE_ONLINE";

        case EVENT_DEVICE_RECONNECT:
            return "EVENT_DEVICE_RECONNECT";

        case EVENT_DEVICE_HEARTBEAT:
            return "EVENT_DEVICE_HEARTBEAT";

        case EVENT_TELEMETRY:
            return "EVENT_TELEMETRY";

        case EVENT_TELEM_REQUEST_RESPONSE:
            return "EVENT_TELEM_REQUEST_RESPONSE";

        default: 
            return "";
    }
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

void TelemetryNode::begin() {
    /* start the debug logger + Serial */
    log->begin(telemConfig.device.serial_baud_rate);
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
        ledStatus->flashTimes(5, 50);
    }
}

void TelemetryNode::run() {
    yield();
    mqttClient->poll(); // poll the MQTT client to keep the connection alive
    yield();

    /* run LEDs if needed */
    if (ledStatus != nullptr) {
        ledStatus->run(); // run LEDs to ensure animations work
         yield();
    }
  
    /* check if we need to perform keep-alive operations */
    if (millis() - tsLastKeepAlive >= telemConfig.timeout.keep_alive) {
        _keepAlive();
        yield();
        return; // return to shorten loop: prevent keepAlive and heartbeat from happening in the same loop
    }

    /* check if we need to perform heartbeat operations */
    if (millis() - tsLastHeartbeat >= telemConfig.timeout.telemetry_heartbeat) {
        _publishHeartbeat();
        yield();
        return;
    }
    
    yield();
}

void TelemetryNode::_connectToWiFi() {
    log->print("[TelemetryNode]: attempting WiFi connection to SSID: ");
    log->println(telemConfig.connection.wifi_ssid);

    // WiFi.mode(WIFI_STA); // may be needed for ESP32s
    WiFi.begin(telemConfig.connection.wifi_ssid, telemConfig.connection.wifi_password);

    /* controls print output and led flash during connection */
    uint8_t msDelay = 150;
    unsigned long tsDotLast = msDelay; // timeout the dot print delay

    // set connection LED flashing
    if (ledStatus != nullptr) {
        ledStatus->flashIndefinitely(msDelay);
    }

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

    // turn LED off
    if (ledStatus != nullptr) {
        ledStatus->off();
        ledStatus->run();
    }
}

void TelemetryNode::_sendMqttWill() {
    // LWT ---- start the last-will-and-testament for sudden deaths
    mqttClient->beginWill(
        telemConfig.topic.device_events,
        telemConfig.connection.mqtt_last_will_msg.length(),
        telemConfig.connection.mqtt_last_will_retain,
        telemConfig.connection.mqtt_last_will_qos
    );

    // print the last will message
    mqttClient->print(telemConfig.connection.mqtt_last_will_msg);
    mqttClient->endWill();  // lwt is ready!
}

void TelemetryNode::_connectToMqttHost(uint8_t attemptNumber) {
    if (attemptNumber > telemConfig.connection.mqtt_connect_reconnect_tries) {
        log->println("[TelemetryNode]: max retries reached! RESTARTING!");
        delay(telemConfig.timeout.mqtt_failed_connect_restart_delay); // wait for the specified delay, then restart
        ESP.restart();
    }

    log->print("[TelemetryNode]: attempting to connect to MQTT host IP ->");
    log->print(telemConfig.connection.mqtt_broker_ip_addr);
    log->print(" & port -> ");
    log->println(telemConfig.connection.mqtt_broker_port);

    log->println("[TelemetryNode]: sending LWT");
    _sendMqttWill();
    
    log->println("[TelemetryNode]: setting connection vars..");
    // setup connection information
    mqttClient->setCleanSession(telemConfig.connection.mqtt_use_clean_session);

    // set node ID, username and password
    mqttClient->setId(telemConfig.connection.mqtt_client_id);
    mqttClient->setUsernamePassword(telemConfig.connection.mqtt_uname, telemConfig.connection.mqtt_pass);

    log->print("[TelemetryNode]: Connecting to MQTT broker with ID -> ");
    log->println(telemConfig.connection.mqtt_client_id);

    // if the connection failed
    if (!mqttClient->connect(telemConfig.connection.mqtt_broker_ip_addr, telemConfig.connection.mqtt_broker_port)) {
        ledStatus->flashIndefinitely(50);
        log->print("[TelemetryNode]: MQTT broker connection FAILED! connection error -> ");
        log->println(mqttClient->connectError());

        log->println("[TelemetryNodel]: waiting to re-attempt MQTT connection...");
        // wait for the connection timeout
        tsLastMqttConnAttempt = millis();
        while(millis() - tsLastMqttConnAttempt < telemConfig.timeout.mqtt_reconnect_try) {
            yield();
        }

        _connectToMqttHost(attemptNumber++); // try connecting again
    }

    yield();
    log->println("[TelemetryNode]: MQTT broker connection SUCCESSFUL!");
    
    /* broadcast telemetry event - ONLINE */
    _publishDeviceEvent(EVENT_DEVICE_ONLINE);
    _publishDeviceResetReason();
}


void TelemetryNode::_keepAlive() {
    yield();
    log->println("[TelemetryNode]: running keep alive logic");

    yield();
    log->println("[TelemetryNode]: managing MQTT broker connection, checking if connected");

    if (mqttClient->connected()) {
        yield();
        log->println("[TelemetryNode]: MQTT client connection OK");
        tsLastKeepAlive = millis();
        return;
    }

    log->println("[TelemetryNode]: MQTT client NOT CONNECTED! Attempting reconnect...");
     _connectToMqttHost(0); // attempt to connect to the MQTT broker

    if (mqttClient->connected()) {
        yield();
        log->println("[TelemetryNode]: MQTT client reconnection SUCCESS");
        tsLastKeepAlive = millis();
        
        /* broadcast telemetry event - MQTT_RECONNECT */
        _publishDeviceEvent(EVENT_DEVICE_RECONNECT);
        return;
    }

    /* MQTT re-connect failed.. */
    log->println("[TelemetryNode]: MQTT client reconnect UNSUCCESSFUL:[MAX RECONNECT ATTEMPTS REACHED], performing HARD RESET!");
    ESP.restart();
    yield();
}

/** 
 * Publishes device information on a schedule. The information that is published
 * varies based on what is enabled in the configuration
 */
void TelemetryNode::_publishHeartbeat() {
    /* check if node is configured to send heartbeats */
    if (!telemConfig.device.is_broadcast_heartbeat) {
        /* not broadcasting heartbeat, nothing to do */
        return;
    }

    /* device is config'd for heartbeats.. send heartbeat */

    /* publish a heartbeat event */
    _publishDeviceEvent(EVENT_DEVICE_HEARTBEAT);

    /* check if we need to broadcast wifi signal info */
    if (telemConfig.device.is_broadcast_wifi_signal) {
        publishWifiSignal();
    }
    
    if (telemConfig.device.is_broadcast_memory_available) {
        publishMemoryAvailable();
    }

    if (telemConfig.device.is_broadcast_time_alive) {
        publishTimeAlive();
    }


    tsLastHeartbeat = millis();
}


void TelemetryNode::_publishDeviceEvent(TelemetryEventType eventType) {
    yield();
     // publish EVENT
    mqttClient->beginMessage(
        telemConfig.topic.device_events,
        telemConfig.device.retain_heartbeat,
        telemConfig.device.qos_heartbeat
    );

    mqttClient->print(telemEventToString(eventType));
    mqttClient->endMessage();
    mqttClient->flush();

    yield();
}

void TelemetryNode::_publishDeviceResetReason() {
    yield();

     // publish EVENT
    mqttClient->beginMessage(
        telemConfig.topic.device_reset_reason,
        telemConfig.device.retain_reset_reason,
        telemConfig.device.qos_reset_reason
    );

    #if defined(ESP32)
      mqttClient->print(esp_reset_reason());

    #elif defined(ESP8266)
      mqttClient-print(ESP.getResetReason());
    #endif
    
    mqttClient->endMessage();
    mqttClient->flush();

    yield();
}

void TelemetryNode::publishWifiSignal() {
    yield();

    /* get wifi signal strength */
    char* timeAlive = getTimeFromMillis();
    int8_t rssi = WiFi.RSSI();

     // publish EVENT
    mqttClient->beginMessage(
        telemConfig.topic.wifi_signal,
        telemConfig.device.retain_wifi_signal,
        telemConfig.device.qos_wifi_signal
    );

    mqttClient->print(rssi);
    mqttClient->endMessage();
    mqttClient->flush();

    yield();
}

void TelemetryNode::publishMemoryAvailable() {
    yield();

     // publish EVENT
    mqttClient->beginMessage(
        telemConfig.topic.memory_available,
        telemConfig.device.retain_memory_available,
        telemConfig.device.qos_memory_available
    );

    mqttClient->print(999);
    mqttClient->endMessage();
    mqttClient->flush();

    yield();
}

void TelemetryNode::publishTimeAlive() {
    yield();

     // publish EVENT
    mqttClient->beginMessage(
        telemConfig.topic.time_alive,
        telemConfig.device.retain_time_alive,
        telemConfig.device.qos_time_alive
    );

    mqttClient->print(getTimeFromMillis());
    mqttClient->endMessage();
    mqttClient->flush();

    yield();
}

// FIX ME
void TelemetryNode::publishTelmetryInfo(JsonDocument jsonPayload) {

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
