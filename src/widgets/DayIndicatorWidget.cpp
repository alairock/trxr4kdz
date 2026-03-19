#include "DayIndicatorWidget.h"
#include "../DisplayManager.h"
#include <time.h>

void DayIndicatorWidget::draw(DisplayManager& display, unsigned long now) {
    struct tm timeinfo;
    time_t t = time(nullptr);
    localtime_r(&t, &timeinfo);

    // tm_wday: 0=Sunday
    int dayOfWeek = timeinfo.tm_wday;
    // Render either Mon..Sun or Sun..Sat depending on config
    int adjusted = weekStartsMonday ? ((dayOfWeek + 6) % 7) : dayOfWeek;

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

    if (cfg["weekStart"].is<const char*>()) {
        String s = cfg["weekStart"].as<String>();
        s.toLowerCase();
        if (s == "monday" || s == "mon") weekStartsMonday = true;
        else if (s == "sunday" || s == "sun") weekStartsMonday = false;
    }
}

void DayIndicatorWidget::serialize(JsonObject& out) const {
    out["weekStart"] = weekStartsMonday ? "monday" : "sunday";
    // colors are serialized by the parent screen
}
