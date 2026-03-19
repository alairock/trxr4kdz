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
    enum class WeatherKind : uint8_t {
        SUNNY,
        PARTLY_CLOUDY,
        CLOUDY,
        RAINY,
        SNOWY,
        LIGHTNING,
        WINDY,
        FOGGY,
        UNKNOWN
    };

    const uint8_t* getLametricIconBitmap(uint32_t id) const;
    const uint16_t* getLametricIconColor8x8(uint32_t id) const;
    void drawLametricColorIcon(DisplayManager& display, int16_t ox, int16_t oy, const uint16_t* px) const;
    WeatherKind classifyWeather(int weatherCode, float windMph) const;
    void drawWeatherIcon(DisplayManager& display, int16_t ox, int16_t oy);
    void drawDropletIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color);
    void drawValue(DisplayManager& display, int16_t ox, int16_t oy, const char* text, uint16_t color, uint8_t fontId);
    void drawTemperatureValue(DisplayManager& display, int16_t ox, int16_t oy, int tempF, uint16_t color, uint8_t fontId);

    WeatherService* _weather = nullptr;
    String _zipCode;
    uint16_t _tempColor = 0xFBE0;   // kept for backward compatibility
    uint16_t _humColor = 0x07FF;    // kept for backward compatibility
    uint8_t _fontId = 1;
    uint16_t _tempIconColor = 0xFBE0; // icon-only color (weather icon)
    uint16_t _humIconColor = 0x07FF;  // icon-only color (humidity)
    uint16_t _valueColor = 0xFFFF;    // text color stays neutral
    uint32_t _tempIconId = 20275;   // Lametric thermometer
    uint32_t _humIconId = 18191;    // Lametric humidity

    // Alternate between temp and humidity
    unsigned long _lastFlip = 0;
    bool _showHumidity = false;
    uint16_t _flipInterval = 5; // seconds
};
