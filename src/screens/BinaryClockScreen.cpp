#include "BinaryClockScreen.h"
#include "../DisplayManager.h"
#include <time.h>
#include <FastLED.h>

static String color565ToHex(uint16_t c) {
    uint8_t r5 = (c >> 11) & 0x1F;
    uint8_t g6 = (c >> 5) & 0x3F;
    uint8_t b5 = c & 0x1F;
    uint8_t r = (r5 << 3) | (r5 >> 2);
    uint8_t g = (g6 << 2) | (g6 >> 4);
    uint8_t b = (b5 << 3) | (b5 >> 2);
    char out[8];
    snprintf(out, sizeof(out), "#%02X%02X%02X", r, g, b);
    return String(out);
}

uint16_t BinaryClockScreen::colorForMode(IndicatorMode mode, unsigned long now, uint8_t slot) const {
    switch (mode) {
        case IndicatorMode::WHITE: return 0xFFFF;
        case IndicatorMode::RED: return 0xF800;
        case IndicatorMode::GREEN: return 0x07E0;
        case IndicatorMode::BLUE: return 0x001F;
        case IndicatorMode::PINK: return 0xF81F;
        case IndicatorMode::PURPLE: return 0x8010;
        case IndicatorMode::RAINBOW_STATIC: {
            static const uint16_t c[4] = {0xF800, 0xFD20, 0x07E0, 0x001F}; // R,O,G,B
            return c[slot % 4];
        }
        case IndicatorMode::RAINBOW_ANIMATED: {
            uint8_t hue = (uint8_t)((now / 20) + (slot * 48));
            CRGB c = CHSV(hue, 255, 255);
            return DisplayManager::rgb565(c.r, c.g, c.b);
        }
    }
    return 0xF800;
}

BinaryClockScreen::IndicatorMode BinaryClockScreen::modeFromString(const String& s) const {
    String m = s;
    m.toLowerCase();
    if (m == "white") return IndicatorMode::WHITE;
    if (m == "red") return IndicatorMode::RED;
    if (m == "green") return IndicatorMode::GREEN;
    if (m == "blue") return IndicatorMode::BLUE;
    if (m == "pink") return IndicatorMode::PINK;
    if (m == "purple") return IndicatorMode::PURPLE;
    if (m == "rainbow" || m == "rainbow_static" || m == "static_rainbow") return IndicatorMode::RAINBOW_STATIC;
    if (m == "rainbow_animated" || m == "animated_rainbow") return IndicatorMode::RAINBOW_ANIMATED;
    return IndicatorMode::RED;
}

const char* BinaryClockScreen::modeToString(IndicatorMode m) const {
    switch (m) {
        case IndicatorMode::WHITE: return "white";
        case IndicatorMode::RED: return "red";
        case IndicatorMode::GREEN: return "green";
        case IndicatorMode::BLUE: return "blue";
        case IndicatorMode::PINK: return "pink";
        case IndicatorMode::PURPLE: return "purple";
        case IndicatorMode::RAINBOW_STATIC: return "rainbow_static";
        case IndicatorMode::RAINBOW_ANIMATED: return "rainbow_animated";
    }
    return "red";
}

BinaryClockScreen::IndicatorSize BinaryClockScreen::sizeFromValue(JsonVariantConst value) const {
    if (value.is<int>() || value.is<long>() || value.is<unsigned int>() || value.is<unsigned long>()) {
        return value.as<int>() >= 2 ? IndicatorSize::PIXEL_2X2 : IndicatorSize::PIXEL_1X1;
    }

    if (value.is<const char*>()) {
        String s = value.as<String>();
        s.toLowerCase();
        if (s == "2" || s == "2x2" || s == "pixel_2x2") return IndicatorSize::PIXEL_2X2;
        if (s == "1" || s == "1x1" || s == "pixel_1x1") return IndicatorSize::PIXEL_1X1;
    }

    return IndicatorSize::PIXEL_2X2;
}

const char* BinaryClockScreen::sizeToString(IndicatorSize s) const {
    switch (s) {
        case IndicatorSize::PIXEL_1X1: return "1x1";
        case IndicatorSize::PIXEL_2X2: return "2x2";
    }
    return "2x2";
}

bool BinaryClockScreen::update(DisplayManager& display, unsigned long now) {
    display.clear();

    time_t nowSec = time(nullptr);
    struct tm tinfo{};
    localtime_r(&nowSec, &tinfo);

    int h24 = tinfo.tm_hour;
    bool isPM = h24 >= 12;

    int h = h24;
    if (!_use24h) {
        h = h24 % 12;
        if (h == 0) h = 12;
    }

    int digits[6] = {
        h / 10, h % 10,
        tinfo.tm_min / 10, tinfo.tm_min % 10,
        tinfo.tm_sec / 10, tinfo.tm_sec % 10
    };

    // 6 columns of 2px + 1px spacing, plus 2 wider group gaps => 22px total.
    // centered on 32px -> x starts at 5.
    // Per-digit BCD bit heights for HH:MM:SS => 2/4/3/4/3/4
    static const uint8_t bitHeights[6] = {2, 4, 3, 4, 3, 4};

    const int firstColumnX = 6;
    int x = firstColumnX;
    uint8_t slot = 0;
    for (int i = 0; i < 6; i++) {
        for (int bit = 0; bit < bitHeights[i]; bit++) {
            bool on = (digits[i] >> bit) & 1;
            int y = 6 - (bit * 2); // bit0 at bottom rows 6-7
            uint16_t c = _offColor;
            if (on) {
                c = _useOnMode ? colorForMode(_onMode, now, slot) : _onColor;
            }
            display.drawPixel(x, y, c);
            display.drawPixel(x + 1, y, c);
            display.drawPixel(x, y + 1, c);
            display.drawPixel(x + 1, y + 1, c);
            slot++;
        }

        x += 3;
        if (i == 1 || i == 3) x += 1; // extra group spacing between HH MM SS
    }

    // AM/PM indicator above first hour column, with 2 empty pixels before the hour column begins.
    // AM => dim purple, PM => selected indicator mode
    const bool indicator2x2 = _indicatorSize == IndicatorSize::PIXEL_2X2;
    const int indicatorX = firstColumnX;
    const int indicatorY = 0;

    auto drawIndicator = [&](uint16_t c0, uint16_t c1, uint16_t c2, uint16_t c3) {
        if (indicator2x2) {
            display.drawPixel(indicatorX, indicatorY, c0);
            display.drawPixel(indicatorX + 1, indicatorY, c1);
            display.drawPixel(indicatorX, indicatorY + 1, c2);
            display.drawPixel(indicatorX + 1, indicatorY + 1, c3);
        } else {
            display.drawPixel(indicatorX, indicatorY, c0);
        }
    };

    if (!isPM) {
        drawIndicator(_offColor, _offColor, _offColor, _offColor);
    } else {
        if (_indicatorMode == IndicatorMode::RAINBOW_STATIC || _indicatorMode == IndicatorMode::RAINBOW_ANIMATED) {
            drawIndicator(
                colorForMode(_indicatorMode, now, 0),
                colorForMode(_indicatorMode, now, 1),
                colorForMode(_indicatorMode, now, 2),
                colorForMode(_indicatorMode, now, 3)
            );
        } else {
            uint16_t ap = colorForMode(_indicatorMode, now, 0);
            drawIndicator(ap, ap, ap, ap);
        }
    }

    return true;
}

void BinaryClockScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["use24h"].is<bool>()) _use24h = cfg["use24h"].as<bool>();
    if (cfg["onColor"].is<const char*>()) {
        _onColor = DisplayManager::hexToColor(cfg["onColor"].as<String>());
        _useOnMode = false;
    }
    if (cfg["offColor"].is<const char*>()) _offColor = DisplayManager::hexToColor(cfg["offColor"].as<String>());

    if (cfg["indicatorMode"].is<const char*>()) {
        _indicatorMode = modeFromString(cfg["indicatorMode"].as<String>());
    }

    if (!cfg["indicatorSize"].isNull()) {
        _indicatorSize = sizeFromValue(cfg["indicatorSize"]);
    }

    if (cfg["onMode"].is<const char*>()) {
        String m = cfg["onMode"].as<String>();
        m.toLowerCase();
        if (m == "custom") {
            _useOnMode = false;
        } else {
            _onMode = modeFromString(m);
            _useOnMode = true;
        }
    }
}

void BinaryClockScreen::serialize(JsonObject& out) const {
    out["use24h"] = _use24h;
    out["indicatorMode"] = modeToString(_indicatorMode);
    out["indicatorSize"] = sizeToString(_indicatorSize);
    out["onMode"] = _useOnMode ? modeToString(_onMode) : "custom";
    out["onColor"] = color565ToHex(_onColor);
    out["offColor"] = color565ToHex(_offColor);
}
