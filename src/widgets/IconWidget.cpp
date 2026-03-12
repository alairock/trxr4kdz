#include "IconWidget.h"
#include "../DisplayManager.h"

// Simple 8x8 icons (1 bit per pixel, MSB left)
const uint8_t IconWidget::ICON_CLOCK[] = {
    0b00111100,
    0b01000010,
    0b10010001,
    0b10010001,
    0b10001111,
    0b10000000,
    0b01000010,
    0b00111100
};

const uint8_t IconWidget::ICON_TEXT[] = {
    0b11111111,
    0b10000001,
    0b10111101,
    0b10000001,
    0b10111101,
    0b10110101,
    0b10000001,
    0b11111111
};

const uint8_t IconWidget::ICON_SMILE[] = {
    0b00111100,
    0b01000010,
    0b10100101,
    0b10000001,
    0b10100101,
    0b10011001,
    0b01000010,
    0b00111100
};

static const uint8_t* getIconById(uint16_t id) {
    switch (id) {
        case 0: return IconWidget::ICON_CLOCK;
        case 1: return IconWidget::ICON_TEXT;
        case 2: return IconWidget::ICON_SMILE;
        default: return nullptr;
    }
}

void IconWidget::draw(DisplayManager& display, unsigned long now) {
    const uint8_t* icon = getIconById(iconId);
    if (icon) {
        display.drawBitmap(x, y, icon, 8, 8, color);
    }
}

void IconWidget::configure(const JsonObjectConst& cfg) {
    if (cfg["iconId"].is<int>()) iconId = cfg["iconId"];
    if (cfg["color"].is<const char*>()) color = DisplayManager::hexToColor(cfg["color"].as<String>());
}

void IconWidget::serialize(JsonObject& out) const {
    out["iconId"] = iconId;
}
