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

    if (_showIcon) {
        _icon.x = 0;
        _icon.y = 0;
        textStartX = 9; // 8px icon + 1px gap
        displayWidth -= 9;
    }

    // Center time on the available width
    // Estimate pixel width: each char ~6px, colon ~4px
    // "H:MM" ~22px, "HH:MM" ~28px, "H MM" same
    int16_t estTimeWidth = _use24h ? 28 : 25; // rough estimate
    int16_t timeX = textStartX + (displayWidth - estTimeWidth) / 2;
    if (timeX < textStartX) timeX = textStartX;

    _time.x = timeX;
    _time.y = timeY;
    _time.use24h = _use24h;

    if (_showDate) {
        // Date only works well with icon (icon left, time center-ish, date right)
        // or without time. On 32px, time+date overlap. Place date where we can.
        _date.x = textStartX;
        _date.y = timeY;
    }

    if (_showDayIndicator) {
        // Center 7 dots on available width
        int16_t dotsX = textStartX + (displayWidth - 7) / 2;
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

    if (_showIcon) _icon.draw(display, now);
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

        JsonArrayConst comps = cfg["complications"];
        for (JsonVariantConst v : comps) {
            const char* c = v.as<const char*>();
            if (!c) continue;
            if (strcmp(c, "time") == 0) _showTime = true;
            else if (strcmp(c, "date") == 0) _showDate = true;
            else if (strcmp(c, "icon") == 0) _showIcon = true;
            else if (strcmp(c, "progress_bar") == 0) _showProgress = true;
            else if (strcmp(c, "day_indicator") == 0) _showDayIndicator = true;
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
    }

    _time.use24h = _use24h;
    applyTimezone();
    layoutWidgets();
}

void ClockScreen::serialize(JsonObject& out) const {
    out["use24h"] = _use24h;
    out["timezone"] = _timezone;

    JsonArray comps = out["complications"].to<JsonArray>();
    if (_showTime) comps.add("time");
    if (_showDate) comps.add("date");
    if (_showIcon) comps.add("icon");
    if (_showProgress) comps.add("progress_bar");
    if (_showDayIndicator) comps.add("day_indicator");

    JsonObject colors = out["colors"].to<JsonObject>();
    (void)colors; // colors are applied via configure(), preserved in memory
}
