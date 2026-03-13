#include "WeatherScreen.h"
#include "../DisplayManager.h"
#include "../WeatherService.h"

void WeatherScreen::setZipCode(const String& zip) {
    _zipCode = zip;
    if (_weather) _weather->setZipCode(zip);
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
        drawDropletIcon(display, 0, 0, _humColor);

        char buf[10];
        int h = (int)(_weather->getHumidity() + 0.5f);
        snprintf(buf, sizeof(buf), "%d%%", h);
        drawValue(display, 10, 0, buf, _humColor, _fontId);
    } else {
        // Temperature page
        drawThermometerIcon(display, 0, 0, _tempColor);

        char buf[10];
        int t = (int)(_weather->getTemperatureF() + 0.5f);
        snprintf(buf, sizeof(buf), "%dF", t);
        drawValue(display, 10, 0, buf, _tempColor, _fontId);
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
    //   .##.....
    //   #..#....
    //   #.##....
    //   #..#....
    //   #.##....
    //   #..#....
    //   ####....
    //   .##.....
    uint16_t dim = 0x4A49; // dim gray for outline
    // Stem (cols 1-2, rows 0-5)
    display.drawPixel(ox+1, oy+0, dim);
    display.drawPixel(ox+2, oy+0, dim);

    display.drawPixel(ox+0, oy+1, dim);
    display.drawPixel(ox+3, oy+1, dim);

    display.drawPixel(ox+0, oy+2, dim);
    display.drawPixel(ox+2, oy+2, color); // fill line
    display.drawPixel(ox+3, oy+2, dim);

    display.drawPixel(ox+0, oy+3, dim);
    display.drawPixel(ox+3, oy+3, dim);

    display.drawPixel(ox+0, oy+4, dim);
    display.drawPixel(ox+2, oy+4, color); // fill line
    display.drawPixel(ox+3, oy+4, dim);

    display.drawPixel(ox+0, oy+5, dim);
    display.drawPixel(ox+3, oy+5, dim);

    // Bulb (rows 6-7)
    display.drawPixel(ox+0, oy+6, color);
    display.drawPixel(ox+1, oy+6, color);
    display.drawPixel(ox+2, oy+6, color);
    display.drawPixel(ox+3, oy+6, color);

    display.drawPixel(ox+1, oy+7, color);
    display.drawPixel(ox+2, oy+7, color);
}

// 8x8 water droplet icon
void WeatherScreen::drawDropletIcon(DisplayManager& display, int16_t ox, int16_t oy, uint16_t color) {
    //   ..#.....
    //   ..#.....
    //   .###....
    //   .###....
    //   #####...
    //   #####...
    //   .###....
    //   ..#.....
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
}
