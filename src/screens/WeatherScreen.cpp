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

// RGB565 8x8 color maps from Lametric icon thumbs (0x0000 means transparent)
static const uint16_t LAMETRIC_20275_THERMOMETER_RGB565[64] = {
    0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0xFFFF,0x0000,0xFFFF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0xFFFF,0xF002,0xFFFF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0xFFFF,0xF002,0xFFFF,0x0000,0x0000,0x0000,
    0x0000,0x0000,0xFFFF,0xF002,0xFFFF,0x0000,0x0000,0x0000,
    0x0000,0xFFFF,0xF002,0xF002,0xF002,0xFFFF,0x0000,0x0000,
    0x0000,0xFFFF,0xF002,0xF002,0xF002,0xFFFF,0x0000,0x0000,
    0x0000,0x0000,0xFFFF,0xFFFF,0xFFFF,0x0000,0x0000,0x0000,
};

static const uint16_t LAMETRIC_18191_HUMIDITY_RGB565[64] = {
    0x0000,0x0000,0x0000,0x0000,0x041F,0x0000,0x0000,0x0000,
    0x0000,0x0000,0x0000,0x041F,0x041F,0x041F,0x0000,0x0000,
    0x0000,0x0000,0x041F,0x041F,0x041F,0x041F,0x041F,0x0000,
    0x0000,0x041F,0x041F,0x041F,0x041F,0x041F,0x041F,0x041F,
    0x0000,0x041F,0x041F,0x041F,0x46FE,0x46FE,0x041F,0x041F,
    0x0000,0x041F,0x041F,0x46FE,0x46FE,0x46FE,0x46FE,0x041F,
    0x0000,0x041F,0x041F,0x041F,0x46FE,0x46FE,0x041F,0x041F,
    0x0000,0x0000,0x041F,0x041F,0x041F,0x041F,0x041F,0x0000,
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

const uint16_t* WeatherScreen::getLametricIconColor8x8(uint32_t id) const {
    switch (id) {
        case 20275: return LAMETRIC_20275_THERMOMETER_RGB565;
        case 18191: return LAMETRIC_18191_HUMIDITY_RGB565;
        default: return nullptr;
    }
}

void WeatherScreen::drawLametricColorIcon(DisplayManager& display, int16_t ox, int16_t oy, const uint16_t* px) const {
    if (!px) return;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint16_t c = px[y * 8 + x];
            if (c != 0x0000) {
                display.drawPixel(ox + x, oy + y, c);
            }
        }
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
    const uint16_t* iconColor = getLametricIconColor8x8(_tempIconId);
    if (iconColor) {
        drawLametricColorIcon(display, ox, oy, iconColor);
        return;
    }

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
    const uint16_t* iconColor = getLametricIconColor8x8(_humIconId);
    if (iconColor) {
        drawLametricColorIcon(display, ox, oy, iconColor);
        return;
    }

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
        if (colors["temp"].is<const char*>()) {
            uint16_t c = DisplayManager::hexToColor(colors["temp"].as<String>());
            _tempColor = c;
            _tempIconColor = c;
        }
        if (colors["humidity"].is<const char*>()) {
            uint16_t c = DisplayManager::hexToColor(colors["humidity"].as<String>());
            _humColor = c;
            _humIconColor = c;
        }
        if (colors["value"].is<const char*>()) {
            _valueColor = DisplayManager::hexToColor(colors["value"].as<String>());
        }
    }
}

void WeatherScreen::serialize(JsonObject& out) const {
    out["zipCode"] = _zipCode;
    out["font"] = _fontId;
    out["flipInterval"] = _flipInterval;
    out["tempIconId"] = _tempIconId;
    out["humidityIconId"] = _humIconId;

    auto toHex = [](uint16_t c, char* out, size_t len) {
        uint8_t r5 = (c >> 11) & 0x1F;
        uint8_t g6 = (c >> 5) & 0x3F;
        uint8_t b5 = c & 0x1F;
        uint8_t r = (r5 << 3) | (r5 >> 2);
        uint8_t g = (g6 << 2) | (g6 >> 4);
        uint8_t b = (b5 << 3) | (b5 >> 2);
        snprintf(out, len, "#%02X%02X%02X", r, g, b);
    };

    char tempHex[8], humHex[8], valueHex[8];
    toHex(_tempIconColor, tempHex, sizeof(tempHex));
    toHex(_humIconColor, humHex, sizeof(humHex));
    toHex(_valueColor, valueHex, sizeof(valueHex));

    JsonObject colors = out["colors"].to<JsonObject>();
    colors["temp"] = tempHex;
    colors["humidity"] = humHex;
    colors["value"] = valueHex;
}
