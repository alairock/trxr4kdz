#include "CalendarWidget.h"
#include "../DisplayManager.h"
#include <time.h>

// 3x4 digit bitmaps (each row is 3 bits, MSB=left)
static const uint8_t DIGIT_3x4[][4] = {
    { 0b111, 0b101, 0b101, 0b111 }, // 0
    { 0b010, 0b110, 0b010, 0b111 }, // 1
    { 0b111, 0b011, 0b100, 0b111 }, // 2
    { 0b111, 0b011, 0b001, 0b111 }, // 3
    { 0b101, 0b111, 0b001, 0b001 }, // 4
    { 0b111, 0b110, 0b001, 0b111 }, // 5
    { 0b111, 0b100, 0b111, 0b111 }, // 6
    { 0b111, 0b001, 0b001, 0b001 }, // 7
    { 0b111, 0b111, 0b101, 0b111 }, // 8
    { 0b111, 0b111, 0b001, 0b111 }, // 9
};

static void drawDigit(DisplayManager& display, int16_t dx, int16_t dy, int digit, uint16_t color) {
    if (digit < 0 || digit > 9) return;
    for (int row = 0; row < 4; row++) {
        uint8_t bits = DIGIT_3x4[digit][row];
        for (int col = 0; col < 3; col++) {
            if (bits & (0b100 >> col)) {
                display.drawPixel(dx + col, dy + row, color);
            }
        }
    }
}

void CalendarWidget::draw(DisplayManager& display, unsigned long now) {
    struct tm timeinfo;
    time_t t = time(nullptr);
    localtime_r(&t, &timeinfo);
    int day = timeinfo.tm_mday;

    // Rows 0-1: solid red header bar (2px thick)
    for (int r = 0; r < 2; r++) {
        for (int i = 0; i < 8; i++) {
            display.drawPixel(x + i, y + r, tabColor);
        }
    }

    // Rows 2-7: white background with black (off) digits
    // Fill background white
    for (int r = 2; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            display.drawPixel(x + c, y + r, frameColor);
        }
    }

    // Punch out digits as black (draw 0x0000 over white)
    int tens = day / 10;
    int ones = day % 10;

    if (tens > 0) {
        drawDigit(display, x + 0, y + 3, tens, 0x0000);
        drawDigit(display, x + 4, y + 3, ones, 0x0000);
    } else {
        drawDigit(display, x + 3, y + 4, ones, 0x0000);
    }
}

void CalendarWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["color"].is<const char*>())
        textColor = DisplayManager::hexToColor(cfg["color"].as<String>());
}

void CalendarWidget::serialize(JsonObject& out) const {
}
