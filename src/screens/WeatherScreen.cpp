#include "WeatherScreen.h"
#include "../DisplayManager.h"
#include "../WeatherService.h"
#include "WeatherIconFrames.h"

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

// Weather condition icons are sourced from LaMetric IDs selected in chat,
// pre-baked into frame/color tables in WeatherIconFrames.h (including GIF frames).
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
            if (c != 0x0000) display.drawPixel(ox + x, oy + y, c);
        }
    }
}

WeatherScreen::WeatherKind WeatherScreen::classifyWeather(int weatherCode, float windMph) const {
    if (windMph >= 18.0f) return WeatherKind::WINDY;
    if (weatherCode == 0) return WeatherKind::SUNNY;
    if (weatherCode == 1 || weatherCode == 2) return WeatherKind::PARTLY_CLOUDY;
    if (weatherCode == 3) return WeatherKind::CLOUDY;
    if (weatherCode == 45 || weatherCode == 48) return WeatherKind::FOGGY;
    if ((weatherCode >= 51 && weatherCode <= 67) || (weatherCode >= 80 && weatherCode <= 82)) return WeatherKind::RAINY;
    if ((weatherCode >= 71 && weatherCode <= 77) || weatherCode == 85 || weatherCode == 86) return WeatherKind::SNOWY;
    if (weatherCode == 95 || weatherCode == 96 || weatherCode == 99) return WeatherKind::LIGHTNING;
    return WeatherKind::UNKNOWN;
}

void WeatherScreen::drawWeatherIcon(DisplayManager& display, int16_t ox, int16_t oy) {
    using namespace WeatherIconFrames;

    WeatherKind kind = WeatherKind::UNKNOWN;
    if (_weather) kind = classifyWeather(_weather->getWeatherCode(), _weather->getWindMph());

    const uint16_t (*frames)[64] = nullptr;
    const uint16_t* delays = nullptr;
    uint8_t count = 1;

    switch (kind) {
        case WeatherKind::SUNNY:
            frames = SUNNY_FRAMES; delays = SUNNY_DELAYS_MS; count = SUNNY_FRAME_COUNT; break;
        case WeatherKind::PARTLY_CLOUDY:
            frames = PARTLY_CLOUDY_FRAMES; delays = PARTLY_CLOUDY_DELAYS_MS; count = PARTLY_CLOUDY_FRAME_COUNT; break;
        case WeatherKind::CLOUDY:
            frames = CLOUDY_FRAMES; delays = CLOUDY_DELAYS_MS; count = CLOUDY_FRAME_COUNT; break;
        case WeatherKind::RAINY:
            frames = RAINY_FRAMES; delays = RAINY_DELAYS_MS; count = RAINY_FRAME_COUNT; break;
        case WeatherKind::SNOWY:
            frames = SNOWY_FRAMES; delays = SNOWY_DELAYS_MS; count = SNOWY_FRAME_COUNT; break;
        case WeatherKind::LIGHTNING:
            frames = LIGHTNING_FRAMES; delays = LIGHTNING_DELAYS_MS; count = LIGHTNING_FRAME_COUNT; break;
        case WeatherKind::WINDY:
            frames = WINDY_FRAMES; delays = WINDY_DELAYS_MS; count = WINDY_FRAME_COUNT; break;
        case WeatherKind::FOGGY:
            frames = FOGGY_FRAMES; delays = FOGGY_DELAYS_MS; count = FOGGY_FRAME_COUNT; break;
        default:
            frames = CLOUDY_FRAMES; delays = CLOUDY_DELAYS_MS; count = CLOUDY_FRAME_COUNT; break;
    }

    if (!frames || count == 0) return;

    uint32_t total = 0;
    for (uint8_t i = 0; i < count; i++) total += delays[i];
    if (total == 0) total = 1;

    uint32_t t = millis() % total;
    uint8_t frameIndex = 0;
    uint32_t acc = 0;
    for (uint8_t i = 0; i < count; i++) {
        acc += delays[i];
        if (t < acc) {
            frameIndex = i;
            break;
        }
    }

    const uint16_t* px = frames[frameIndex];
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            uint16_t c = px[y * 8 + x];
            if (c != 0x0000) display.drawPixel(ox + x, oy + y, c);
        }
    }
}

bool WeatherScreen::update(DisplayManager& display, unsigned long now) {
    display.clear();

    if (!_weather || !_weather->hasData()) {
        display.drawFontText(2, 0, "No Data", 0xFFFF, 1);
        return true;
    }

    if (now - _lastFlip >= (unsigned long)_flipInterval * 1000UL) {
        _showHumidity = !_showHumidity;
        _lastFlip = now;
    }

    if (_showHumidity) {
        drawDropletIcon(display, 0, 0, _humIconColor);

        char buf[10];
        int h = (int)(_weather->getHumidity() + 0.5f);
        snprintf(buf, sizeof(buf), "%d%%", h);
        drawValue(display, 10, 0, buf, _valueColor, _fontId);
    } else {
        drawWeatherIcon(display, 0, 0);

        int t = (int)(_weather->getTemperatureF() + 0.5f);
        drawTemperatureValue(display, 10, 0, t, _valueColor, _fontId);
    }

    return true;
}

void WeatherScreen::drawValue(DisplayManager& display, int16_t ox, int16_t oy,
                               const char* text, uint16_t color, uint8_t fontId) {
    uint8_t charW = DisplayManager::fontCharWidth(fontId);
    int16_t textW = strlen(text) * charW;
    int16_t availW = MATRIX_WIDTH - ox;
    int16_t tx = ox + (availW - textW) / 2;
    int16_t ty = (8 - DisplayManager::fontHeight(fontId)) / 2;
    display.drawFontText(tx, ty, text, color, fontId);
}

void WeatherScreen::drawTemperatureValue(DisplayManager& display, int16_t ox, int16_t oy,
                                         int tempF, uint16_t color, uint8_t fontId) {
    char buf[10];
    snprintf(buf, sizeof(buf), "%dF", tempF);
    drawValue(display, ox, oy, buf, color, fontId);
}

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
