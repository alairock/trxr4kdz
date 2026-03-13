#pragma once

#include "Screen.h"

class WeatherService;

class WeatherScreen : public Screen {
public:
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::WEATHER; }
    const char* typeName() const override { return "weather"; }

    void setWeatherService(WeatherService* ws) { _weather = ws; }

    String getZipCode() const { return _zipCode; }
    void setZipCode(const String& zip);

private:
    void drawThermometerIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color);
    void drawDropletIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color);
    void drawValue(DisplayManager& display, int16_t ox, int16_t oy, const char* text, uint16_t color, uint8_t fontId);

    WeatherService* _weather = nullptr;
    String _zipCode;
    uint16_t _tempColor = 0xFBE0;   // orange
    uint16_t _humColor = 0x07FF;    // cyan
    uint8_t _fontId = 1;

    // Alternate between temp and humidity
    unsigned long _lastFlip = 0;
    bool _showHumidity = false;
    uint16_t _flipInterval = 5; // seconds
};
