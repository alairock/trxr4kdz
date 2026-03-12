#pragma once

#include "Screen.h"
#include "../widgets/IconWidget.h"

class TextTickerScreen : public Screen {
public:
    void activate() override;
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::TEXT_TICKER; }
    const char* typeName() const override { return "text_ticker"; }

private:
    String _text = "Hello World!";
    uint16_t _color = 0xFFFF;
    float _speed = 30.0f;    // pixels per second
    uint16_t _iconId = 0;
    bool _hasIcon = false;

    float _scrollX = 0;
    unsigned long _lastUpdate = 0;
    int16_t _textPixelWidth = 0;

    IconWidget _icon;
};
