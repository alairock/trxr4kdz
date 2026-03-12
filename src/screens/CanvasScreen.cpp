#include "CanvasScreen.h"
#include "../DisplayManager.h"

CanvasScreen::CanvasScreen() {
    clearFramebuffer();
    durationSec = 0; // Canvas screens don't auto-advance by default
}

void CanvasScreen::activate() {
    _lastPush = millis();
}

void CanvasScreen::clearFramebuffer() {
    memset(_framebuffer, 0, sizeof(_framebuffer));
    _dirty = true;
}

void CanvasScreen::pushPixels(const uint8_t* rgb, size_t len) {
    // Expects raw RGB888 data: 32*8*3 = 768 bytes
    size_t pixels = len / 3;
    if (pixels > 32 * 8) pixels = 32 * 8;

    for (size_t i = 0; i < pixels; i++) {
        uint8_t r = rgb[i * 3];
        uint8_t g = rgb[i * 3 + 1];
        uint8_t b = rgb[i * 3 + 2];
        _framebuffer[i] = DisplayManager::rgb565(r, g, b);
    }
    _lastPush = millis();
    _dirty = true;
}

void CanvasScreen::pushDrawCommands(const JsonObjectConst& cmd) {
    // AWTRIX3-compatible draw commands
    // dp = draw pixel: [[x, y, "#RRGGBB"], ...]
    if (cmd["dp"].is<JsonArrayConst>()) {
        for (JsonArrayConst pixel : cmd["dp"].as<JsonArrayConst>()) {
            if (pixel.size() >= 3) {
                int16_t px = pixel[0];
                int16_t py = pixel[1];
                uint16_t c = DisplayManager::hexToColor(pixel[2].as<String>());
                if (px >= 0 && px < 32 && py >= 0 && py < 8) {
                    _framebuffer[py * 32 + px] = c;
                }
            }
        }
    }

    // dl = draw line: [[x0, y0, x1, y1, "#RRGGBB"], ...]
    if (cmd["dl"].is<JsonArrayConst>()) {
        for (JsonArrayConst line : cmd["dl"].as<JsonArrayConst>()) {
            if (line.size() >= 5) {
                int16_t x0 = line[0], y0 = line[1], x1 = line[2], y1 = line[3];
                uint16_t c = DisplayManager::hexToColor(line[4].as<String>());
                // Bresenham for framebuffer
                int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
                int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1;
                int16_t err = dx - dy;
                while (true) {
                    if (x0 >= 0 && x0 < 32 && y0 >= 0 && y0 < 8)
                        _framebuffer[y0 * 32 + x0] = c;
                    if (x0 == x1 && y0 == y1) break;
                    int16_t e2 = 2 * err;
                    if (e2 > -dy) { err -= dy; x0 += sx; }
                    if (e2 < dx) { err += dx; y0 += sy; }
                }
            }
        }
    }

    // df = fill rect: [[x, y, w, h, "#RRGGBB"], ...]
    if (cmd["df"].is<JsonArrayConst>()) {
        for (JsonArrayConst rect : cmd["df"].as<JsonArrayConst>()) {
            if (rect.size() >= 5) {
                int16_t rx = rect[0], ry = rect[1], rw = rect[2], rh = rect[3];
                uint16_t c = DisplayManager::hexToColor(rect[4].as<String>());
                for (int16_t yy = ry; yy < ry + rh && yy < 8; yy++) {
                    for (int16_t xx = rx; xx < rx + rw && xx < 32; xx++) {
                        if (xx >= 0 && yy >= 0)
                            _framebuffer[yy * 32 + xx] = c;
                    }
                }
            }
        }
    }

    // dt = draw text: [[x, y, "text", "#RRGGBB"], ...]
    // Text in framebuffer is complex; we'll handle it at render time via DisplayManager
    // For now, skip dt in framebuffer mode

    // cl = clear
    if (cmd["cl"].is<bool>() && cmd["cl"].as<bool>()) {
        clearFramebuffer();
    }

    _lastPush = millis();
    _dirty = true;
}

bool CanvasScreen::update(DisplayManager& display, unsigned long now) {
    // Check lifetime expiry
    if (_lifetime > 0 && _lastPush > 0 && (now - _lastPush) > (_lifetime * 1000UL)) {
        // Auto-disable this screen
        enabled = false;
        return false;
    }

    if (!_dirty) return false; // No need to redraw

    display.clear();
    // Blit framebuffer to display
    for (int16_t y = 0; y < 8; y++) {
        for (int16_t x = 0; x < 32; x++) {
            uint16_t c = _framebuffer[y * 32 + x];
            if (c != 0) {
                display.drawPixel(x, y, c);
            }
        }
    }

    _dirty = false;
    return true;
}

void CanvasScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["lifetime"].is<int>()) _lifetime = cfg["lifetime"];
}

void CanvasScreen::serialize(JsonObject& out) const {
    out["lifetime"] = _lifetime;
}
