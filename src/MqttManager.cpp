#include "MqttManager.h"
#include <ArduinoJson.h>
#include <WiFi.h>
#include "screens/CanvasScreen.h"

MqttManager* MqttManager::_instance = nullptr;

void MqttManager::begin(ConfigManager& config, ScreenManager& screens, OvertakeManager& overtake) {
    _config = &config;
    _screens = &screens;
    _overtake = &overtake;
    _instance = this;
    _mqtt.setCallback(MqttManager::mqttCallbackThunk);
    _mqtt.setBufferSize(2048); // allow larger canvas draw payloads
}

String MqttManager::topicPrefix() const {
    String base = _config ? _config->getMqttBaseTopic() : String("trxr4kdz");
    if (base.length() == 0) base = "trxr4kdz";
    while (base.endsWith("/")) base.remove(base.length() - 1);
    return base;
}

void MqttManager::mqttCallbackThunk(char* topic, uint8_t* payload, unsigned int length) {
    if (_instance) _instance->mqttCallback(topic, payload, length);
}

void MqttManager::mqttCallback(char* topic, uint8_t* payload, unsigned int length) {
    String t = topic ? String(topic) : String("");
    _lastTopic = t;
    _lastPayloadLen = length;
    _lastMessageMs = millis();
    String prefix = topicPrefix();
    String root = prefix + "/canvas/";
    if (!t.startsWith(root)) return;

    String rest = t.substring(root.length()); // {id}/draw or {id}/rgbhex
    int slash = rest.indexOf('/');
    if (slash < 0) return;
    String id = rest.substring(0, slash);
    String action = rest.substring(slash + 1);

    CanvasScreen* canvas = _screens ? _screens->getCanvasScreen(id) : nullptr;
    if (!canvas) return;

    String body;
    body.reserve(length + 1);
    for (unsigned int i = 0; i < length; i++) body += (char)payload[i];

    if (action == "draw") {
        JsonDocument doc;
        if (deserializeJson(doc, body) == DeserializationError::Ok && doc.is<JsonObject>()) {
            if (_overtake && canvas->overtakeEnabled()) {
                _overtake->pushDrawCommands(doc.as<JsonObjectConst>());
                _overtake->trigger(canvas->overtakeDurationSec(), canvas->overtakeSoundMode(), canvas->overtakeTone(), canvas->overtakeVolume());
            } else {
                canvas->pushDrawCommands(doc.as<JsonObjectConst>());
            }
        }
        return;
    }

    if (action == "rgbhex") {
        size_t hexLen = body.length();
        size_t byteLen = hexLen / 2;
        if (byteLen == 0) return;
        uint8_t* rgb = new uint8_t[byteLen];
        for (size_t i = 0; i < byteLen; i++) {
            char hex[3] = { body[i * 2], body[i * 2 + 1], 0 };
            rgb[i] = (uint8_t)strtoul(hex, nullptr, 16);
        }
        if (_overtake && canvas->overtakeEnabled()) {
            _overtake->pushPixels(rgb, byteLen);
            _overtake->trigger(canvas->overtakeDurationSec(), canvas->overtakeSoundMode(), canvas->overtakeTone(), canvas->overtakeVolume());
        } else {
            canvas->pushPixels(rgb, byteLen);
        }
        delete[] rgb;
        return;
    }
}

void MqttManager::reconnectIfNeeded() {
    if (!_config || !_screens) return;
    if (!_config->getMqttEnabled()) return;
    if (_config->getMqttHost().length() == 0) { _lastError = "mqtt_host_empty"; return; }
    if (WiFi.status() != WL_CONNECTED) { _lastError = "wifi_not_connected"; return; }
    if (_mqtt.connected()) return;

    unsigned long now = millis();
    if (now - _lastReconnectTry < 5000) return;
    _lastReconnectTry = now;
    _reconnectAttempts++;

    // PubSubClient stores the server pointer; keep host storage alive.
    _serverHostCache = _config->getMqttHost();
    _mqtt.setServer(_serverHostCache.c_str(), _config->getMqttPort());

    String clientId = String("trxr4kdz-") + String((uint32_t)ESP.getEfuseMac(), HEX);
    bool ok;
    if (_config->getMqttUser().length() > 0) {
        ok = _mqtt.connect(clientId.c_str(), _config->getMqttUser().c_str(), _config->getMqttPassword().c_str());
    } else {
        ok = _mqtt.connect(clientId.c_str());
    }

    _lastState = _mqtt.state();
    if (!ok) {
        _lastError = String("connect_failed:") + String(_lastState);
        return;
    }

    _lastConnectMs = millis();
    _lastError = "";

    String subDraw = topicPrefix() + "/canvas/+/draw";
    String subRgbHex = topicPrefix() + "/canvas/+/rgbhex";
    _mqtt.subscribe(subDraw.c_str());
    _mqtt.subscribe(subRgbHex.c_str());

    String statusTopic = topicPrefix() + "/status";
    _mqtt.publish(statusTopic.c_str(), "online", true);
}

void MqttManager::update() {
    reconnectIfNeeded();
    if (_mqtt.connected()) _mqtt.loop();
}
