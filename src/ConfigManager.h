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
    String getApiToken() const { return _apiToken; }
    bool getMqttEnabled() const { return _mqttEnabled; }
    String getMqttHost() const { return _mqttHost; }
    uint16_t getMqttPort() const { return _mqttPort; }
    String getMqttUser() const { return _mqttUser; }
    String getMqttPassword() const { return _mqttPassword; }
    String getMqttBaseTopic() const { return _mqttBaseTopic; }

    bool getAlarmEnabled() const { return _alarmEnabled; }
    uint8_t getAlarmHour() const { return _alarmHour; }
    uint8_t getAlarmMinute() const { return _alarmMinute; }
    uint8_t getAlarmDaysMask() const { return _alarmDaysMask; }
    String getAlarmFlashMode() const { return _alarmFlashMode; }
    String getAlarmColor() const { return _alarmColor; }
    uint8_t getAlarmVolume() const { return _alarmVolume; }
    String getAlarmTone() const { return _alarmTone; }
    uint8_t getAlarmSnoozeMinutes() const { return _alarmSnoozeMinutes; }
    uint8_t getAlarmTimeoutMinutes() const { return _alarmTimeoutMinutes; }

    void setWifiSSID(const String& v) { _wifiSSID = v; }
    void setWifiPassword(const String& v) { _wifiPassword = v; }
    void setBrightness(uint8_t v) { _brightness = v; }
    void setHostname(const String& v) { _hostname = v; }
    void setAPPassword(const String& v) { _apPassword = v; }
    void setApiToken(const String& v) { _apiToken = v; }
    void setMqttEnabled(bool v) { _mqttEnabled = v; }
    void setMqttHost(const String& v) { _mqttHost = v; }
    void setMqttPort(uint16_t v) { _mqttPort = v; }
    void setMqttUser(const String& v) { _mqttUser = v; }
    void setMqttPassword(const String& v) { _mqttPassword = v; }
    void setMqttBaseTopic(const String& v) { _mqttBaseTopic = v; }

    void setAlarmEnabled(bool v) { _alarmEnabled = v; }
    void setAlarmHour(uint8_t v) { _alarmHour = v; }
    void setAlarmMinute(uint8_t v) { _alarmMinute = v; }
    void setAlarmDaysMask(uint8_t v) { _alarmDaysMask = v; }
    void setAlarmFlashMode(const String& v) { _alarmFlashMode = v; }
    void setAlarmColor(const String& v) { _alarmColor = v; }
    void setAlarmVolume(uint8_t v) { _alarmVolume = v; }
    void setAlarmTone(const String& v) { _alarmTone = v; }
    void setAlarmSnoozeMinutes(uint8_t v) { _alarmSnoozeMinutes = v; }
    void setAlarmTimeoutMinutes(uint8_t v) { _alarmTimeoutMinutes = v; }

    bool hasWifiCredentials() const { return _wifiSSID.length() > 0 && _wifiPassword.length() > 0; }

private:
    void saveWifiToNVS();
    void loadWifiFromNVS();

    String _wifiSSID;
    String _wifiPassword;
    uint8_t _brightness = 40;
    String _hostname = "ulanzi";
    String _apPassword = "12345678";
    String _apiToken = "";

    bool _mqttEnabled = false;
    String _mqttHost = "";
    uint16_t _mqttPort = 1883;
    String _mqttUser = "";
    String _mqttPassword = "";
    String _mqttBaseTopic = "trxr4kdz";

    bool _alarmEnabled = false;
    uint8_t _alarmHour = 7;
    uint8_t _alarmMinute = 0;
    uint8_t _alarmDaysMask = 0b0111110; // Mon-Fri
    String _alarmFlashMode = "solid";   // solid|rainbow
    String _alarmColor = "#FF0000";
    uint8_t _alarmVolume = 80;
    String _alarmTone = "beep";
    uint8_t _alarmSnoozeMinutes = 9;
    uint8_t _alarmTimeoutMinutes = 10;

    Preferences _prefs;
};
