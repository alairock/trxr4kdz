#pragma once

#include "Screen.h"

class CanvasScreen : public Screen {
public:
    CanvasScreen();
    void activate() override;
    bool update(DisplayManager& display, unsigned long now) override;
    void configure(const JsonObjectConst& cfg) override;
    void serialize(JsonObject& out) const override;
    ScreenType type() const override { return ScreenType::CANVAS; }
    const char* typeName() const override { return "canvas"; }

    // External API methods
    void pushPixels(const uint8_t* rgb, size_t len);
    void pushDrawCommands(const JsonObjectConst& cmd);
    void clearFramebuffer();

    // Persistence
    bool saveFramebuffer();
    bool loadFramebuffer();
    const uint16_t* framebuffer() const { return _framebuffer; }
    void composedFrame(uint16_t* out, unsigned long now);

    bool overtakeEnabled() const { return _overtakeEnabled; }
    int32_t overtakeDurationSec() const { return _overtakeDurationSec; }
    String overtakeSoundMode() const { return _overtakeSoundMode; }
    String overtakeTone() const { return _overtakeTone; }
    uint8_t overtakeVolume() const { return _overtakeVolume; }

private:
    String framebufferPath() const;
    void renderEffectFrame(uint16_t* out, unsigned long now);

    uint16_t _framebuffer[32 * 8]; // 512 bytes
    uint32_t _lifetime = 0;         // 0 = indefinite (no auto-disable)
    unsigned long _lastPush = 0;
    bool _dirty = true;

    bool _overtakeEnabled = false;
    int32_t _overtakeDurationSec = 30; // -1 infinite
    String _overtakeSoundMode = "off"; // off|single|repeat
    String _overtakeTone = "beep";
    uint8_t _overtakeVolume = 80;

    bool _effectEnabled = false;
    String _effectType = "rainbow"; // rainbow|plasma|twinkle|noise
    uint8_t _effectSpeed = 50;        // 1..100
    uint8_t _effectIntensity = 60;    // 0..100
};
