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

    for (int i = 0; i < 7; i++) {
        uint16_t c = (i == adjusted) ? activeColor : inactiveColor;
        display.drawPixel(x + i, y, c);
    }
}

void DayIndicatorWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["activeColor"].is<const char*>()) activeColor = DisplayManager::hexToColor(cfg["activeColor"].as<String>());
    if (cfg["inactiveColor"].is<const char*>()) inactiveColor = DisplayManager::hexToColor(cfg["inactiveColor"].as<String>());
}

void DayIndicatorWidget::serialize(JsonObject& out) const {
    // colors are serialized by the parent screen
}
