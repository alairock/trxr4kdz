#include "OvertakeManager.h"

void OvertakeManager::begin(BuzzerManager& buzzer, ScreenManager& screens) {
    _buzzer = &buzzer;
    _screens = &screens;
    memset(_framebuffer, 0, sizeof(_framebuffer));
}

uint16_t OvertakeManager::toneFreq(const String& tone, uint8_t step) const {
    if (tone == "chime") {
        static const uint16_t seq[] = {784, 988, 1175, 988};
        return seq[step % 4];
    }
    if (tone == "pulse") {
        static const uint16_t seq[] = {880, 660};
        return seq[step % 2];
    }
    return 1000;
}

void OvertakeManager::trigger(int32_t durationSec, const String& soundMode, const String& tone, uint8_t volume) {
    _active = true;
    _infinite = (durationSec < 0);
    _endAt = _infinite ? 0 : (millis() + (unsigned long)durationSec * 1000UL);

    _soundMode = soundMode;
    _tone = tone;
    _volume = volume;
    _singlePlayed = false;
    _soundStopped = false;
    _nextToneAt = 0;
    _toneStep = 0;
}

void OvertakeManager::setPixel(int16_t x, int16_t y, uint16_t color) {
    if (x < 0 || x >= MATRIX_WIDTH || y < 0 || y >= MATRIX_HEIGHT) return;
    _framebuffer[y * MATRIX_WIDTH + x] = color;
}

void OvertakeManager::pushPixels(const uint8_t* rgb, size_t len) {
    if (!rgb || len == 0) return;
    size_t maxPix = min((size_t)(MATRIX_WIDTH * MATRIX_HEIGHT), len / 3);
    for (size_t i = 0; i < maxPix; i++) {
        uint8_t r = rgb[i * 3 + 0];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];
        _framebuffer[i] = DisplayManager::rgb565(r, g, b);
    }
}

void OvertakeManager::pushDrawCommands(const JsonObjectConst& cmd) {
    if (cmd["cl"].is<bool>() && cmd["cl"].as<bool>()) {
        memset(_framebuffer, 0, sizeof(_framebuffer));
    }

    if (cmd["df"].is<JsonArrayConst>()) {
        for (JsonVariantConst v : cmd["df"].as<JsonArrayConst>()) {
            if (!v.is<JsonArrayConst>()) continue;
            JsonArrayConst a = v.as<JsonArrayConst>();
            if (a.size() < 5) continue;
            int x = a[0] | 0, y = a[1] | 0, w = a[2] | 0, h = a[3] | 0;
            uint16_t c = DisplayManager::hexToColor(String((const char*)a[4]));
            for (int yy = y; yy < y + h; yy++) for (int xx = x; xx < x + w; xx++) setPixel(xx, yy, c);
        }
    }

    if (cmd["dl"].is<JsonArrayConst>()) {
        for (JsonVariantConst v : cmd["dl"].as<JsonArrayConst>()) {
            if (!v.is<JsonArrayConst>()) continue;
            JsonArrayConst a = v.as<JsonArrayConst>();
            if (a.size() < 5) continue;
            int x0 = a[0] | 0, y0 = a[1] | 0, x1 = a[2] | 0, y1 = a[3] | 0;
            uint16_t c = DisplayManager::hexToColor(String((const char*)a[4]));
            int dx = abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
            int dy = -abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
            int err = dx + dy;
            while (true) {
                setPixel(x0, y0, c);
                if (x0 == x1 && y0 == y1) break;
                int e2 = 2 * err;
                if (e2 >= dy) { err += dy; x0 += sx; }
                if (e2 <= dx) { err += dx; y0 += sy; }
            }
        }
    }

    if (cmd["dp"].is<JsonArrayConst>()) {
        for (JsonVariantConst v : cmd["dp"].as<JsonArrayConst>()) {
            if (!v.is<JsonArrayConst>()) continue;
            JsonArrayConst a = v.as<JsonArrayConst>();
            if (a.size() < 3) continue;
            int x = a[0] | 0, y = a[1] | 0;
            uint16_t c = DisplayManager::hexToColor(String((const char*)a[2]));
            setPixel(x, y, c);
        }
    }
}

void OvertakeManager::clear() {
    if (_buzzer) _buzzer->stop();
    _active = false;
    _singlePlayed = false;
    _soundStopped = false;
    _nextToneAt = 0;
    memset(_framebuffer, 0, sizeof(_framebuffer));
}

void OvertakeManager::stopSound() {
    _soundStopped = true;
    if (_buzzer) _buzzer->stop();
}

bool OvertakeManager::isActive() const {
    return _active;
}

void OvertakeManager::update() {
    if (!_active) return;

    unsigned long now = millis();
    if (!_infinite && now >= _endAt) {
        clear();
        return;
    }

    if (!_buzzer || _soundStopped) return;

    if (_soundMode == "single") {
        if (!_singlePlayed) {
            _buzzer->beep(toneFreq(_tone, 0), 180, _volume);
            _singlePlayed = true;
        }
    } else if (_soundMode == "repeat") {
        if (now >= _nextToneAt) {
            _buzzer->beep(toneFreq(_tone, _toneStep++), 180, _volume);
            _nextToneAt = now + 280;
        }
    }
}

void OvertakeManager::render(DisplayManager& display) {
    if (!_active) return;

    display.clear();
    for (int y = 0; y < MATRIX_HEIGHT; y++) {
        for (int x = 0; x < MATRIX_WIDTH; x++) {
            uint16_t c = _framebuffer[y * MATRIX_WIDTH + x];
            if (c != 0) display.drawPixel(x, y, c);
        }
    }
}
