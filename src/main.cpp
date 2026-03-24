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
#include "MqttManager.h"
#include "AlarmManager.h"
#include "OvertakeManager.h"
#include "NotificationManager.h"

ConfigManager configManager;
BuzzerManager buzzerManager;
DisplayManager displayManager;
WiFiManager wifiManager;
WebServerManager webServerManager;
ScreenManager screenManager;
ButtonManager buttonManager;
SettingsMenu settingsMenu;
WeatherService weatherService;
MqttManager mqttManager;
AlarmManager alarmManager;
OvertakeManager overtakeManager;
NotificationManager notificationManager;

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

// Fallback when no enabled screens: scroll config URL
String noScreenUrl;
int16_t noScreenScrollX = MATRIX_WIDTH;
unsigned long noScreenLastStep = 0;

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
    displayManager.showSmallRainbow("trxr4kdz");

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
    settingsMenu.begin(displayManager, screenManager, wifiManager);

    // Start HTTP server (with screen manager)
    webServerManager.begin(configManager, wifiManager, displayManager, screenManager, &weatherService, &alarmManager, &overtakeManager, &notificationManager);

    // MQTT bridge (canvas + notification updates via MQTT topics)
    mqttManager.begin(configManager, screenManager, overtakeManager, &notificationManager);

    // Alarm scheduler/renderer (global feature)
    alarmManager.begin(configManager, buzzerManager);
    overtakeManager.begin(buzzerManager, screenManager);
    notificationManager.begin();

    Serial.println("=== Boot complete ===");
}

void loop() {
    // Rainbow boot banner until we start boot IP flow
    if (!buzzerInitDone) {
        static unsigned long lastBootAnim = 0;
        unsigned long now = millis();
        if (now - lastBootAnim >= 90) {
            lastBootAnim = now;
            displayManager.showSmallRainbow("trxr4kdz");
        }
    }

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
    mqttManager.update();
    alarmManager.update();
    buttonManager.update();

    // Read physical button events
    ButtonEvent leftEvt = buttonManager.getLeftEvent();
    ButtonEvent rightEvt = buttonManager.getRightEvent();
    ButtonEvent middleEvt = buttonManager.getMiddleEvent();

    // Merge remote/API-triggered button actions
    auto remote = webServerManager.takeRemoteButtonAction();
    if (remote == WebServerManager::RemoteButtonAction::LEFT_SHORT) leftEvt = ButtonEvent::SHORT_PRESS;
    else if (remote == WebServerManager::RemoteButtonAction::RIGHT_SHORT) rightEvt = ButtonEvent::SHORT_PRESS;
    else if (remote == WebServerManager::RemoteButtonAction::MIDDLE_SHORT) middleEvt = ButtonEvent::SHORT_PRESS;
    else if (remote == WebServerManager::RemoteButtonAction::MIDDLE_DOUBLE) middleEvt = ButtonEvent::DOUBLE_PRESS;
    else if (remote == WebServerManager::RemoteButtonAction::MIDDLE_LONG) middleEvt = ButtonEvent::LONG_PRESS;

    if (alarmManager.isAlarming()) {
        // Alarm overrides: single middle = snooze, double middle = dismiss until next schedule
        if (middleEvt == ButtonEvent::SHORT_PRESS) {
            alarmManager.snooze();
        } else if (middleEvt == ButtonEvent::DOUBLE_PRESS) {
            alarmManager.dismissUntilNextSchedule();
        }
    } else if (overtakeManager.isActive()) {
        // Adhoc overtake overrides: single middle = stop audio only, double = clear takeover
        if (middleEvt == ButtonEvent::SHORT_PRESS) {
            overtakeManager.stopSound();
        } else if (middleEvt == ButtonEvent::DOUBLE_PRESS) {
            overtakeManager.clear();
        }
    } else if (settingsMenu.isActive()) {
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

        // Left/Right scrub always (both playing and paused).
        // nextScreen/prevScreen reset switch timer, so duration restarts on the new screen.
        if (screensStarted) {
            if (leftEvt == ButtonEvent::SHORT_PRESS) {
                screenManager.prevScreen();
            }
            if (rightEvt == ButtonEvent::SHORT_PRESS) {
                screenManager.nextScreen();
            }
        }
    }

    overtakeManager.update();

    unsigned long nowMs = millis();
    bool devicePreviewActive = webServerManager.renderDevicePreviewIfActive(nowMs);

    if (!devicePreviewActive) {
        if (alarmManager.isAlarming()) {
            alarmManager.render(displayManager, nowMs);
        } else if (overtakeManager.isActive()) {
            overtakeManager.render(displayManager);
        } else if (screensStarted && !settingsMenu.isActive()) {
            if (statusFlashUntil > nowMs) {
                displayManager.showSmallRainbow(statusFlashText);
            } else if (screenManager.getActiveScreen() == nullptr) {
                if (noScreenUrl.length() == 0 || (nowMs % 5000) < 60) {
                    noScreenUrl = "http://" + (wifiManager.isSTAConnected() ? wifiManager.getSTAIP() : wifiManager.getAPIP());
                }
                if (nowMs - noScreenLastStep >= 50) {
                    noScreenLastStep = nowMs;
                    displayManager.clear();
                    int16_t ty = (8 - DisplayManager::fontHeight(1)) / 2;
                    uint8_t charW = DisplayManager::fontCharWidth(1);
                    uint8_t hueStep = 256 / max((int)noScreenUrl.length(), 1);
                    uint8_t hueOffset = (nowMs / 10) & 0xFF;
                    int16_t cx = noScreenScrollX;
                    for (unsigned int i = 0; i < noScreenUrl.length(); i++) {
                        CRGB c = CHSV(hueOffset + i * hueStep, 255, 255);
                        uint16_t color = DisplayManager::rgb565(c.r, c.g, c.b);
                        char ch[2] = { noScreenUrl[i], 0 };
                        displayManager.drawFontText(cx, ty, ch, color, 1);
                        cx += (noScreenUrl[i] == '.' || noScreenUrl[i] == ':') ? 2 : charW;
                    }
                    displayManager.show();
                    noScreenScrollX--;

                    int16_t textW = 0;
                    for (unsigned int i = 0; i < noScreenUrl.length(); i++)
                        textW += (noScreenUrl[i] == '.' || noScreenUrl[i] == ':') ? 2 : charW;
                    if (noScreenScrollX < -textW) noScreenScrollX = MATRIX_WIDTH;
                }
            } else {
                // Reset fallback scroller when a screen is active again
                noScreenScrollX = MATRIX_WIDTH;
                noScreenLastStep = nowMs;
                screenManager.update();
            }
        }

        bool wifiDisconnected = !wifiManager.isSTAConnected();
        bool mqttConfigured = configManager.getMqttEnabled() && configManager.getMqttHost().length() > 0;
        bool mqttProblem = mqttConfigured && !mqttManager.isConnected();
        notificationManager.render(displayManager, wifiDisconnected, mqttProblem, nowMs);
    }

    // Notifications must be on top of everything.
    displayManager.show();
}
