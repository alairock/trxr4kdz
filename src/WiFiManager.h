#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "ConfigManager.h"

class WiFiManager {
public:
    void begin(ConfigManager& config);
    void update();
    void connectSTA(const String& ssid, const String& password);
    String getAPIP() const;
    String getSTAIP() const;
    bool isSTAConnected() const;

private:
    void startAP(const String& apPassword);
    String _ssid;
    String _password;
    String _apPassword;
    unsigned long _lastReconnect = 0;
    static const unsigned long RECONNECT_INTERVAL = 30000;
};
