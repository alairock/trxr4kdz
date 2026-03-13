#include <Arduino.h>
#include <time.h>
#include "pins.h"
#include "ConfigManager.h"
#include "BuzzerManager.h"
#include "DisplayManager.h"
#include "WiFiManager.h"
#include "WebServerManager.h"
#include "screens/ScreenManager.h"
#include "ButtonManager.h"
#include "SettingsMenu.h"
#include "WeatherService.h"

ConfigManager configManager;
BuzzerManager buzzerManager;
DisplayManager displayManager;
WiFiManager wifiManager;
WebServerManager webServerManager;
ScreenManager screenManager;
ButtonManager buttonManager;
SettingsMenu settingsMenu;
WeatherService weatherService;

bool buzzerInitDone = false;
bool screensStarted = false;
unsigned long bootTime;

// transient playback status overlay
unsigned long statusFlashUntil = 0;
String statusFlashText = "";

// Boot IP scroll state
String bootIP;
bool bootIPScrolling = false;
int16_t bootIPScrollX = MATRIX_WIDTH;
unsigned long bootIPLastStep = 0;

void setup() {
    // Silence buzzer ASAP — GPIO 15 floats high during flash/reset
    pinMode(PIN_BUZZER, OUTPUT);
    digitalWrite(PIN_BUZZER, LOW);

    Serial.begin(115200);
    Serial.println("\n=== Ulanzi TC001 Boot ===");

    bootTime = millis();

    buzzerManager.begin();

    // Load config from LittleFS
    configManager.begin();

    // Init display and show boot message
    displayManager.begin(configManager.getBrightness());
    displayManager.showSmallRainbow("BOOT");

    // Start WiFi (AP always, STA if configured)
    wifiManager.begin(configManager);

    // Configure NTP time sync with Pacific timezone default
    setenv("TZ", "MST7MDT,M3.2.0,M11.1.0", 1);
    tzset();
    configTime(0, 0, "pool.ntp.org", "time.nist.gov");

    // Init weather service (I2C sensor + online fetch)
    weatherService.begin();

    // Init buttons
    buttonManager.begin();

    // Init settings menu
    settingsMenu.begin(displayManager, screenManager);

    // Start HTTP server (with screen manager)
    webServerManager.begin(configManager, wifiManager, displayManager, screenManager, &weatherService);

    Serial.println("=== Boot complete ===");
}

void loop() {
    // After 2s, enable buzzer, beep, show IP, then start screens
    if (!buzzerInitDone && millis() - bootTime >= 2000) {
        buzzerManager.ready();
        buzzerManager.beep(1000, 150);
        buzzerInitDone = true;

        // Start scrolling IP address
        if (wifiManager.isSTAConnected()) {
            bootIP = wifiManager.getSTAIP();
        } else {
            bootIP = wifiManager.getAPIP();
        }
        bootIPScrolling = true;
        bootIPScrollX = MATRIX_WIDTH;
        bootIPLastStep = millis();
    }

    // Scroll IP during boot with rainbow colors
    if (bootIPScrolling) {
        unsigned long now = millis();
        if (now - bootIPLastStep >= 50) { // ~20px/sec scroll speed
            bootIPLastStep = now;
            displayManager.clear();
            int16_t ty = (8 - DisplayManager::fontHeight(1)) / 2;
            uint8_t charW = DisplayManager::fontCharWidth(1);
            uint8_t hueStep = 256 / max((int)bootIP.length(), 1);
            uint8_t hueOffset = (now / 10) & 0xFF;
            int16_t cx = bootIPScrollX;
            for (unsigned int i = 0; i < bootIP.length(); i++) {
                CRGB c = CHSV(hueOffset + i * hueStep, 255, 255);
                uint16_t color = DisplayManager::rgb565(c.r, c.g, c.b);
                char ch[2] = { bootIP[i], 0 };
                displayManager.drawFontText(cx, ty, ch, color, 1);
                cx += (bootIP[i] == '.') ? 2 : charW;
            }
            displayManager.show();
            bootIPScrollX--;

            // Stop when text fully scrolled off left
            int16_t textW = 0;
            for (unsigned int i = 0; i < bootIP.length(); i++)
                textW += (bootIP[i] == '.') ? 2 : charW;
            if (bootIPScrollX < -textW) {
                bootIPScrolling = false;
            }
        }
    }

    // Start screens after IP scroll finishes
    if (!screensStarted && buzzerInitDone && !bootIPScrolling) {
        screenManager.begin(displayManager, &weatherService);
        screensStarted = true;
    }

    buzzerManager.update();
    wifiManager.update();
    webServerManager.update();
    weatherService.update();
    buttonManager.update();

    // Read button events
    ButtonEvent leftEvt = buttonManager.getLeftEvent();
    ButtonEvent rightEvt = buttonManager.getRightEvent();
    ButtonEvent middleEvt = buttonManager.getMiddleEvent();

    if (settingsMenu.isActive()) {
        // Settings menu handles all input
        settingsMenu.update(leftEvt, rightEvt, middleEvt);
    } else {
        // Long press middle enters settings
        if (middleEvt == ButtonEvent::LONG_PRESS && screensStarted) {
            settingsMenu.enter();
        }

        // Double press middle toggles play/pause rotation
        if (middleEvt == ButtonEvent::DOUBLE_PRESS && screensStarted) {
            bool nowCycling = !screenManager.isCycling();
            screenManager.setCycling(nowCycling);
            screenManager.save();
            statusFlashText = nowCycling ? "playing" : "paused";
            statusFlashUntil = millis() + 1200;
        }

        // Left/Right scrub only when paused
        if (!screenManager.isCycling() && screensStarted) {
            if (leftEvt == ButtonEvent::SHORT_PRESS) {
                screenManager.prevScreen();
            }
            if (rightEvt == ButtonEvent::SHORT_PRESS) {
                screenManager.nextScreen();
            }
        }
    }

    if (screensStarted && !settingsMenu.isActive()) {
        if (statusFlashUntil > millis()) {
            displayManager.showSmallRainbow(statusFlashText);
        } else {
            screenManager.update();
        }
    }
    displayManager.update();
}
