#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>

class DisplayManager;

enum class ScreenType : uint8_t {
    CLOCK,
    BINARY_CLOCK,
    WEATHER,
    CRYPTO,
    TEXT_TICKER,
    CANVAS,
    CUSTOM
};

class Screen {
public:
    virtual ~Screen() = default;
    virtual void activate() {}
    virtual void deactivate() {}
    virtual bool update(DisplayManager& display, unsigned long now) = 0;
    virtual void configure(const JsonObjectConst& cfg) = 0;
    virtual void serialize(JsonObject& out) const = 0;
    virtual ScreenType type() const = 0;
    virtual const char* typeName() const = 0;

    String id;
    bool enabled = true;
    uint16_t durationSec = 10;
    uint8_t order = 0;
};
