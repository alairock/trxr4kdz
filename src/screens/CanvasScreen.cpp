#include "CanvasScreen.h"
#include "../DisplayManager.h"
#include <LittleFS.h>
#include <FastLED.h>

CanvasScreen::CanvasScreen() {
    clearFramebuffer();
    durationSec = 10;
}

void CanvasScreen::activate() {
    _lastPush = millis();
    _dirty = true;
}

void CanvasScreen::clearFramebuffer() {
    memset(_framebuffer, 0, sizeof(_framebuffer));
    _dirty = true;
}

String CanvasScreen::framebufferPath() const {
    return "/canvas-" + id + ".bin";
}

bool CanvasScreen::saveFramebuffer() {
    File f = LittleFS.open(framebufferPath(), "w");
    if (!f) return false;
    f.write((const uint8_t*)_framebuffer, sizeof(_framebuffer));
    f.close();
    Serial.printf("[Canvas] Saved %s (%d bytes)\n", id.c_str(), sizeof(_framebuffer));
    return true;
}

bool CanvasScreen::loadFramebuffer() {
    File f = LittleFS.open(framebufferPath(), "r");
    if (!f) return false;
    size_t read = f.read((uint8_t*)_framebuffer, sizeof(_framebuffer));
    f.close();
    if (read == sizeof(_framebuffer)) {
        Serial.printf("[Canvas] Loaded %s (%d bytes)\n", id.c_str(), read);
        _dirty = true;
        return true;
    }
    return false;
}

void CanvasScreen::pushPixels(const uint8_t* rgb, size_t len) {
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
    saveFramebuffer();
}

void CanvasScreen::pushDrawCommands(const JsonObjectConst& cmd) {
    // Process in painter's order: clear, fills, lines, pixels (foreground last)

    // cl = clear
    if (cmd["cl"].is<bool>() && cmd["cl"].as<bool>()) {
        memset(_framebuffer, 0, sizeof(_framebuffer));
    }

    // df = fill rect (backgrounds): [[x, y, w, h, "#RRGGBB"], ...]
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

    // dl = draw line: [[x0, y0, x1, y1, "#RRGGBB"], ...]
    if (cmd["dl"].is<JsonArrayConst>()) {
        for (JsonArrayConst line : cmd["dl"].as<JsonArrayConst>()) {
            if (line.size() >= 5) {
                int16_t x0 = line[0], y0 = line[1], x1 = line[2], y1 = line[3];
                uint16_t c = DisplayManager::hexToColor(line[4].as<String>());
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

    // dp = draw pixel (foreground): [[x, y, "#RRGGBB"], ...]
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

    _lastPush = millis();
    _dirty = true;
    saveFramebuffer();
}

void CanvasScreen::renderEffectFrame(uint16_t* out, unsigned long now) {
    memset(out, 0, sizeof(uint16_t) * 32 * 8);
    if (!_effectEnabled) return;

    uint8_t speed = _effectSpeed > 100 ? 100 : _effectSpeed;
    uint8_t intensity = _effectIntensity > 100 ? 100 : _effectIntensity;

    // Non-linear speed curve: much slower at low end, still fast at high end.
    float speedNorm = (float)speed / 100.0f;           // 0..1
    float speedMul = 0.03f + speedNorm * speedNorm * 3.2f; // 0.03..3.23
    uint32_t t = (uint32_t)(now * speedMul);

    // Legacy effect ports tuned for 32x8
    if (_effectType == "swirl_in" || _effectType == "swirl_out" || _effectType == "siwrl_out" || _effectType == "siwrlout") {
        float cx = 16.0f, cy = 4.0f;
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 32; x++) {
                float xd = cx - x;
                float yd = cy - y;
                uint16_t dist = (uint16_t)sqrtf(xd * xd + yd * yd);
                uint8_t hue = (_effectType == "swirl_in")
                    ? (uint8_t)(map(dist, 0, 16, 0, 255) + (t / 2))
                    : (uint8_t)(255 - map(dist, 0, 16, 0, 255) + (t / 2));
                CRGB c = CHSV(hue, 255, (uint8_t)(50 + intensity * 2));
                out[y * 32 + x] = DisplayManager::rgb565(c.r, c.g, c.b);
            }
        }
        return;
    }

    if (_effectType == "fade") {
        uint8_t baseHue = (uint8_t)(t / 6);
        for (int y = 0; y < 8; y++) {
            uint8_t hue = baseHue + (uint8_t)(y * 18);
            CRGB c = CHSV(hue, 255, (uint8_t)(40 + intensity * 2));
            uint16_t cc = DisplayManager::rgb565(c.r, c.g, c.b);
            for (int x = 0; x < 32; x++) out[y * 32 + x] = cc;
        }
        return;
    }

    if (_effectType == "matrix") {
        for (int x = 0; x < 32; x++) {
            uint8_t head = (uint8_t)((x * 17 + t / 2) % 8);
            for (int y = 0; y < 8; y++) {
                int d = (head - y + 8) % 8;
                if (d == 0) out[y * 32 + x] = 0x9FEA;
                else if (d < 4) out[y * 32 + x] = DisplayManager::rgb565(0, (uint8_t)(120 - d * 30), 0);
            }
        }
        return;
    }

    if (_effectType == "radar") {
        static CRGB trail[32][8];
        for (int x = 0; x < 32; x++) for (int y = 0; y < 8; y++) trail[x][y].fadeToBlackBy(24);
        uint8_t beam = (uint8_t)(t & 0xFF);
        for (uint8_t r = 0; r <= 32; r++) {
            int i = 16 + (int)(r * (cos8(beam) - 128) / 128.0f);
            int j = 4 - (int)(r * (sin8(beam) - 128) / 128.0f);
            if (i >= 0 && i < 32 && j >= 0 && j < 8) trail[i][j] = CHSV(beam, 255, (uint8_t)(80 + intensity));
        }
        for (int x = 0; x < 32; x++) for (int y = 0; y < 8; y++) out[y * 32 + x] = DisplayManager::rgb565(trail[x][y].r, trail[x][y].g, trail[x][y].b);
        return;
    }

    if (_effectType == "ripple") {
        static int rx = 16, ry = 4;
        static float life = 40;
        static CRGB color = CRGB::Blue;
        if (life > 30) {
            rx = random(32); ry = random(8); life = 0;
            color = CHSV(random8(), 255, 255);
        }
        life += 0.45f + (speed / 120.0f);
        for (int y = 0; y < 8; y++) for (int x = 0; x < 32; x++) {
            int dx = abs(x - rx), dy = abs(y - ry);
            float dist = sqrtf((float)(dx * dx + dy * dy));
            if (dist >= life && dist < life + 2) out[y * 32 + x] = DisplayManager::rgb565(color.r, color.g, color.b);
        }
        return;
    }

    if (_effectType == "ping_pong" || _effectType == "brick_breaker") {
        static int p1 = 4, p2 = 4, d1 = 1, d2 = -1;
        static int bx = 16, by = 4, dx = 1, dy = 1;
        static uint32_t last = 0;
        if (now - last > (uint32_t)(150 - speed)) {
            last = now;
            p1 += d1; p2 += d2;
            if (p1 <= 0 || p1 >= 5) d1 = -d1;
            if (p2 <= 0 || p2 >= 5) d2 = -d2;
            bx += dx; by += dy;
            if (by <= 0 || by >= 7) dy = -dy;
            if (bx <= 0 || bx >= 31) dx = -dx;
        }
        for (int i = 0; i < 3; i++) { out[(p1 + i) * 32 + 0] = 0xFFFF; out[(p2 + i) * 32 + 31] = 0xFFFF; }
        out[by * 32 + bx] = 0xF800;
        if (_effectType == "brick_breaker") {
            for (int x = 0; x < 32; x++) if ((x + (t / 20)) % 2 == 0) out[(x / 11) * 32 + x] = 0x001F;
            int pad = max(0, min(29, bx - 1));
            out[7 * 32 + pad] = out[7 * 32 + pad + 1] = out[7 * 32 + pad + 2] = 0xFFFF;
        }
        return;
    }

    if (_effectType == "snake") {
        static int hx = 0, hy = 0, dir = 1;
        static int ax = 20, ay = 4;
        static uint32_t last = 0;
        if (now - last > (uint32_t)(140 - speed)) {
            last = now;
            if (random8() < 20) dir = random8() % 4;
            if (dir == 0) hy = (hy + 7) % 8; else if (dir == 1) hx = (hx + 1) % 32; else if (dir == 2) hy = (hy + 1) % 8; else hx = (hx + 31) % 32;
            if (hx == ax && hy == ay) { ax = random(32); ay = random(8); }
        }
        out[hy * 32 + hx] = 0x07E0;
        out[ay * 32 + ax] = 0xF800;
        return;
    }

    if (_effectType == "looking_eyes") {
        static float blink = 60;
        static float gaze = 30;
        static int eyeX = 3, eyeY = 3, newX = 3, newY = 3;
        if (blink < 7) {
            int lid = (int)blink;
            for (int ex = 6; ex <= 18; ex += 12) {
                for (int y = max(0, lid); y < min(8, 8 - lid); y++) {
                    for (int x = 0; x < 8; x++) out[y * 32 + ex + x] = 0xFFFF;
                }
            }
        } else {
            for (int ex = 6; ex <= 18; ex += 12) {
                for (int y = 1; y < 7; y++) for (int x = 1; x < 7; x++) out[y * 32 + ex + x] = 0xFFFF;
                out[(eyeY) * 32 + (ex + eyeX)] = 0;
                out[(eyeY) * 32 + (ex + eyeX + 1)] = 0;
                out[(eyeY + 1) * 32 + (ex + eyeX)] = 0;
                out[(eyeY + 1) * 32 + (ex + eyeX + 1)] = 0;
            }
        }
        blink -= 0.1f; if (blink <= 0) blink = random(60, 300);
        gaze -= 0.5f; if (gaze <= 0) { gaze = random(20, 80); newX = random(1, 5); newY = random(1, 5); eyeX = newX; eyeY = newY; }
        return;
    }

    // fallback simple effects
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 32; x++) {
            uint8_t hue = (uint8_t)(x * 8 + t / 8);
            CRGB c = CHSV(hue, 255, (uint8_t)(35 + (intensity * 220) / 100));
            out[y * 32 + x] = DisplayManager::rgb565(c.r, c.g, c.b);
        }
    }
}

void CanvasScreen::composedFrame(uint16_t* out, unsigned long now) {
    uint16_t bg[32 * 8];
    renderEffectFrame(bg, now);

    // Brightness boost for effect layer so high intensity can get punchier.
    float intensityNorm = (_effectIntensity > 100 ? 100 : _effectIntensity) / 100.0f;
    float gain = 0.55f + intensityNorm * 1.35f; // 0.55..1.90
    uint8_t floorV = (uint8_t)(8 + intensityNorm * 36); // keep dim effects visible

    for (int i = 0; i < 32 * 8; i++) {
        uint16_t c = bg[i];
        if (c != 0) {
            uint8_t r5 = (c >> 11) & 0x1F;
            uint8_t g6 = (c >> 5) & 0x3F;
            uint8_t b5 = c & 0x1F;
            uint8_t r = (r5 << 3) | (r5 >> 2);
            uint8_t g = (g6 << 2) | (g6 >> 4);
            uint8_t b = (b5 << 3) | (b5 >> 2);

            int rr = (int)(r * gain); if (rr > 255) rr = 255; if (rr < floorV) rr = floorV;
            int gg = (int)(g * gain); if (gg > 255) gg = 255; if (gg < floorV) gg = floorV;
            int bb = (int)(b * gain); if (bb > 255) bb = 255; if (bb < floorV) bb = floorV;
            bg[i] = DisplayManager::rgb565((uint8_t)rr, (uint8_t)gg, (uint8_t)bb);
        }
    }

    for (int i = 0; i < 32 * 8; i++) {
        uint16_t fg = _framebuffer[i];
        out[i] = (fg != 0) ? fg : bg[i];
    }
}

bool CanvasScreen::update(DisplayManager& display, unsigned long now) {
    if (_lifetime > 0 && _lastPush > 0 && (now - _lastPush) > (_lifetime * 1000UL)) {
        enabled = false;
        return false;
    }

    uint16_t out[32 * 8];
    composedFrame(out, now);

    display.clear();
    for (int16_t y = 0; y < 8; y++) {
        for (int16_t x = 0; x < 32; x++) {
            uint16_t c = out[y * 32 + x];
            if (c != 0) display.drawPixel(x, y, c);
        }
    }
    return true;
}

void CanvasScreen::configure(const JsonObjectConst& cfg) {
    if (cfg["lifetime"].is<int>()) _lifetime = cfg["lifetime"];

    if (cfg["overtakeEnabled"].is<bool>()) _overtakeEnabled = cfg["overtakeEnabled"].as<bool>();
    if (cfg["overtakeDurationSec"].is<int>()) _overtakeDurationSec = cfg["overtakeDurationSec"].as<int>();
    if (cfg["overtakeSoundMode"].is<const char*>()) _overtakeSoundMode = cfg["overtakeSoundMode"].as<String>();
    if (cfg["overtakeTone"].is<const char*>()) _overtakeTone = cfg["overtakeTone"].as<String>();
    if (cfg["overtakeVolume"].is<int>()) _overtakeVolume = (uint8_t)cfg["overtakeVolume"].as<int>();

    if (cfg["effectEnabled"].is<bool>()) _effectEnabled = cfg["effectEnabled"].as<bool>();
    if (cfg["effectType"].is<const char*>()) _effectType = cfg["effectType"].as<String>();
    if (cfg["effectSpeed"].is<int>()) {
        int v = cfg["effectSpeed"].as<int>();
        if (v < 0) v = 0;
        if (v > 100) v = 100;
        _effectSpeed = (uint8_t)v;
    }
    if (cfg["effectIntensity"].is<int>()) {
        int v = cfg["effectIntensity"].as<int>();
        if (v < 0) v = 0;
        if (v > 100) v = 100;
        _effectIntensity = (uint8_t)v;
    }

    // Try to load persisted framebuffer
    loadFramebuffer();
}

void CanvasScreen::serialize(JsonObject& out) const {
    out["lifetime"] = _lifetime;
    out["overtakeEnabled"] = _overtakeEnabled;
    out["overtakeDurationSec"] = _overtakeDurationSec;
    out["overtakeSoundMode"] = _overtakeSoundMode;
    out["overtakeTone"] = _overtakeTone;
    out["overtakeVolume"] = _overtakeVolume;

    out["effectEnabled"] = _effectEnabled;
    out["effectType"] = _effectType;
    out["effectSpeed"] = _effectSpeed;
    out["effectIntensity"] = _effectIntensity;
}
