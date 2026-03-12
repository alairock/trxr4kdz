#pragma once

#include <Arduino.h>
#include <FastLED.h>
#include <FastLED_NeoMatrix.h>
#include <Adafruit_GFX.h>
#include "pins.h"

#define MATRIX_WIDTH  32
#define MATRIX_HEIGHT  8
#define NUM_LEDS      (MATRIX_WIDTH * MATRIX_HEIGHT)

class DisplayManager {
public:
    void begin(uint8_t brightness);
    void showText(const String& text, uint16_t color = 0xFFFF);
    void update();
    FastLED_NeoMatrix* getMatrix() { return _matrix; }

    // Drawing primitives
    void clear();
    void drawPixel(int16_t x, int16_t y, uint16_t color);
    void drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
    void drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
    void drawText(int16_t x, int16_t y, const String& text, uint16_t color);
    void drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color);
    void drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct, uint16_t fgColor, uint16_t bgColor);
    void show();
    void setBrightness(uint8_t b);

    static uint16_t rgb565(uint8_t r, uint8_t g, uint8_t b);
    static uint16_t hexToColor(const String& hex);

private:
    CRGB _leds[NUM_LEDS];
    FastLED_NeoMatrix* _matrix = nullptr;
    unsigned long _lastShow = 0;
    static const unsigned long FRAME_INTERVAL = 33; // ~30fps
};
