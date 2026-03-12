#include "ConfigManager.h"
#include <LittleFS.h>

bool ConfigManager::begin() {
    if (!LittleFS.begin(true)) {
        Serial.println("[Config] LittleFS mount failed");
        return false;
    }
    Serial.println("[Config] LittleFS mounted");

    File file = LittleFS.open("/config.json", "r");
    if (!file) {
        Serial.println("[Config] No config.json, using defaults");
        return true;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, file);
    file.close();

    if (err) {
        Serial.printf("[Config] Parse error: %s, using defaults\n", err.c_str());
        return true;
    }

    _wifiSSID = doc["wifi_ssid"] | "";
    _wifiPassword = doc["wifi_password"] | "";
    _brightness = doc["brightness"] | 40;
    _hostname = (doc["hostname"] | "ulanzi");
    _apPassword = (doc["ap_password"] | "12345678");

    Serial.printf("[Config] Loaded - SSID: %s, brightness: %d, hostname: %s\n",
                  _wifiSSID.c_str(), _brightness, _hostname.c_str());
    return true;
}

bool ConfigManager::save() {
    JsonDocument doc;
    doc["wifi_ssid"] = _wifiSSID;
    doc["wifi_password"] = _wifiPassword;
    doc["brightness"] = _brightness;
    doc["hostname"] = _hostname;
    doc["ap_password"] = _apPassword;

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[Config] Failed to open config.json for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();
    Serial.println("[Config] Saved");
    return true;
}
