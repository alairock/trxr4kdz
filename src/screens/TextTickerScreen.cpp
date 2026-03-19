#include "TextTickerScreen.h"
#include "../DisplayManager.h"
#include <FastLED.h>
#include <string.h>

void TextTickerScreen::parseLinesFromText(const String& text) {
    _lineCount = 0;
    int start = 0;
    while (start <= (int)text.length() && _lineCount < MAX_LINES) {
        int nl = text.indexOf('\n', start);
        if (nl < 0) nl = text.length();
        _lines[_lineCount++] = text.substring(start, nl);
        start = nl + 1;
    }

    if (_lineCount == 0) {
        _lineCount = 1;
        _lines[0] = "";
    }
    if (_lineIndex >= _lineCount) _lineIndex = 0;
}

const String& TextTickerScreen::currentLine() const {
    return _lines[_lineIndex];
}

void TextTickerScreen::advanceLine() {
    if (_lineCount <= 1) return;
    _lineIndex = (_lineIndex + 1) % _lineCount;
}

void TextTickerScreen::ensureTextWidth() {
    _textPixelWidth = currentLine().length() * DisplayManager::fontCharWidth(1); // TomThumb width
}

TextTickerScreen::Direction TextTickerScreen::directionFromString(const String& s) const {
    String m = s;
    m.toLowerCase();
    if (m == "right") return Direction::RIGHT;
    return Direction::LEFT;
}

const char* TextTickerScreen::directionToString(Direction d) const {
    return d == Direction::RIGHT ? "right" : "left";
}

TextTickerScreen::EffectMode TextTickerScreen::effectFromString(const String& s) const {
    String m = s;
    m.toLowerCase();
    if (m == "blink") return EffectMode::BLINK;
    if (m == "rainbow") return EffectMode::RAINBOW;
    if (m == "rainbow_blink" || m == "blink_rainbow") return EffectMode::RAINBOW_BLINK;
    return EffectMode::NONE;
}

const char* TextTickerScreen::effectToString(EffectMode e) const {
    switch (e) {
        case EffectMode::BLINK: return "blink";
        case EffectMode::RAINBOW: return "rainbow";
        case EffectMode::RAINBOW_BLINK: return "rainbow_blink";
        default: return "none";
    }
}

void TextTickerScreen::clearIconFrames() {
    _iconFrameCount = 0;
    memset(_iconFrames, 0, sizeof(_iconFrames));
    memset(_iconFrameDelayMs, 0, sizeof(_iconFrameDelayMs));
}

void TextTickerScreen::drawIcon(DisplayManager& display, unsigned long now) {
    // Keep icon animation phase stable per screen activation so it does not jump,
    // and so we can render animated frames when payload frames exist.
    int16_t iconX = (int16_t)_scrollX;

    if (_iconFrameCount > 0) {
        uint32_t total = 0;
        for (uint8_t i = 0; i < _iconFrameCount; i++) {
            total += (_iconFrameDelayMs[i] > 0 ? _iconFrameDelayMs[i] : 120);
        }
        if (total == 0) total = 1;

        uint32_t t = (now - _activatedAt) % total;
        uint8_t frame = 0;
        uint32_t acc = 0;
        for (uint8_t i = 0; i < _iconFrameCount; i++) {
            acc += (_iconFrameDelayMs[i] > 0 ? _iconFrameDelayMs[i] : 120);
            if (t < acc) { frame = i; break; }
        }

        const uint16_t* px = _iconFrames[frame];
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                uint16_t c = px[y * 8 + x];
                if (c != 0x0000) display.drawPixel(iconX + x, y, c);
            }
        }
        return;
    }

    _icon.x = iconX;
    _icon.y = 0;
    _icon.draw(display, now);
}

uint16_t TextTickerScreen::colorForChar(unsigned long now, uint8_t charIndex) const {
    bool blinkOn = true;
    if ((_effect == EffectMode::BLINK || _effect == EffectMode::RAINBOW_BLINK) && _blinkMs > 0) {
        blinkOn = ((now / _blinkMs) % 2) == 0;
    }
    if (!blinkOn) return 0x0000;

    if (_effect == EffectMode::RAINBOW || _effect == EffectMode::RAINBOW_BLINK) {
        uint16_t speed = _rainbowSpeed == 0 ? 1 : _rainbowSpeed;
        uint8_t hue = (uint8_t)((now / speed) + (charIndex * _rainbowStep));
        CRGB c = CHSV(hue, 255, 255);
        return DisplayManager::rgb565(c.r, c.g, c.b);
    }

    return _color;
}

void TextTickerScreen::resetScroll(unsigned long now) {
    const int16_t iconPad = _hasIcon ? 1 : 0;
    const int16_t iconWidth = _hasIcon ? 8 : 0;
    const int16_t contentWidth = _textPixelWidth + iconWidth + iconPad;

    // If speed is zero, pin strip to the left edge.
    if (_speed <= 0.0f) {
        _scrollX = 0;
        _lastUpdate = now;
        _pausedUntil = now;
        return;
    }

    // Scroll a single "content strip" (icon + 1px pad + text) across the matrix.
    _scrollX = (_direction == Direction::LEFT) ? MATRIX_WIDTH : -contentWidth;
    _lastUpdate = now;
    _pausedUntil = now + _startDelayMs;
}

void TextTickerScreen::activate() {
    _activatedAt = millis();
    _lastUpdate = _activatedAt;
    if (!_customLines) parseLinesFromText(_text);
    if (_lineIndex >= _lineCount) _lineIndex = 0;
    ensureTextWidth();
    resetScroll(_activatedAt);
}

bool TextTickerScreen::update(DisplayManager& display, unsigned long now) {
    if (_lastUpdate == 0) _lastUpdate = now;

    const int16_t iconPad = _hasIcon ? 1 : 0;
    const int16_t iconWidth = _hasIcon ? 8 : 0;
    const int16_t contentWidth = _textPixelWidth + iconWidth + iconPad;
    const int16_t rightLimit = MATRIX_WIDTH;
    const int16_t leftLimit = -contentWidth;

    if (_speed <= 0.0f) {
        _scrollX = 0;
        _lastUpdate = now;
    } else if (now >= _pausedUntil) {
        float elapsed = (now - _lastUpdate) / 1000.0f;
        _lastUpdate = now;

        float delta = _speed * elapsed;
        if (_direction == Direction::LEFT) _scrollX -= delta;
        else _scrollX += delta;

        if (_bidirectional) {
            bool hitLeft = _scrollX <= leftLimit;
            bool hitRight = _scrollX >= rightLimit;
            if (hitLeft || hitRight) {
                _direction = (_direction == Direction::LEFT) ? Direction::RIGHT : Direction::LEFT;
                _pausedUntil = now + _pauseMs;
                _lastUpdate = now;
                advanceLine();
                ensureTextWidth();

                // Clamp to the edge using updated content width for the new line.
                if (hitLeft) {
                    _scrollX = -(_textPixelWidth + (_hasIcon ? 9 : 0));
                } else {
                    _scrollX = MATRIX_WIDTH;
                }
            }
        } else {
            if (_direction == Direction::LEFT && _scrollX < leftLimit) {
                advanceLine();
                ensureTextWidth();
                _scrollX = rightLimit;
                _pausedUntil = now + _pauseMs;
                _lastUpdate = now;
            } else if (_direction == Direction::RIGHT && _scrollX > rightLimit) {
                advanceLine();
                ensureTextWidth();
                _scrollX = -(_textPixelWidth + (_hasIcon ? 9 : 0));
                _pausedUntil = now + _pauseMs;
                _lastUpdate = now;
            }
        }
    } else {
        _lastUpdate = now;
    }

    display.clear();

    if (_hasIcon) {
        drawIcon(display, now);
    }

    const String& line = currentLine();
    int16_t x = (int16_t)_scrollX + (_hasIcon ? 9 : 0); // 8px icon + 1px margin
    const uint8_t charW = DisplayManager::fontCharWidth(1);
    for (uint8_t i = 0; i < line.length(); i++) {
        String ch(line[i]);
        display.drawFontText(x + (i * charW), 1, ch, colorForChar(now, i), 1);
    }

    return true;
}

void TextTickerScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["text"].is<const char*>()) {
        _text = cfg["text"].as<String>();
        _customLines = false;
        parseLinesFromText(_text);
    }

    if (cfg["lines"].is<JsonArrayConst>()) {
        JsonArrayConst arr = cfg["lines"].as<JsonArrayConst>();
        _lineCount = 0;
        bool hasNonEmpty = false;
        for (JsonVariantConst v : arr) {
            if (_lineCount >= MAX_LINES) break;
            if (!v.is<const char*>()) continue;
            String line = v.as<String>();
            if (line.length() > 0) hasNonEmpty = true;
            _lines[_lineCount++] = line;
        }

        if (_lineCount == 0 || !hasNonEmpty) {
            _customLines = false;
            parseLinesFromText(_text);
        } else {
            _customLines = true;
        }
        _lineIndex = 0;
    }

    if (cfg["color"].is<const char*>()) {
        _colorHex = cfg["color"].as<String>();
        _color = DisplayManager::hexToColor(_colorHex);
    }
    if (cfg["speed"].is<float>() || cfg["speed"].is<int>()) _speed = cfg["speed"];

    if (cfg["iconId"].is<int>()) {
        _iconId = cfg["iconId"];
        _icon.iconId = _iconId;
        _hasIcon = true;
    }
    if (cfg["hasIcon"].is<bool>()) _hasIcon = cfg["hasIcon"];

    if (cfg["icon"].is<JsonObjectConst>()) {
        JsonObjectConst icon = cfg["icon"].as<JsonObjectConst>();
        if (icon["id"].is<int>()) {
            _iconId = icon["id"].as<int>();
            _icon.iconId = _iconId;
        }

        if (icon["frames"].is<JsonArrayConst>()) {
            clearIconFrames();
            JsonArrayConst frames = icon["frames"].as<JsonArrayConst>();
            uint8_t fi = 0;
            for (JsonVariantConst fv : frames) {
                if (fi >= MAX_ICON_FRAMES) break;
                if (!fv.is<JsonObjectConst>()) continue;
                JsonObjectConst fo = fv.as<JsonObjectConst>();

                uint16_t delayMs = 120;
                if (fo["delayMs"].is<int>()) delayMs = fo["delayMs"].as<int>();
                _iconFrameDelayMs[fi] = delayMs;

                if (fo["pixels"].is<JsonArrayConst>()) {
                    JsonArrayConst pix = fo["pixels"].as<JsonArrayConst>();
                    uint8_t pi = 0;
                    for (JsonVariantConst pv : pix) {
                        if (pi >= 64) break;
                        if (pv.is<const char*>()) {
                            String hx = pv.as<String>();
                            _iconFrames[fi][pi] = (hx == "#000000") ? 0x0000 : DisplayManager::hexToColor(hx);
                        } else {
                            _iconFrames[fi][pi] = 0x0000;
                        }
                        pi++;
                    }
                    while (pi < 64) _iconFrames[fi][pi++] = 0x0000;
                    fi++;
                }
            }
            _iconFrameCount = fi;
            if (_iconFrameCount > 0) _hasIcon = true;
        }
    }

    if (cfg["direction"].is<const char*>()) _direction = directionFromString(cfg["direction"].as<String>());
    if (cfg["bidirectional"].is<bool>()) _bidirectional = cfg["bidirectional"];
    if (cfg["effect"].is<const char*>()) _effect = effectFromString(cfg["effect"].as<String>());

    if (cfg["pauseMs"].is<int>()) _pauseMs = cfg["pauseMs"];
    if (cfg["startDelayMs"].is<int>()) _startDelayMs = cfg["startDelayMs"];
    if (cfg["blinkMs"].is<int>()) _blinkMs = cfg["blinkMs"];
    if (cfg["rainbowStep"].is<int>()) _rainbowStep = cfg["rainbowStep"];
    if (cfg["rainbowSpeed"].is<int>()) _rainbowSpeed = cfg["rainbowSpeed"];

    ensureTextWidth();
}

void TextTickerScreen::serialize(JsonObject& out) const {
    out["text"] = _text;
    JsonArray lines = out.createNestedArray("lines");
    if (_customLines) {
        for (uint8_t i = 0; i < _lineCount; i++) lines.add(_lines[i]);
    }

    out["color"] = _colorHex;
    out["speed"] = _speed;
    out["iconId"] = _iconId;
    out["hasIcon"] = _hasIcon;

    if (_iconFrameCount > 0) {
        JsonObject icon = out["icon"].to<JsonObject>();
        icon["id"] = _iconId;
        JsonArray frames = icon["frames"].to<JsonArray>();
        for (uint8_t fi = 0; fi < _iconFrameCount; fi++) {
            JsonObject fo = frames.add<JsonObject>();
            fo["delayMs"] = _iconFrameDelayMs[fi] > 0 ? _iconFrameDelayMs[fi] : 120;
            JsonArray pix = fo["pixels"].to<JsonArray>();
            for (uint8_t pi = 0; pi < 64; pi++) {
                uint16_t c = _iconFrames[fi][pi];
                if (c == 0x0000) {
                    pix.add("#000000");
                } else {
                    uint8_t r5 = (c >> 11) & 0x1F;
                    uint8_t g6 = (c >> 5) & 0x3F;
                    uint8_t b5 = c & 0x1F;
                    uint8_t r = (r5 << 3) | (r5 >> 2);
                    uint8_t g = (g6 << 2) | (g6 >> 4);
                    uint8_t b = (b5 << 3) | (b5 >> 2);
                    char hx[8];
                    snprintf(hx, sizeof(hx), "#%02X%02X%02X", r, g, b);
                    pix.add(hx);
                }
            }
        }
    }

    out["direction"] = directionToString(_direction);
    out["bidirectional"] = _bidirectional;
    out["effect"] = effectToString(_effect);
    out["pauseMs"] = _pauseMs;
    out["startDelayMs"] = _startDelayMs;
    out["blinkMs"] = _blinkMs;
    out["rainbowStep"] = _rainbowStep;
    out["rainbowSpeed"] = _rainbowSpeed;
}
