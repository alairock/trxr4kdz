#pragma once

#include <Arduino.h>

class WeatherService {
public:
    void begin();
    void update();  // call from loop, throttles internally

    // Configure
    void setZipCode(const String& zip) { _zipCode = zip; _needGeocode = true; }
    String getZipCode() const { return _zipCode; }
    bool hasZipCode() const { return _zipCode.length() > 0; }

    // Data
    float getTemperatureF() const { return _tempF; }
    float getHumidity() const { return _humidity; }
    bool hasData() const { return _hasData; }
    bool isOnline() const { return _online; }
    bool hasGeocode() const { return _hasGeocode; }
    bool needsGeocode() const { return _needGeocode; }
    float getLat() const { return _lat; }
    float getLon() const { return _lon; }
    String getLastError() const { return _lastError; }

private:
    void readSensor();
    void fetchOnline();
    bool geocodeZip();

    String _zipCode;
    float _lat = 0, _lon = 0;
    bool _needGeocode = false;
    bool _hasGeocode = false;

    float _tempF = 0;
    float _humidity = 0;
    bool _hasData = false;
    bool _online = false;

    bool _sensorAvailable = false;
    String _lastError;

    unsigned long _lastFetch = 0;
    unsigned long _lastSensorRead = 0;
    static const unsigned long FETCH_INTERVAL = 300000;   // 5 min
    static const unsigned long SENSOR_INTERVAL = 10000;   // 10 sec
};
