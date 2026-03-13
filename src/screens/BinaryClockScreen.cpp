#include "BinaryClockScreen.h"
#include "../DisplayManager.h"
#include <time.h>

bool BinaryClockScreen::update(DisplayManager& display, unsigned long now) {
    (void)now;
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

    // AM/PM dot at top-left (requested): AM dim purple, PM red
    uint16_t ap = isPM ? _pmColor : _offColor;
    display.drawPixel(0, 0, ap);
    display.drawPixel(1, 0, ap);
    display.drawPixel(0, 1, ap);
    display.drawPixel(1, 1, ap);

    return true;
}

void BinaryClockScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["use24h"].is<bool>()) _use24h = cfg["use24h"].as<bool>();
    if (cfg["onColor"].is<const char*>()) _onColor = DisplayManager::hexToColor(cfg["onColor"].as<String>());
    if (cfg["offColor"].is<const char*>()) _offColor = DisplayManager::hexToColor(cfg["offColor"].as<String>());
    if (cfg["pmColor"].is<const char*>()) _pmColor = DisplayManager::hexToColor(cfg["pmColor"].as<String>());
}

void BinaryClockScreen::serialize(JsonObject& out) const {
    out["use24h"] = _use24h;
}
