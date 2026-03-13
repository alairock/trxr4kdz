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

private:
    String framebufferPath() const;

    uint16_t _framebuffer[32 * 8]; // 512 bytes
    uint32_t _lifetime = 0;         // 0 = indefinite (no auto-disable)
    unsigned long _lastPush = 0;
    bool _dirty = true;
};
