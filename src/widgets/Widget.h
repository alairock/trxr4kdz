#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class DisplayManager;

class Widget {
public:
    virtual ~Widget() = default;
    virtual void draw(DisplayManager& display, unsigned long now) = 0;
    virtual void configure(const JsonObjectConst& cfg) = 0;
    virtual void serialize(JsonObject& out) const = 0;
    int16_t x = 0, y = 0;
};
