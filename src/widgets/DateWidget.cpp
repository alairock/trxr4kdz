#include "DateWidget.h"
#include "../DisplayManager.h"
#include <time.h>

static const char* MONTHS[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

void DateWidget::draw(DisplayManager& display, unsigned long now) {
    struct tm timeinfo;
    time_t t = time(nullptr);
    localtime_r(&t, &timeinfo);

    char buf[8];
    if (shortMonth) {
        snprintf(buf, sizeof(buf), "%s%d", MONTHS[timeinfo.tm_mon], timeinfo.tm_mday);
    } else {
        snprintf(buf, sizeof(buf), "%d/%d", timeinfo.tm_mon + 1, timeinfo.tm_mday);
    }

    display.drawText(x, y, buf, color);
}

void DateWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["color"].is<const char*>()) color = DisplayManager::hexToColor(cfg["color"].as<String>());
    if (cfg["shortMonth"].is<bool>()) shortMonth = cfg["shortMonth"];
}

void DateWidget::serialize(JsonObject& out) const {
    out["shortMonth"] = shortMonth;
}
