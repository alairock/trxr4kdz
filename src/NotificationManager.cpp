#include "NotificationManager.h"
#include <LittleFS.h>

static const char* kPath = "/notifications.json";

bool NotificationManager::begin() {
    File f = LittleFS.open(kPath, "r");
    if (!f) return true;

    JsonDocument doc;
    if (deserializeJson(doc, f)) {
        f.close();
        return false;
    }
    f.close();
    return configure(doc.as<JsonObjectConst>());
}

bool NotificationManager::save() {
    JsonDocument doc;
    serialize(doc);
    File f = LittleFS.open(kPath, "w");
    if (!f) return false;
    serializeJsonPretty(doc, f);
    f.close();
    return true;
}

uint16_t NotificationManager::parseColor(const String& s) const {
    return DisplayManager::hexToColor(s);
}

void NotificationManager::drawCorner(DisplayManager& d, bool top, bool right, const IndicatorCfg& cfg) const {
    if (!cfg.state && cfg.previewUntil == 0) return;

    const int x = right ? (MATRIX_WIDTH - 1) : 0;
    const int y = top ? 0 : (MATRIX_HEIGHT - 1);
    const int ax = right ? (MATRIX_WIDTH - 2) : 1;
    const int ay = top ? 1 : (MATRIX_HEIGHT - 2);

    const uint16_t c = parseColor(cfg.color);
    d.drawPixel(x, y, c);
    if (cfg.style == 3) {
        d.drawPixel(ax, y, c);
        d.drawPixel(x, ay, c);
    }
}

void NotificationManager::drawRightEdgeMid(DisplayManager& d, const IndicatorCfg& cfg) const {
    if (!cfg.state && cfg.previewUntil == 0) return;

    const uint16_t c = parseColor(cfg.color);
    d.drawPixel(MATRIX_WIDTH - 1, 3, c);
    d.drawPixel(MATRIX_WIDTH - 1, 4, c);
}

void NotificationManager::drawWifi(DisplayManager& d, bool on) const {
    if (!on) return;
    d.drawPixel(0, 0, DisplayManager::hexToColor("#FF0000"));
}

void NotificationManager::drawMqtt(DisplayManager& d, bool on) const {
    if (!on) return;
    d.drawPixel(0, MATRIX_HEIGHT - 1, DisplayManager::hexToColor("#FFD400"));
}

void NotificationManager::render(DisplayManager& display, bool wifiDisconnected, bool mqttProblem, unsigned long now) {
    const bool blinkOn = ((now / 350UL) % 2UL) == 0UL;

    if (now > _rightTop.previewUntil) _rightTop.previewUntil = 0;
    if (now > _rightMid.previewUntil) _rightMid.previewUntil = 0;
    if (now > _rightBottom.previewUntil) _rightBottom.previewUntil = 0;

    drawCorner(display, true, true, _rightTop);
    drawRightEdgeMid(display, _rightMid);
    drawCorner(display, false, true, _rightBottom);

    if (wifiDisconnected) drawWifi(display, blinkOn);
    if (mqttProblem) drawMqtt(display, blinkOn);
}

void NotificationManager::serialize(JsonDocument& doc) const {
    JsonObject rt = doc["rightTop"].to<JsonObject>();
    rt["enabled"] = true;
    rt["color"] = _rightTop.color;
    rt["style"] = _rightTop.style;
    rt["state"] = _rightTop.state;

    JsonObject rm = doc["rightMid"].to<JsonObject>();
    rm["enabled"] = true;
    rm["color"] = _rightMid.color;
    rm["style"] = 2;
    rm["state"] = _rightMid.state;

    JsonObject rb = doc["rightBottom"].to<JsonObject>();
    rb["enabled"] = true;
    rb["color"] = _rightBottom.color;
    rb["style"] = _rightBottom.style;
    rb["state"] = _rightBottom.state;

    JsonObject system = doc["system"].to<JsonObject>();
    system["wifiIndicator"] = "top-left red flashing when disconnected";
    system["mqttIndicator"] = "bottom-left yellow flashing when configured but disconnected";
}

bool NotificationManager::configure(const JsonObjectConst& obj) {
    auto apply = [&](IndicatorCfg& c, JsonObjectConst in, bool allowStyle) {
        if (!in) return;
        if (in["color"].is<const char*>()) c.color = in["color"].as<String>();
        if (in["state"].is<bool>()) c.state = in["state"].as<bool>();
        if (allowStyle && in["style"].is<int>()) {
            int s = in["style"].as<int>();
            c.style = (s == 1) ? 1 : 3;
        }
    };

    apply(_rightTop, obj["rightTop"].as<JsonObjectConst>(), true);
    apply(_rightMid, obj["rightMid"].as<JsonObjectConst>(), false);
    apply(_rightBottom, obj["rightBottom"].as<JsonObjectConst>(), true);
    return true;
}

bool NotificationManager::setIndicatorState(const String& id, bool on, bool persist) {
    if (id == "rightTop") _rightTop.state = on;
    else if (id == "rightMid") _rightMid.state = on;
    else if (id == "rightBottom") _rightBottom.state = on;
    else return false;

    if (persist) save();
    return true;
}

bool NotificationManager::previewIndicator(const String& id, unsigned long durationMs) {
    unsigned long until = millis() + durationMs;
    if (id == "rightTop") _rightTop.previewUntil = until;
    else if (id == "rightMid") _rightMid.previewUntil = until;
    else if (id == "rightBottom") _rightBottom.previewUntil = until;
    else if (id == "wifi" || id == "mqtt") return true;
    else return false;
    return true;
}
