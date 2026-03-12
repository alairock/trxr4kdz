#include "TextTickerScreen.h"
#include "../DisplayManager.h"

void TextTickerScreen::activate() {
    // Reset scroll position to start from right edge
    _scrollX = MATRIX_WIDTH;
    _lastUpdate = millis();
    // Estimate text width: 6 pixels per character (5px + 1px spacing) for default font
    _textPixelWidth = _text.length() * 6;
}

bool TextTickerScreen::update(DisplayManager& display, unsigned long now) {
    // Calculate elapsed time and advance scroll
    if (_lastUpdate == 0) _lastUpdate = now;
    float elapsed = (now - _lastUpdate) / 1000.0f;
    _lastUpdate = now;

    _scrollX -= _speed * elapsed;

    // Wrap around when text fully exits left side
    if (_scrollX < -_textPixelWidth) {
        _scrollX = MATRIX_WIDTH;
    }

    display.clear();

    int16_t textAreaStart = 0;
    if (_hasIcon) {
        _icon.x = 0;
        _icon.y = 0;
        _icon.draw(display, now);
        textAreaStart = 9;
    }

    // Draw scrolling text
    display.drawText((int16_t)_scrollX + textAreaStart, 0, _text, _color);

    return true;
}

void TextTickerScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["text"].is<const char*>()) _text = cfg["text"].as<String>();
    if (cfg["color"].is<const char*>()) _color = DisplayManager::hexToColor(cfg["color"].as<String>());
    if (cfg["speed"].is<float>() || cfg["speed"].is<int>()) _speed = cfg["speed"];
    if (cfg["iconId"].is<int>()) {
        _iconId = cfg["iconId"];
        _hasIcon = true;
        _icon.iconId = _iconId;
    }
    if (cfg["hasIcon"].is<bool>()) _hasIcon = cfg["hasIcon"];

    _textPixelWidth = _text.length() * 6;
}

void TextTickerScreen::serialize(JsonObject& out) const {
    out["text"] = _text;
    out["speed"] = _speed;
    out["iconId"] = _iconId;
    out["hasIcon"] = _hasIcon;
}
