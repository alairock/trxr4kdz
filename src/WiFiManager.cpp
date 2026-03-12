#include "WiFiManager.h"

void WiFiManager::begin(ConfigManager& config) {
    startAP(config.getAPPassword());

    if (config.hasWifiCredentials()) {
        WiFi.mode(WIFI_AP_STA);
        connectSTA(config.getWifiSSID(), config.getWifiPassword());
    }

    WiFi.setHostname(config.getHostname().c_str());
    Serial.printf("[WiFi] AP IP: %s\n", getAPIP().c_str());
}

void WiFiManager::startAP(const String& apPassword) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[20];
    snprintf(apName, sizeof(apName), "Ulanzi-%02X%02X", mac[4], mac[5]);

    WiFi.mode(WIFI_AP);
    WiFi.softAP(apName, apPassword.c_str());
    Serial.printf("[WiFi] AP started: %s\n", apName);
}

void WiFiManager::connectSTA(const String& ssid, const String& password) {
    _ssid = ssid;
    _password = password;
    WiFi.begin(_ssid.c_str(), _password.c_str());
    _lastReconnect = millis();
    Serial.printf("[WiFi] Connecting to: %s\n", _ssid.c_str());
}

void WiFiManager::update() {
    if (_ssid.length() == 0) return;
    if (WiFi.status() == WL_CONNECTED) return;

    unsigned long now = millis();
    if (now - _lastReconnect >= RECONNECT_INTERVAL) {
        Serial.println("[WiFi] Reconnecting STA...");
        WiFi.begin(_ssid.c_str(), _password.c_str());
        _lastReconnect = now;
    }
}

String WiFiManager::getAPIP() const {
    return WiFi.softAPIP().toString();
}

String WiFiManager::getSTAIP() const {
    return WiFi.localIP().toString();
}

bool WiFiManager::isSTAConnected() const {
    return WiFi.status() == WL_CONNECTED;
}
