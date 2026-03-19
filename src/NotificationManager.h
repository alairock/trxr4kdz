#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "DisplayManager.h"

class NotificationManager {
public:
    bool begin();
    bool save();

    void render(DisplayManager& display, bool wifiDisconnected, bool mqttProblem, unsigned long now);

    void serialize(JsonDocument& doc) const;
    bool configure(const JsonObjectConst& obj);

    bool setIndicatorState(const String& id, bool on, bool persist = true);
    bool previewIndicator(const String& id, unsigned long durationMs = 1500);

private:
    struct IndicatorCfg {
        String color = "#00FF00";
        uint8_t style = 3; // corners: 1 or 3, edge: forced 2
        bool state = false;
        unsigned long previewUntil = 0;
    };

    IndicatorCfg _rightTop;
    IndicatorCfg _rightMid;
    IndicatorCfg _rightBottom;

    void drawCorner(DisplayManager& d, bool top, bool right, const IndicatorCfg& cfg) const;
    void drawRightEdgeMid(DisplayManager& d, const IndicatorCfg& cfg) const;
    void drawWifi(DisplayManager& d, bool on) const;
    void drawMqtt(DisplayManager& d, bool on) const;
    uint16_t parseColor(const String& s) const;
};
