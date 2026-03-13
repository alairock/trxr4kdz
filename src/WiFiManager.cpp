#include "WiFiManager.h"

void WiFiManager::begin(ConfigManager& config) {
    _apPassword = config.getAPPassword();

    if (config.hasWifiCredentials()) {
        // Try STA first
        WiFi.mode(WIFI_STA);
        WiFi.setHostname(config.getHostname().c_str());
        connectSTA(config.getWifiSSID(), config.getWifiPassword());

        // Wait up to 10s for connection
        Serial.print("[WiFi] Waiting for connection");
        unsigned long start = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - start < 10000) {
            delay(250);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.printf("[WiFi] Connected! IP: %s\n", getSTAIP().c_str());
            // Also start AP so user can still access captive portal
            WiFi.mode(WIFI_AP_STA);
            startAP(_apPassword);
        } else {
            Serial.println("[WiFi] STA failed, falling back to AP only");
            WiFi.mode(WIFI_AP);
            startAP(_apPassword);
        }
    } else {
        // No credentials, AP only
        WiFi.mode(WIFI_AP);
        WiFi.setHostname(config.getHostname().c_str());
        startAP(_apPassword);
    }

    Serial.printf("[WiFi] AP: %s  Pass: %s  IP: %s\n",
        WiFi.softAPSSID().c_str(), _apPassword.c_str(), getAPIP().c_str());
}

void WiFiManager::startAP(const String& apPassword) {
    uint8_t mac[6];
    WiFi.macAddress(mac);
    char apName[20];
    snprintf(apName, sizeof(apName), "Ulanzi-%02X%02X", mac[4], mac[5]);

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
