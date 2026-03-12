#include "DisplayManager.h"

void DisplayManager::begin(uint8_t brightness) {
    FastLED.addLeds<WS2812B, PIN_MATRIX_DATA, GRB>(_leds, NUM_LEDS);
    FastLED.setBrightness(brightness);
    FastLED.clear(true);

    _matrix = new FastLED_NeoMatrix(
        _leds, MATRIX_WIDTH, MATRIX_HEIGHT,
        NEO_MATRIX_TOP + NEO_MATRIX_LEFT + NEO_MATRIX_ROWS + NEO_MATRIX_ZIGZAG
    );
    _matrix->begin();
    _matrix->setTextWrap(false);
    _matrix->setBrightness(brightness);

    Serial.printf("[Display] Init %dx%d, brightness %d\n", MATRIX_WIDTH, MATRIX_HEIGHT, brightness);
}

void DisplayManager::showText(const String& text, uint16_t color) {
    if (!_matrix) return;
    _matrix->fillScreen(0);
    _matrix->setCursor(1, 0);
    _matrix->setTextColor(color);
    _matrix->print(text);
    _matrix->show();
    _lastShow = millis();
}

void DisplayManager::update() {
    if (!_matrix) return;
    unsigned long now = millis();
    if (now - _lastShow >= FRAME_INTERVAL) {
        _matrix->show();
        _lastShow = now;
    }
}

// Drawing primitives

void DisplayManager::clear() {
    if (!_matrix) return;
    _matrix->fillScreen(0);
}

void DisplayManager::drawPixel(int16_t x, int16_t y, uint16_t color) {
    if (!_matrix) return;
    _matrix->drawPixel(x, y, color);
}

void DisplayManager::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (!_matrix) return;
    _matrix->drawLine(x0, y0, x1, y1, color);
}

void DisplayManager::drawRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_matrix) return;
    _matrix->drawRect(x, y, w, h, color);
}

void DisplayManager::fillRect(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color) {
    if (!_matrix) return;
    _matrix->fillRect(x, y, w, h, color);
}

void DisplayManager::drawText(int16_t x, int16_t y, const String& text, uint16_t color) {
    if (!_matrix) return;
    _matrix->setCursor(x, y);
    _matrix->setTextColor(color);
    _matrix->print(text);
}

void DisplayManager::drawBitmap(int16_t x, int16_t y, const uint8_t* bitmap, int16_t w, int16_t h, uint16_t color) {
    if (!_matrix) return;
    _matrix->drawBitmap(x, y, bitmap, w, h, color);
}

void DisplayManager::drawProgressBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t pct, uint16_t fgColor, uint16_t bgColor) {
    if (!_matrix) return;
    int16_t fillW = (int16_t)((uint32_t)w * pct / 100);
    if (fillW > w) fillW = w;
    // Draw background
    _matrix->fillRect(x, y, w, h, bgColor);
    // Draw foreground
    if (fillW > 0) {
        _matrix->fillRect(x, y, fillW, h, fgColor);
    }
}

void DisplayManager::show() {
    if (!_matrix) return;
    _matrix->show();
    _lastShow = millis();
}

void DisplayManager::setBrightness(uint8_t b) {
    if (!_matrix) return;
    _matrix->setBrightness(b);
    FastLED.setBrightness(b);
}

uint16_t DisplayManager::rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

uint16_t DisplayManager::hexToColor(const String& hex) {
    // Parse "#RRGGBB" or "RRGGBB"
    const char* p = hex.c_str();
    if (*p == '#') p++;
    uint32_t val = strtoul(p, nullptr, 16);
    uint8_t r = (val >> 16) & 0xFF;
    uint8_t g = (val >> 8) & 0xFF;
    uint8_t b = val & 0xFF;
    return rgb565(r, g, b);
}
