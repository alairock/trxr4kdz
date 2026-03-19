#include "TimeWidget.h"
#include "../DisplayManager.h"
#include <time.h>

void TimeWidget::draw(DisplayManager& display, unsigned long now) {
    struct tm timeinfo;
    time_t t = time(nullptr);
    localtime_r(&t, &timeinfo);

    int hour = timeinfo.tm_hour;
    if (!use24h) {
        hour = hour % 12;
        if (hour == 0) hour = 12;
    }

    char buf[12];
    if (showSeconds) {
        snprintf(buf, sizeof(buf), "%d:%02d:%02d", hour, timeinfo.tm_min, timeinfo.tm_sec);
    } else {
        // Blink colon every second
        bool colonOn = (timeinfo.tm_sec % 2 == 0);
        if (colonOn) {
            snprintf(buf, sizeof(buf), "%d:%02d", hour, timeinfo.tm_min);
        } else {
            snprintf(buf, sizeof(buf), "%d %02d", hour, timeinfo.tm_min);
        }
    }

    int16_t drawX = x;
    if (autoCenter) {
        uint8_t charW = DisplayManager::fontCharWidth(fontId);
        int16_t textLen = strlen(buf);
        int16_t colonCount = 0;
        for (int i = 0; buf[i]; i++) {
            if (buf[i] == ':' || buf[i] == ' ') colonCount++;
        }
        // Narrow chars (colon/space) are roughly half width
        int16_t textWidth = (textLen - colonCount) * charW + colonCount * (charW / 2);
        drawX = x + (displayWidth - textWidth) / 2 + 1; // slight right bias for visual centering on 32px matrix
    }
    display.drawFontText(drawX, y, buf, color, fontId);
}

void TimeWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["use24h"].is<bool>()) use24h = cfg["use24h"];
    if (cfg["showSeconds"].is<bool>()) showSeconds = cfg["showSeconds"];
    if (cfg["color"].is<const char*>()) color = DisplayManager::hexToColor(cfg["color"].as<String>());
}

void TimeWidget::serialize(JsonObject& out) const {
    out["use24h"] = use24h;
    out["showSeconds"] = showSeconds;
}
