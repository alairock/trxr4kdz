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
        Serial.println("[Config] No config.json, checking NVS backup");
        loadWifiFromNVS();
        if (_wifiSSID.length() > 0) {
            Serial.printf("[Config] Recovered WiFi from NVS: %s\n", _wifiSSID.c_str());
            save();  // Re-create config.json from NVS backup
        }
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
    _apiToken = (doc["api_token"] | "");

    _mqttEnabled = doc["mqtt_enabled"] | false;
    _mqttHost = doc["mqtt_host"] | "";
    _mqttPort = doc["mqtt_port"] | 1883;
    _mqttUser = doc["mqtt_user"] | "";
    _mqttPassword = doc["mqtt_password"] | "";
    _mqttBaseTopic = doc["mqtt_base_topic"] | "trxr4kdz";

    _alarmEnabled = doc["alarm_enabled"] | false;
    _alarmHour = doc["alarm_hour"] | 7;
    _alarmMinute = doc["alarm_minute"] | 0;
    _alarmDaysMask = doc["alarm_days_mask"] | 0b0111110;
    _alarmFlashMode = doc["alarm_flash_mode"] | "solid";
    _alarmColor = doc["alarm_color"] | "#FF0000";
    _alarmVolume = doc["alarm_volume"] | 80;
    _alarmTone = doc["alarm_tone"] | "beep";
    _alarmSnoozeMinutes = doc["alarm_snooze_minutes"] | 9;
    _alarmTimeoutMinutes = doc["alarm_timeout_minutes"] | 10;

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
    doc["api_token"] = _apiToken;

    doc["mqtt_enabled"] = _mqttEnabled;
    doc["mqtt_host"] = _mqttHost;
    doc["mqtt_port"] = _mqttPort;
    doc["mqtt_user"] = _mqttUser;
    doc["mqtt_password"] = _mqttPassword;
    doc["mqtt_base_topic"] = _mqttBaseTopic;

    doc["alarm_enabled"] = _alarmEnabled;
    doc["alarm_hour"] = _alarmHour;
    doc["alarm_minute"] = _alarmMinute;
    doc["alarm_days_mask"] = _alarmDaysMask;
    doc["alarm_flash_mode"] = _alarmFlashMode;
    doc["alarm_color"] = _alarmColor;
    doc["alarm_volume"] = _alarmVolume;
    doc["alarm_tone"] = _alarmTone;
    doc["alarm_snooze_minutes"] = _alarmSnoozeMinutes;
    doc["alarm_timeout_minutes"] = _alarmTimeoutMinutes;

    File file = LittleFS.open("/config.json", "w");
    if (!file) {
        Serial.println("[Config] Failed to open config.json for writing");
        return false;
    }

    serializeJsonPretty(doc, file);
    file.close();
    Serial.println("[Config] Saved");

    // Back up WiFi credentials to NVS (survives LittleFS reformat)
    saveWifiToNVS();

    return true;
}

void ConfigManager::saveWifiToNVS() {
    _prefs.begin("wifi", false);
    _prefs.putString("ssid", _wifiSSID);
    _prefs.putString("pass", _wifiPassword);
    _prefs.end();
}

void ConfigManager::loadWifiFromNVS() {
    _prefs.begin("wifi", true);
    _wifiSSID = _prefs.getString("ssid", "");
    _wifiPassword = _prefs.getString("pass", "");
    _prefs.end();
}
