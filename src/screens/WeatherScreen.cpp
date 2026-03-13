#include "WeatherScreen.h"
#include "../DisplayManager.h"
#include "../WeatherService.h"

// Lametric icon thumbnails converted to 1-bit 8x8 bitmaps
// 20275 thermometer, 18191 humidity
static const uint8_t LAMETRIC_20275_THERMOMETER[8] = {
    0b00111000,
    0b00101000,
    0b00111000,
    0b00111000,
    0b00111000,
    0b01111100,
    0b01111100,
    0b00111000
};

static const uint8_t LAMETRIC_18191_HUMIDITY[8] = {
    0b00001000,
    0b00011100,
    0b00111110,
    0b01111111,
    0b01111111,
    0b01111111,
    0b01111111,
    0b00111110
};

void WeatherScreen::setZipCode(const String& zip) {
    _zipCode = zip;
    if (_weather) _weather->setZipCode(zip);
}

const uint8_t* WeatherScreen::getLametricIconBitmap(uint32_t id) const {
    switch (id) {
        case 20275: return LAMETRIC_20275_THERMOMETER;
        case 18191: return LAMETRIC_18191_HUMIDITY;
        default: return nullptr;
    }
}

bool WeatherScreen::update(DisplayManager& display, unsigned long now) {
    display.clear();

    if (!_weather || !_weather->hasData()) {
        display.drawFontText(2, 0, "No Data", 0xFFFF, 1);
        return true;
    }

    // Flip between temp and humidity
    if (now - _lastFlip >= (unsigned long)_flipInterval * 1000UL) {
        _showHumidity = !_showHumidity;
        _lastFlip = now;
    }

    if (_showHumidity) {
        // Humidity page
        drawDropletIcon(display, 0, 0, _humIconColor);

        char buf[10];
        int h = (int)(_weather->getHumidity() + 0.5f);
        snprintf(buf, sizeof(buf), "%d%%", h);
        drawValue(display, 10, 0, buf, _valueColor, _fontId);
    } else {
        // Temperature page
        drawThermometerIcon(display, 0, 0, _tempIconColor);

        char buf[10];
        int t = (int)(_weather->getTemperatureF() + 0.5f);
        snprintf(buf, sizeof(buf), "%dF", t);
        drawValue(display, 10, 0, buf, _valueColor, _fontId);
    }

    return true;
}

void WeatherScreen::drawValue(DisplayManager& display, int16_t ox, int16_t oy,
                               const char* text, uint16_t color, uint8_t fontId) {
    uint8_t charW = DisplayManager::fontCharWidth(fontId);
    int16_t textW = strlen(text) * charW;
    // Center in remaining 22px (32 - 10 icon area)
    int16_t availW = MATRIX_WIDTH - ox;
    int16_t tx = ox + (availW - textW) / 2;
    // Center vertically
    int16_t ty = (8 - DisplayManager::fontHeight(fontId)) / 2;
    display.drawFontText(tx, ty, text, color, fontId);
}

// 8x8 thermometer icon
void WeatherScreen::drawThermometerIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color) {
    const uint8_t* icon = getLametricIconBitmap(_tempIconId);
    if (icon) {
        display.drawBitmap(ox, oy, icon, 8, 8, color);
        return;
    }

    // Fallback legacy shape
    uint16_t dim = 0x4A49;
    display.drawPixel(ox+1, oy+0, dim);
    display.drawPixel(ox+2, oy+0, dim);
    display.drawPixel(ox+0, oy+1, dim);
    display.drawPixel(ox+3, oy+1, dim);
    display.drawPixel(ox+0, oy+2, dim);
    display.drawPixel(ox+2, oy+2, color);
    display.drawPixel(ox+3, oy+2, dim);
    display.drawPixel(ox+0, oy+3, dim);
    display.drawPixel(ox+3, oy+3, dim);
    display.drawPixel(ox+0, oy+4, dim);
    display.drawPixel(ox+2, oy+4, color);
    display.drawPixel(ox+3, oy+4, dim);
    display.drawPixel(ox+0, oy+5, dim);
    display.drawPixel(ox+3, oy+5, dim);
    display.drawPixel(ox+0, oy+6, color);
    display.drawPixel(ox+1, oy+6, color);
    display.drawPixel(ox+2, oy+6, color);
    display.drawPixel(ox+3, oy+6, color);
    display.drawPixel(ox+1, oy+7, color);
    display.drawPixel(ox+2, oy+7, color);
}

// 8x8 water droplet icon
void WeatherScreen::drawDropletIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color) {
    const uint8_t* icon = getLametricIconBitmap(_humIconId);
    if (icon) {
        display.drawBitmap(ox, oy, icon, 8, 8, color);
        return;
    }

    // Fallback legacy shape
    display.drawPixel(ox+2, oy+0, color);
    display.drawPixel(ox+2, oy+1, color);
    display.drawPixel(ox+1, oy+2, color);
    display.drawPixel(ox+2, oy+2, color);
    display.drawPixel(ox+3, oy+2, color);
    display.drawPixel(ox+1, oy+3, color);
    display.drawPixel(ox+2, oy+3, color);
    display.drawPixel(ox+3, oy+3, color);
    display.drawPixel(ox+0, oy+4, color);
    display.drawPixel(ox+1, oy+4, color);
    display.drawPixel(ox+2, oy+4, color);
    display.drawPixel(ox+3, oy+4, color);
    display.drawPixel(ox+4, oy+4, color);
    display.drawPixel(ox+0, oy+5, color);
    display.drawPixel(ox+1, oy+5, color);
    display.drawPixel(ox+2, oy+5, color);
    display.drawPixel(ox+3, oy+5, color);
    display.drawPixel(ox+4, oy+5, color);
    display.drawPixel(ox+1, oy+6, color);
    display.drawPixel(ox+2, oy+6, color);
    display.drawPixel(ox+3, oy+6, color);
    display.drawPixel(ox+2, oy+7, color);
}

void WeatherScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["zipCode"].is<const char*>()) {
        _zipCode = cfg["zipCode"].as<String>();
        if (_weather) _weather->setZipCode(_zipCode);
    }
    if (cfg["font"].is<int>()) _fontId = cfg["font"].as<uint8_t>();
    if (cfg["flipInterval"].is<int>()) _flipInterval = cfg["flipInterval"].as<uint16_t>();
    if (cfg["tempIconId"].is<int>()) _tempIconId = cfg["tempIconId"].as<uint32_t>();
    if (cfg["humidityIconId"].is<int>()) _humIconId = cfg["humidityIconId"].as<uint32_t>();
    if (cfg["colors"].is<JsonObjectConst>()) {
        JsonObjectConst colors = cfg["colors"];
        if (colors["temp"].is<const char*>())
            _tempColor = DisplayManager::hexToColor(colors["temp"].as<String>());
        if (colors["humidity"].is<const char*>())
            _humColor = DisplayManager::hexToColor(colors["humidity"].as<String>());
    }
}

void WeatherScreen::serialize(JsonObject& out) const {
    out["zipCode"] = _zipCode;
    out["font"] = _fontId;
    out["flipInterval"] = _flipInterval;
    out["tempIconId"] = _tempIconId;
    out["humidityIconId"] = _humIconId;
}
