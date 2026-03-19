#pragma once

#include <Arduino.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include "ConfigManager.h"
#include "screens/ScreenManager.h"
#include "OvertakeManager.h"

class MqttManager {
public:
    void begin(ConfigManager& config, ScreenManager& screens, OvertakeManager& overtake);
    void update();
    static MqttManager* current() { return _instance; }

    bool isConnected() { return _mqtt.connected(); }
    int lastState() const { return _lastState; }
    String lastError() const { return _lastError; }
    unsigned long lastConnectMs() const { return _lastConnectMs; }
    unsigned long reconnectAttempts() const { return _reconnectAttempts; }
    String lastTopic() const { return _lastTopic; }
    unsigned int lastPayloadLen() const { return _lastPayloadLen; }
    unsigned long lastMessageMs() const { return _lastMessageMs; }

private:
    static void mqttCallbackThunk(char* topic, uint8_t* payload, unsigned int length);
    void mqttCallback(char* topic, uint8_t* payload, unsigned int length);
    void reconnectIfNeeded();
    String topicPrefix() const;

    static MqttManager* _instance;

    ConfigManager* _config = nullptr;
    ScreenManager* _screens = nullptr;
    OvertakeManager* _overtake = nullptr;
    WiFiClient _net;
    PubSubClient _mqtt{_net};
    String _serverHostCache;
    unsigned long _lastReconnectTry = 0;
    unsigned long _lastConnectMs = 0;
    unsigned long _reconnectAttempts = 0;
    int _lastState = 0;
    String _lastError = "";
    String _lastTopic = "";
    unsigned int _lastPayloadLen = 0;
    unsigned long _lastMessageMs = 0;
};
