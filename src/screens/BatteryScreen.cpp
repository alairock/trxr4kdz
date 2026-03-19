#include "BatteryScreen.h"
#include "../DisplayManager.h"
#include "../../include/pins.h"

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

uint8_t BatteryScreen::readBatteryPercent() const {
    int raw = analogRead(PIN_BATTERY);
    if (_maxRaw <= _minRaw) return 0;

    long pct = ((long)(raw - _minRaw) * 100L) / (long)(_maxRaw - _minRaw);
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    return (uint8_t)pct;
}

bool BatteryScreen::update(DisplayManager& display, unsigned long /*now*/) {
    const uint8_t pct = readBatteryPercent();
    const bool isLow = pct <= _lowThreshold;

    display.clear();

    // Fixed LaMetric icon 389 palette (8x8). -1 means transparent.
    static const int32_t ICON389[64] = {
        -1,-1,0xD6BA,0xDEFB,0xD6BA,-1,-1,-1,
        -1,0xCE79,0x0A20,0x0A20,0x0A20,0xCE79,-1,-1,
        -1,0xCE79,0x1400,0x1400,0x1400,0xCE79,-1,-1,
        -1,0xBDD6,0x1CC1,0x1CC1,0x1CC1,0xBDD6,-1,-1,
        -1,0xBDD6,0x2542,0x2542,0x2542,0xBDD6,-1,-1,
        -1,0xBDD6,0x2542,0x2542,0x2542,0xBDD6,-1,-1,
        -1,0xBDD6,0x2602,0x2602,0x2602,0xAD95,-1,-1,
        -1,0xAD74,0xAD74,0xAD74,0xAD74,0xAD74,-1,-1,
    };

    const int ix = 0;
    const int iy = 0;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int32_t c = ICON389[y * 8 + x];
            if (c < 0) continue;

            uint16_t color = (uint16_t)c;
            // Optional low-battery tint on green fill portions only.
            if (isLow && (color == 0x0A20 || color == 0x1400 || color == 0x1CC1 || color == 0x2542 || color == 0x2602)) {
                color = _lowColor;
            }
            display.drawPixel(ix + x, iy + y, color);
        }
    }

    String txt = String((int)pct) + "%";
    display.drawSmallText(10, 1, txt, _textColor);

    return true;
}

void BatteryScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["minRaw"].is<int>()) _minRaw = (uint16_t)cfg["minRaw"].as<int>();
    if (cfg["maxRaw"].is<int>()) _maxRaw = (uint16_t)cfg["maxRaw"].as<int>();
    if (cfg["lowThreshold"].is<int>()) {
        int t = cfg["lowThreshold"].as<int>();
        if (t < 0) t = 0;
        if (t > 100) t = 100;
        _lowThreshold = (uint8_t)t;
    }

    if (cfg["textColor"].is<const char*>()) _textColor = DisplayManager::hexToColor(cfg["textColor"].as<String>());
    if (cfg["lowColor"].is<const char*>()) _lowColor = DisplayManager::hexToColor(cfg["lowColor"].as<String>());
}

void BatteryScreen::serialize(JsonObject& out) const {
    out["minRaw"] = _minRaw;
    out["maxRaw"] = _maxRaw;
    out["lowThreshold"] = _lowThreshold;
    out["textColor"] = color565ToHex(_textColor);
    out["lowColor"] = color565ToHex(_lowColor);
    out["iconId"] = 389; // fixed icon
}
