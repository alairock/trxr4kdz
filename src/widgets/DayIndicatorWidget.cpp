#include "DayIndicatorWidget.h"
#include "../DisplayManager.h"
#include <time.h>

void DayIndicatorWidget::draw(DisplayManager& display, unsigned long now) {
    struct tm timeinfo;
    time_t t = time(nullptr);
    localtime_r(&t, &timeinfo);

    // tm_wday: 0=Sunday. Draw 7 pixels for Mon-Sun
    int dayOfWeek = timeinfo.tm_wday;
    // Convert to Mon=0..Sun=6
    int adjusted = (dayOfWeek + 6) % 7;

    // 2 pixels per day, 1 pixel gap between days = 20px total
    for (int i = 0; i < 7; i++) {
        uint16_t c = (i == adjusted) ? activeColor : inactiveColor;
        int16_t px = x + i * 3; // 2px dot + 1px gap = stride of 3
        display.drawPixel(px, y, c);
        display.drawPixel(px + 1, y, c);
    }
}

void DayIndicatorWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["activeColor"].is<const char*>()) activeColor = DisplayManager::hexToColor(cfg["activeColor"].as<String>());
    if (cfg["inactiveColor"].is<const char*>()) inactiveColor = DisplayManager::hexToColor(cfg["inactiveColor"].as<String>());
}

void DayIndicatorWidget::serialize(JsonObject& out) const {
    // colors are serialized by the parent screen
}
