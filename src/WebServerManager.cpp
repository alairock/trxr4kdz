#include "WebServerManager.h"
#include "screens/CanvasScreen.h"
#include <ArduinoJson.h>

void WebServerManager::begin(ConfigManager& config, WiFiManager& wifi, DisplayManager& display, ScreenManager& screens, WeatherService* weather) {
    _config = &config;
    _wifi = &wifi;
    _display = &display;
    _screens = &screens;
    _weather = weather;

    _server.on("/api/status", HTTP_GET, [this]() { handleStatus(); });
    _server.on("/api/weather/debug", HTTP_GET, [this]() {
        if (!_weather) {
            _server.send(500, "application/json", "{\"error\":\"no weather service\"}");
            return;
        }
        JsonDocument doc;
        doc["hasData"] = _weather->hasData();
        doc["online"] = _weather->isOnline();
        doc["tempF"] = _weather->getTemperatureF();
        doc["humidity"] = _weather->getHumidity();
        doc["zipCode"] = _weather->getZipCode();
        doc["hasZipCode"] = _weather->hasZipCode();
        doc["hasGeocode"] = _weather->hasGeocode();
        doc["needsGeocode"] = _weather->needsGeocode();
        doc["lat"] = _weather->getLat();
        doc["lon"] = _weather->getLon();
        doc["lastError"] = _weather->getLastError();
        String json;
        serializeJson(doc, json);
        _server.send(200, "application/json", json);
    });
    _server.on("/api/config", HTTP_GET, [this]() { handleGetConfig(); });
    _server.on("/api/config", HTTP_POST, [this]() { handlePostConfig(); });
    _server.on("/api/wifi", HTTP_POST, [this]() { handlePostWifi(); });
    _server.on("/api/update", HTTP_POST,
        [this]() { handleUpdate(); },
        [this]() { handleUpdateUpload(); }
    );

    // Screen API routes
    _server.on("/api/screens", HTTP_GET, [this]() { handleGetScreens(); });
    _server.on("/api/screens", HTTP_POST, [this]() { handlePostScreen(); });
    _server.on("/api/screens/next", HTTP_POST, [this]() { handleScreensNext(); });
    _server.on("/api/screens/reorder", HTTP_POST, [this]() { handleScreensReorder(); });
    _server.on("/api/screens/cycling", HTTP_PUT, [this]() { handleScreensCycling(); });

    // Dynamic routes via onNotFound
    _server.onNotFound([this]() {
        String uri = _server.uri();
        String param;

        if (_server.method() == HTTP_GET && matchRoute(uri, "/api/screens/", param)) {
            handleGetScreen();
        } else if (_server.method() == HTTP_PUT && matchRoute(uri, "/api/screens/", param)) {
            handlePutScreen();
        } else if (_server.method() == HTTP_DELETE && matchRoute(uri, "/api/screens/", param)) {
            handleDeleteScreen();
        } else if (_server.method() == HTTP_POST && uri.endsWith("/enable")) {
            handleScreenEnable();
        } else if (_server.method() == HTTP_POST && uri.endsWith("/disable")) {
            handleScreenDisable();
        } else if (_server.method() == HTTP_POST && uri.startsWith("/api/canvas/") && uri.endsWith("/draw")) {
            handleCanvasDraw();
        } else if (_server.method() == HTTP_POST && uri.startsWith("/api/canvas/")) {
            handleCanvasPush();
        } else {
            _server.send(404, "application/json", "{\"error\":\"not found\"}");
        }
    });

    _server.begin();
    Serial.println("[Web] Server started on port 80");
}

void WebServerManager::update() {
    _server.handleClient();
}

bool WebServerManager::matchRoute(const String& uri, const String& pattern, String& param) {
    if (!uri.startsWith(pattern)) return false;
    param = uri.substring(pattern.length());
    // Remove trailing slashes or sub-paths for simple matching
    int slash = param.indexOf('/');
    if (slash >= 0) param = param.substring(0, slash);
    return param.length() > 0;
}

// --- Existing handlers ---

void WebServerManager::handleStatus() {
    JsonDocument doc;
    doc["version"] = FIRMWARE_VERSION;
    doc["hostname"] = _config->getHostname();
    doc["ap_ip"] = _wifi->getAPIP();
    doc["sta_ip"] = _wifi->getSTAIP();
    doc["sta_connected"] = _wifi->isSTAConnected();
    doc["rssi"] = WiFi.RSSI();
    doc["free_heap"] = ESP.getFreeHeap();
    doc["uptime_s"] = millis() / 1000;

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetConfig() {
    JsonDocument doc;
    doc["wifi_ssid"] = _config->getWifiSSID();
    doc["wifi_password"] = "********";
    doc["brightness"] = _config->getBrightness();
    doc["hostname"] = _config->getHostname();
    doc["ap_password"] = "********";

    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePostConfig() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    if (doc["brightness"].is<int>()) {
        _config->setBrightness(doc["brightness"]);
    }
    if (doc["hostname"].is<const char*>()) {
        _config->setHostname(doc["hostname"].as<String>());
    }
    if (doc["ap_password"].is<const char*>()) {
        _config->setAPPassword(doc["ap_password"].as<String>());
    }

    _config->save();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handlePostWifi() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    String ssid = doc["ssid"] | "";
    String password = doc["password"] | "";
    if (ssid.length() == 0) {
        _server.send(400, "application/json", "{\"error\":\"ssid required\"}");
        return;
    }

    _config->setWifiSSID(ssid);
    _config->setWifiPassword(password);
    _config->save();

    WiFi.mode(WIFI_AP_STA);
    _wifi->connectSTA(ssid, password);

    _server.send(200, "application/json", "{\"status\":\"connecting\"}");
}

void WebServerManager::handleUpdate() {
    if (Update.hasError()) {
        _server.send(500, "application/json", "{\"error\":\"update failed\"}");
        _display->showSmallRainbow("FAIL");
    } else {
        _server.send(200, "application/json", "{\"status\":\"ok, restarting\"}");
        _display->showSmallRainbow("trxr4kdz");
        delay(1000);
        ESP.restart();
    }
}

void WebServerManager::handleUpdateUpload() {
    HTTPUpload& upload = _server.upload();

    if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("[OTA] Begin: %s\n", upload.filename.c_str());
        _display->showSmallRainbow("UPDATE");
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
            Update.printError(Serial);
        }
    } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) {
            Serial.printf("[OTA] Success: %u bytes\n", upload.totalSize);
        } else {
            Update.printError(Serial);
        }
    }
}

// --- Screen API handlers ---

void WebServerManager::handleGetScreens() {
    JsonDocument doc;
    _screens->serializeAll(doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleGetScreen() {
    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    JsonDocument doc;
    _screens->serializeScreen(id, doc);
    String json;
    serializeJson(doc, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handlePostScreen() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    const char* typeStr = doc["type"] | "";
    ScreenType type;
    if (strcmp(typeStr, "clock") == 0) type = ScreenType::CLOCK;
    else if (strcmp(typeStr, "text_ticker") == 0) type = ScreenType::TEXT_TICKER;
    else if (strcmp(typeStr, "canvas") == 0) type = ScreenType::CANVAS;
    else if (strcmp(typeStr, "weather") == 0) type = ScreenType::WEATHER;
    else {
        _server.send(400, "application/json", "{\"error\":\"invalid type\"}");
        return;
    }

    String id = doc["id"] | "";
    Screen* s = _screens->addScreen(type, id);
    if (!s) {
        _server.send(500, "application/json", "{\"error\":\"max screens reached\"}");
        return;
    }

    if (doc["enabled"].is<bool>()) s->enabled = doc["enabled"];
    if (doc["duration"].is<int>()) s->durationSec = doc["duration"];
    if (doc["settings"].is<JsonObject>()) {
        s->configure(doc["settings"].as<JsonObjectConst>());
    }

    _screens->save();

    JsonDocument resp;
    _screens->serializeScreen(s->id, resp);
    String json;
    serializeJson(resp, json);
    _server.send(201, "application/json", json);
}

void WebServerManager::handlePutScreen() {
    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    if (doc["enabled"].is<bool>()) s->enabled = doc["enabled"];
    if (doc["duration"].is<int>()) s->durationSec = doc["duration"];
    if (doc["settings"].is<JsonObject>()) {
        s->configure(doc["settings"].as<JsonObjectConst>());
    }

    _screens->save();

    JsonDocument resp;
    _screens->serializeScreen(id, resp);
    String json;
    serializeJson(resp, json);
    _server.send(200, "application/json", json);
}

void WebServerManager::handleDeleteScreen() {
    String uri = _server.uri();
    String id = uri.substring(String("/api/screens/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    if (_screens->removeScreen(id)) {
        _screens->save();
        _server.send(200, "application/json", "{\"status\":\"deleted\"}");
    } else {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
    }
}

void WebServerManager::handleScreenEnable() {
    String uri = _server.uri();
    // /api/screens/{id}/enable
    String id = uri.substring(String("/api/screens/").length());
    id = id.substring(0, id.indexOf('/'));

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    s->enabled = true;
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"enabled\"}");
}

void WebServerManager::handleScreenDisable() {
    String uri = _server.uri();
    // /api/screens/{id}/disable
    String id = uri.substring(String("/api/screens/").length());
    id = id.substring(0, id.indexOf('/'));

    Screen* s = _screens->getScreen(id);
    if (!s) {
        _server.send(404, "application/json", "{\"error\":\"screen not found\"}");
        return;
    }

    s->enabled = false;
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"disabled\"}");
}

void WebServerManager::handleScreensReorder() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    JsonArray order = doc["order"].as<JsonArray>();
    if (!order) {
        _server.send(400, "application/json", "{\"error\":\"order array required\"}");
        return;
    }

    String ids[MAX_SCREENS];
    uint8_t count = 0;
    for (JsonVariant v : order) {
        if (count >= MAX_SCREENS) break;
        ids[count++] = v.as<String>();
    }

    _screens->reorder(ids, count);
    _screens->save();
    _server.send(200, "application/json", "{\"status\":\"reordered\"}");
}

void WebServerManager::handleScreensNext() {
    _screens->nextScreen();
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleScreensCycling() {
    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    bool enabled = doc["enabled"] | true;
    _screens->setCycling(enabled);
    _screens->save();

    String json = enabled ? "{\"cycling\":true}" : "{\"cycling\":false}";
    _server.send(200, "application/json", json);
}

void WebServerManager::handleCanvasPush() {
    String uri = _server.uri();
    // /api/canvas/{id}
    String id = uri.substring(String("/api/canvas/").length());
    int slash = id.indexOf('/');
    if (slash >= 0) id = id.substring(0, slash);

    CanvasScreen* canvas = _screens->getCanvasScreen(id);
    if (!canvas) {
        _server.send(404, "application/json", "{\"error\":\"canvas screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    // Expect raw RGB data or base64 in JSON
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (!err && doc["rgb"].is<const char*>()) {
        // Base64-encoded RGB data
        // For simplicity, accept raw hex string of RGB values
        const char* hexData = doc["rgb"];
        size_t hexLen = strlen(hexData);
        size_t byteLen = hexLen / 2;
        if (byteLen > 0) {
            uint8_t* rgb = new uint8_t[byteLen];
            for (size_t i = 0; i < byteLen; i++) {
                char hex[3] = { hexData[i*2], hexData[i*2+1], 0 };
                rgb[i] = (uint8_t)strtoul(hex, nullptr, 16);
            }
            canvas->pushPixels(rgb, byteLen);
            delete[] rgb;
        }
    }

    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}

void WebServerManager::handleCanvasDraw() {
    String uri = _server.uri();
    // /api/canvas/{id}/draw
    String id = uri.substring(String("/api/canvas/").length());
    id = id.substring(0, id.indexOf('/'));

    CanvasScreen* canvas = _screens->getCanvasScreen(id);
    if (!canvas) {
        _server.send(404, "application/json", "{\"error\":\"canvas screen not found\"}");
        return;
    }

    if (!_server.hasArg("plain")) {
        _server.send(400, "application/json", "{\"error\":\"no body\"}");
        return;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, _server.arg("plain"));
    if (err) {
        _server.send(400, "application/json", "{\"error\":\"invalid json\"}");
        return;
    }

    canvas->pushDrawCommands(doc.as<JsonObjectConst>());
    _server.send(200, "application/json", "{\"status\":\"ok\"}");
}
