#include "BinaryClockScreen.h"
#include "../DisplayManager.h"
#include <time.h>
#include <FastLED.h>

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
    return _pmColor;
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
    int x = 5;
    for (int i = 0; i < 6; i++) {
        for (int bit = 0; bit < 4; bit++) {
            bool on = (digits[i] >> bit) & 1;
            int y = 6 - (bit * 2); // bit0 at bottom rows 6-7
            uint16_t c = on ? _onColor : _offColor;
            display.drawPixel(x, y, c);
            display.drawPixel(x + 1, y, c);
            display.drawPixel(x, y + 1, c);
            display.drawPixel(x + 1, y + 1, c);
        }

        x += 3;
        if (i == 1 || i == 3) x += 1; // extra group spacing between HH MM SS
    }

    // AM/PM indicator at top-left:
    // AM => dim purple, PM => selected indicator mode
    if (!isPM) {
        display.drawPixel(0, 0, _offColor);
        display.drawPixel(1, 0, _offColor);
        display.drawPixel(0, 1, _offColor);
        display.drawPixel(1, 1, _offColor);
    } else {
        if (_indicatorMode == IndicatorMode::RAINBOW_STATIC || _indicatorMode == IndicatorMode::RAINBOW_ANIMATED) {
            display.drawPixel(0, 0, colorForMode(_indicatorMode, now, 0));
            display.drawPixel(1, 0, colorForMode(_indicatorMode, now, 1));
            display.drawPixel(0, 1, colorForMode(_indicatorMode, now, 2));
            display.drawPixel(1, 1, colorForMode(_indicatorMode, now, 3));
        } else {
            uint16_t ap = colorForMode(_indicatorMode, now, 0);
            display.drawPixel(0, 0, ap);
            display.drawPixel(1, 0, ap);
            display.drawPixel(0, 1, ap);
            display.drawPixel(1, 1, ap);
        }
    }

    return true;
}

void BinaryClockScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["use24h"].is<bool>()) _use24h = cfg["use24h"].as<bool>();
    if (cfg["onColor"].is<const char*>()) _onColor = DisplayManager::hexToColor(cfg["onColor"].as<String>());
    if (cfg["offColor"].is<const char*>()) _offColor = DisplayManager::hexToColor(cfg["offColor"].as<String>());
    if (cfg["pmColor"].is<const char*>()) _pmColor = DisplayManager::hexToColor(cfg["pmColor"].as<String>());

    if (cfg["indicatorMode"].is<const char*>()) {
        _indicatorMode = modeFromString(cfg["indicatorMode"].as<String>());
    }
}

void BinaryClockScreen::serialize(JsonObject& out) const {
    out["use24h"] = _use24h;
    out["indicatorMode"] = modeToString(_indicatorMode);
}
