#include "ProgressBarWidget.h"
#include "../DisplayManager.h"
#include <time.h>

void ProgressBarWidget::draw(DisplayManager& display, unsigned long now) {
    uint8_t pct = percent;
    if (autoProgress) {
        struct tm timeinfo;
        time_t t = time(nullptr);
        localtime_r(&t, &timeinfo);
        // Progress through the day (0-100%)
        uint32_t secOfDay = timeinfo.tm_hour * 3600 + timeinfo.tm_min * 60 + timeinfo.tm_sec;
        pct = (uint8_t)((secOfDay * 100UL) / 86400UL);
    }
    display.drawProgressBar(x, y, w, h, pct, fgColor, bgColor);
}

void ProgressBarWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["w"].is<int>()) w = cfg["w"];
    if (cfg["h"].is<int>()) h = cfg["h"];
    if (cfg["percent"].is<int>()) percent = cfg["percent"];
    if (cfg["autoProgress"].is<bool>()) autoProgress = cfg["autoProgress"];
    if (cfg["fgColor"].is<const char*>()) fgColor = DisplayManager::hexToColor(cfg["fgColor"].as<String>());
    if (cfg["bgColor"].is<const char*>()) bgColor = DisplayManager::hexToColor(cfg["bgColor"].as<String>());
}

void ProgressBarWidget::serialize(JsonObject& out) const {
    out["w"] = w;
    out["h"] = h;
    out["percent"] = percent;
    out["autoProgress"] = autoProgress;
}
