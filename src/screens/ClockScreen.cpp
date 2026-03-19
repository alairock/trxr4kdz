#include "ClockScreen.h"
#include "../DisplayManager.h"
#include <time.h>

void ClockScreen::applyTimezone() {
    setenv("TZ", _timezone.c_str(), 1);
    tzset();
}

void ClockScreen::layoutWidgets() {
    int16_t textStartX = 0;
    int16_t timeY = 0;
    int16_t displayWidth = MATRIX_WIDTH;

    if (_showCalendar) {
        _calendar.x = 0;
        _calendar.y = 0;
        textStartX = 9; // 8px calendar + 1px gap
        displayWidth -= 9;
    } else if (_showIcon) {
        _icon.x = 0;
        _icon.y = 0;
        textStartX = 9; // 8px icon + 1px gap
        displayWidth -= 9;
    }

    _time.x = textStartX;
    _time.y = timeY;
    _time.use24h = _use24h;
    _time.fontId = _fontId;
    _time.autoCenter = true;
    _time.displayWidth = displayWidth;

    if (_showDate) {
        // Date only works well with icon (icon left, time center-ish, date right)
        // or without time. On 32px, time+date overlap. Place date where we can.
        _date.x = textStartX;
        _date.y = timeY;
    }

    if (_showDayIndicator) {
        // 7 days × 2px + 6 gaps × 1px = 20px total
        int16_t dotsX = textStartX + (displayWidth - 20) / 2;
        _dayIndicator.x = dotsX;
        _dayIndicator.y = 7; // bottom row
    }

    if (_showProgress) {
        _progress.x = textStartX;
        _progress.y = 7;
        _progress.w = displayWidth;
        _progress.h = 1;
    }
}

bool ClockScreen::update(DisplayManager& display, unsigned long now) {
    display.clear();

    if (_showCalendar) _calendar.draw(display, now);
    else if (_showIcon) _icon.draw(display, now);
    if (_showTime) _time.draw(display, now);
    if (_showDate && !_showTime) _date.draw(display, now); // only if time is off
    if (_showDayIndicator) _dayIndicator.draw(display, now);
    if (_showProgress) _progress.draw(display, now);

    return true;
}

void ClockScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["use24h"].is<bool>()) _use24h = cfg["use24h"];
    if (cfg["timezone"].is<const char*>()) _timezone = cfg["timezone"].as<String>();

    // Parse complications array
    if (cfg["complications"].is<JsonArrayConst>()) {
        _showTime = false;
        _showDate = false;
        _showIcon = false;
        _showProgress = false;
        _showDayIndicator = false;
        _showCalendar = false;

        JsonArrayConst comps = cfg["complications"];
        for (JsonVariantConst v : comps) {
            const char* c = v.as<const char*>();
            if (!c) continue;
            if (strcmp(c, "time") == 0) _showTime = true;
            else if (strcmp(c, "date") == 0) _showDate = true;
            else if (strcmp(c, "icon") == 0) _showIcon = true;
            else if (strcmp(c, "progress_bar") == 0) _showProgress = true;
            else if (strcmp(c, "day_indicator") == 0) _showDayIndicator = true;
            else if (strcmp(c, "calendar") == 0) _showCalendar = true;
        }
    }

    // Configure sub-widget colors
    if (cfg["colors"].is<JsonObjectConst>()) {
        JsonObjectConst colors = cfg["colors"];
        if (colors["time"].is<const char*>())
            _time.color = DisplayManager::hexToColor(colors["time"].as<String>());
        if (colors["date"].is<const char*>())
            _date.color = DisplayManager::hexToColor(colors["date"].as<String>());
        if (colors["icon"].is<const char*>())
            _icon.color = DisplayManager::hexToColor(colors["icon"].as<String>());
        if (colors["calendar"].is<const char*>())
            _calendar.textColor = DisplayManager::hexToColor(colors["calendar"].as<String>());
    }

    if (cfg["font"].is<int>()) _fontId = cfg["font"].as<uint8_t>();

    if (cfg["weekStart"].is<const char*>()) {
        String s = cfg["weekStart"].as<String>();
        s.toLowerCase();
        if (s == "monday" || s == "mon") _dayIndicator.weekStartsMonday = true;
        else if (s == "sunday" || s == "sun") _dayIndicator.weekStartsMonday = false;
    }

    if (cfg["dayIndicatorActive"].is<const char*>()) {
        _dayIndicator.activeColor = DisplayManager::hexToColor(cfg["dayIndicatorActive"].as<String>());
    }
    if (cfg["dayIndicatorInactive"].is<const char*>()) {
        _dayIndicator.inactiveColor = DisplayManager::hexToColor(cfg["dayIndicatorInactive"].as<String>());
    }

    _time.use24h = _use24h;
    _time.fontId = _fontId;
    applyTimezone();
    layoutWidgets();
}

void ClockScreen::serialize(JsonObject& out) const {
    out["use24h"] = _use24h;
    out["timezone"] = _timezone;
    out["font"] = _fontId;
    out["weekStart"] = _dayIndicator.weekStartsMonday ? "monday" : "sunday";
    auto color565ToHexLocal = [](uint16_t c) {
        uint8_t r5 = (c >> 11) & 0x1F;
        uint8_t g6 = (c >> 5) & 0x3F;
        uint8_t b5 = c & 0x1F;
        uint8_t r = (r5 << 3) | (r5 >> 2);
        uint8_t g = (g6 << 2) | (g6 >> 4);
        uint8_t b = (b5 << 3) | (b5 >> 2);
        char outHex[8];
        snprintf(outHex, sizeof(outHex), "#%02X%02X%02X", r, g, b);
        return String(outHex);
    };
    out["dayIndicatorActive"] = color565ToHexLocal(_dayIndicator.activeColor);
    out["dayIndicatorInactive"] = color565ToHexLocal(_dayIndicator.inactiveColor);

    JsonArray comps = out["complications"].to<JsonArray>();
    if (_showTime) comps.add("time");
    if (_showDate) comps.add("date");
    if (_showIcon) comps.add("icon");
    if (_showProgress) comps.add("progress_bar");
    if (_showDayIndicator) comps.add("day_indicator");
    if (_showCalendar) comps.add("calendar");

    JsonObject colors = out["colors"].to<JsonObject>();
    (void)colors; // colors are applied via configure(), preserved in memory
}
