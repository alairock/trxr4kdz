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
    enum class Direction : uint8_t { LEFT, RIGHT };
    enum class EffectMode : uint8_t { NONE, BLINK, RAINBOW, RAINBOW_BLINK };

    static const uint8_t MAX_LINES = 8;

    void parseLinesFromText(const String& text);
    void ensureTextWidth();
    const String& currentLine() const;
    void advanceLine();
    uint16_t colorForChar(unsigned long now, uint8_t charIndex) const;
    void resetScroll(unsigned long now);
    Direction directionFromString(const String& s) const;
    const char* directionToString(Direction d) const;
    EffectMode effectFromString(const String& s) const;
    const char* effectToString(EffectMode e) const;
    void drawIcon(DisplayManager& display, unsigned long now);
    void clearIconFrames();

    String _text = "Hello World!";
    String _lines[MAX_LINES];
    uint8_t _lineCount = 1;
    uint8_t _lineIndex = 0;
    bool _customLines = false;

    uint16_t _color = 0xFFFF;
    String _colorHex = "#FFFFFF";
    float _speed = 20.0f;    // pixels per second (readability-first default)
    uint16_t _iconId = 0;
    bool _hasIcon = false;

    static const uint8_t MAX_ICON_FRAMES = 16;
    uint16_t _iconFrames[MAX_ICON_FRAMES][64] = {};
    uint16_t _iconFrameDelayMs[MAX_ICON_FRAMES] = {};
    uint8_t _iconFrameCount = 0;

    Direction _direction = Direction::LEFT;
    bool _bidirectional = false;
    EffectMode _effect = EffectMode::NONE;

    uint16_t _pauseMs = 600;      // hold at edge/loop before movement resumes
    uint16_t _startDelayMs = 300; // delay before first movement on activate
    uint16_t _blinkMs = 500;      // blink cycle (on/off)
    uint16_t _rainbowStep = 18;   // per-character hue spacing
    uint16_t _rainbowSpeed = 20;  // lower is faster (ms per hue step)

    float _scrollX = 0;
    unsigned long _lastUpdate = 0;
    unsigned long _pausedUntil = 0;
    unsigned long _activatedAt = 0;
    int16_t _textPixelWidth = 0;

    IconWidget _icon;
};
