#include "WeatherService.h"
#include <Wire.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "pins.h"

#define SHT30_ADDR 0x44

void WeatherService::begin() {
    // Init I2C for SHT30 sensor
    Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);

    // Check if SHT30 is present
    Wire.beginTransmission(SHT30_ADDR);
    _sensorAvailable = (Wire.endTransmission() == 0);
    Serial.printf("[Weather] SHT30 sensor %s\n", _sensorAvailable ? "found" : "not found");
}

void WeatherService::update() {
    unsigned long now = millis();

    // Geocode zip if needed
    if (_needGeocode && _zipCode.length() > 0) {
        Serial.printf("[Weather] Geocoding zip: %s\n", _zipCode.c_str());
        if (geocodeZip()) {
            _needGeocode = false;
            _hasGeocode = true;
            _lastFetch = 0; // force immediate fetch
            Serial.printf("[Weather] Geocode success, fetching weather now\n");
        } else {
            _needGeocode = false; // don't retry every loop
            Serial.printf("[Weather] Geocode FAILED\n");
        }
    }

    // Fetch online weather
    if (_hasGeocode && (now - _lastFetch >= FETCH_INTERVAL || _lastFetch == 0)) {
        Serial.printf("[Weather] Fetching online weather...\n");
        fetchOnline();
        _lastFetch = now;
    }

    // Read sensor as fallback (or if no zip configured)
    if (!_online && _sensorAvailable && (now - _lastSensorRead >= SENSOR_INTERVAL || _lastSensorRead == 0)) {
        readSensor();
        _lastSensorRead = now;
    }
}

void WeatherService::readSensor() {
    // SHT30 single shot measurement, high repeatability
    Wire.beginTransmission(SHT30_ADDR);
    Wire.write(0x2C);
    Wire.write(0x06);
    if (Wire.endTransmission() != 0) return;

    delay(20); // measurement time

    Wire.requestFrom((uint8_t)SHT30_ADDR, (uint8_t)6);
    if (Wire.available() < 6) return;

    uint8_t data[6];
    for (int i = 0; i < 6; i++) data[i] = Wire.read();

    // Convert temperature
    float tempC = ((((data[0] * 256) + data[1]) * 175.0) / 65535.0) - 45.0;
    _tempF = tempC * 9.0 / 5.0 + 32.0;

    // Convert humidity
    _humidity = ((data[3] * 256) + data[4]) * 100.0 / 65535.0;

    _hasData = true;
    _online = false;
    Serial.printf("[Weather] Sensor: %.1f°F, %.0f%%\n", _tempF, _humidity);
}

bool WeatherService::geocodeZip() {
    HTTPClient http;
    String url = "http://api.zippopotam.us/us/" + _zipCode;
    http.begin(url);
    http.setTimeout(5000);
    int code = http.GET();

    if (code != 200) {
        _lastError = "geocode HTTP " + String(code);
        Serial.printf("[Weather] Geocode failed: %d\n", code);
        http.end();
        return false;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, http.getStream());
    http.end();

    if (err) {
        Serial.printf("[Weather] Geocode parse error: %s\n", err.c_str());
        return false;
    }

    JsonArray places = doc["places"].as<JsonArray>();
    if (!places || places.size() == 0) return false;

    const char* latStr = places[0]["latitude"];
    const char* lonStr = places[0]["longitude"];
    if (!latStr || !lonStr) return false;

    _lat = atof(latStr);
    _lon = atof(lonStr);
    Serial.printf("[Weather] Geocoded %s -> %.4f, %.4f\n", _zipCode.c_str(), _lat, _lon);
    return true;
}

void WeatherService::fetchOnline() {
    WiFiClientSecure client;
    client.setInsecure(); // skip cert verification for Open-Meteo
    HTTPClient http;
    // Open-Meteo: free, no API key, returns current temp + humidity
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" +
                 String(_lat, 4) + "&longitude=" + String(_lon, 4) +
                 "&current=temperature_2m,relative_humidity_2m&temperature_unit=fahrenheit";

    http.begin(client, url);
    http.setTimeout(5000);
    int code = http.GET();

    if (code != 200) {
        _lastError = "fetch HTTP " + String(code);
        Serial.printf("[Weather] Fetch failed: %d\n", code);
        http.end();
        _online = false;
        return;
    }

    String payload = http.getString();
    http.end();
    Serial.printf("[Weather] Response (%d bytes): %.100s\n", payload.length(), payload.c_str());

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, payload);

    if (err) {
        _lastError = "parse:" + String(err.c_str()) + " body:" + payload.substring(0, 80);
        Serial.printf("[Weather] Parse error: %s\n", err.c_str());
        _online = false;
        return;
    }

    JsonObject current = doc["current"];
    if (!current) {
        _lastError = "fetch: no 'current' key";
        _online = false;
        return;
    }

    _tempF = current["temperature_2m"] | 0.0f;
    _humidity = current["relative_humidity_2m"] | 0.0f;
    _hasData = true;
    _online = true;
    _lastError = "";
    Serial.printf("[Weather] Online: %.1f°F, %.0f%%\n", _tempF, _humidity);
}
