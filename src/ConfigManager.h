#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include <Preferences.h>

class ConfigManager {
public:
    bool begin();
    bool save();

    String getWifiSSID() const { return _wifiSSID; }
    String getWifiPassword() const { return _wifiPassword; }
    uint8_t getBrightness() const { return _brightness; }
    String getHostname() const { return _hostname; }
    String getAPPassword() const { return _apPassword; }

    void setWifiSSID(const String& v) { _wifiSSID = v; }
    void setWifiPassword(const String& v) { _wifiPassword = v; }
    void setBrightness(uint8_t v) { _brightness = v; }
    void setHostname(const String& v) { _hostname = v; }
    void setAPPassword(const String& v) { _apPassword = v; }

    bool hasWifiCredentials() const { return _wifiSSID.length() > 0 && _wifiPassword.length() > 0; }

private:
    void saveWifiToNVS();
    void loadWifiFromNVS();

    String _wifiSSID;
    String _wifiPassword;
    uint8_t _brightness = 40;
    String _hostname = "ulanzi";
    String _apPassword = "12345678";
    Preferences _prefs;
};
