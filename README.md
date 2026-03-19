# TRXR4KDZ — Custom Firmware for Ulanzi TC001

Custom ESP32 firmware for the **Ulanzi TC001** (32x8 WS2812B matrix) with:

- Modular screen system (clock, binary, weather, ticker, canvas, battery)
- Screen ordering + cycling control
- Canvas API (raw frame + draw commands)
- Alarm API
- MQTT status + config
- OTA update endpoint
- Built-in web UI (`/ui`)

---

## 1) Getting Started

### 1A) Getting started as a **user** (just flashed firmware)

This path is for people who want to run the firmware, not develop it.

#### Step 1: Flash TRXR4KDZ firmware

You can get onto TRXR4KDZ from either stock Ulanzi firmware or AWTRIX 3.

**From AWTRIX 3 (OTA/Web update):**
- Open the AWTRIX update page.
- Upload `firmware.bin` built from this repo.
- If AWTRIX endpoint is exposed, direct update is also typically:
  - `POST /update` with multipart field `update`.

**From stock Ulanzi firmware:**
- If stock OTA accepts app image, upload the same `firmware.bin`.
- If OTA path fails, use serial/USB once:
  - `pio run -e ulanzi -t upload`

#### Step 2: Connect to AP mode and configure Wi-Fi

After boot, device starts AP mode (always available, also when STA is connected):

- **AP SSID format:** `Ulanzi-XXXX` (last MAC bytes)
- **Default AP password:** `12345678`
- **AP IP:** `192.168.4.1`

Connect your phone/laptop to that AP, then open:

- `http://192.168.4.1/` (captive/setup page)

Enter your Wi-Fi credentials and save. Device reboots and joins STA.

#### Step 3: Open the UI

Once connected to your Wi-Fi, open:

- `http://<device-ip>/ui`

Tip: `GET /api/status` returns `sta_ip`, `sta_connected`, and health.

---

### 1B) Getting started as a **hacker/contributor**

#### Prerequisites

- Python 3
- [PlatformIO](https://platformio.org/)
- Ulanzi TC001 connected over USB for dev flashing

#### Clone + build

```bash
git clone https://github.com/alairock/trxr4kdz.git
cd trxr4kdz

# project-venv pio (recommended in this repo)
.venv/bin/pio run -e ulanzi
```

If using global PlatformIO, replace `.venv/bin/pio` with `pio`.

#### Flash (serial)

```bash
# firmware
.venv/bin/pio run -e ulanzi -t upload

# optional LittleFS
.venv/bin/pio run -e ulanzi -t uploadfs

# serial monitor
.venv/bin/pio device monitor
```

#### Flash (OTA)

```bash
curl -X POST http://<device-ip>/api/update \
  -F "firmware=@.pio/build/ulanzi/firmware.bin"
```

---

## 2) Authentication

Protected endpoints require auth **if** `api_token` is configured.

Use either:

- Header: `X-API-Key: <token>`
- Query: `?token=<token>`

Unauthorized response:

- **401** `{"error":"unauthorized"}`

---

## 3) API Documentation

Base URL: `http://<device-ip>`

> Notes:
> - JSON bodies use `Content-Type: application/json` unless multipart is specified.
> - Example responses below are representative samples.

## 3.1 Device / System

### GET `/api/status`  
Auth: **No**

Returns runtime/system health.

**200 sample**
```json
{
  "version": "0.1.0",
  "hostname": "ulanzi",
  "ap_ip": "192.168.4.1",
  "sta_ip": "192.168.0.48",
  "sta_connected": true,
  "rssi": -58,
  "free_heap": 206480,
  "uptime_s": 12,
  "weather": {
    "network_state": "waiting_interval",
    "request_in_flight": false,
    "in_flight_age_ms": 0,
    "last_success_ms": 7020,
    "last_failure_ms": 0,
    "consecutive_failures": 0,
    "backoff_ms": 0,
    "last_op_latency_ms": 631,
    "loop_update_us": 29,
    "loop_update_us_max": 996,
    "last_error": ""
  }
}
```

---

### GET `/api/config`  
Auth: **Yes**

Returns current config (secrets masked/flagged).

**200 sample**
```json
{
  "wifi_ssid": "MyWiFi",
  "wifi_password": "********",
  "brightness": 40,
  "hostname": "ulanzi",
  "ap_password": "********",
  "api_token_set": true,
  "mqtt_enabled": true,
  "mqtt_host": "192.168.0.10",
  "mqtt_port": 1883,
  "mqtt_user": "user",
  "mqtt_password_set": true,
  "mqtt_base_topic": "trxr4kdz"
}
```

---

### POST `/api/config`  
Auth: **Yes**

Patch/update config fields.

**Request schema (all optional):**
```json
{
  "brightness": 0,
  "hostname": "string",
  "ap_password": "string",
  "api_token": "string",
  "mqtt_enabled": true,
  "mqtt_host": "string",
  "mqtt_port": 1883,
  "mqtt_user": "string",
  "mqtt_password": "string",
  "mqtt_base_topic": "string"
}
```

**Responses**
- `200` `{"status":"ok"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`

---

### POST `/api/wifi`  
Auth: **Yes**

Save STA credentials and schedule restart.

**Request schema**
```json
{
  "ssid": "required string",
  "password": "optional string"
}
```

**Responses**
- `200` `{"status":"connecting"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `400` `{"error":"ssid required"}`

---

### POST `/api/update`  
Auth: **Yes**

OTA update upload.

**Request**: `multipart/form-data` with field:
- `firmware` = `.bin`

**Responses**
- `200` `{"status":"ok, restarting"}`
- `500` `{"error":"update failed","reason":"...","code":8}`

---

### GET `/api/weather/debug`  
Auth: **No**

Detailed weather diagnostics.

**200 sample (shape)**
```json
{
  "hasData": true,
  "online": true,
  "tempF": 72.5,
  "humidity": 33,
  "weatherCode": 0,
  "windMph": 5,
  "zipCode": "80014"
}
```

---

## 3.2 MQTT

### GET `/api/mqtt/status`  
Auth: **Yes**

**200 sample**
```json
{
  "available": true,
  "connected": true,
  "lastState": 0,
  "lastError": "",
  "lastConnectMs": 123456,
  "reconnectAttempts": 0,
  "lastTopic": "trxr4kdz/canvas/canvas-1/draw",
  "lastPayloadLen": 97,
  "lastMessageMs": 123460
}
```

---

## 3.3 Alarm

### GET `/api/alarm`  
Auth: **Yes**

**200 sample**
```json
{
  "enabled": true,
  "hour": 7,
  "minute": 0,
  "daysMask": 62,
  "flashMode": "solid",
  "color": "#ff0000",
  "volume": 80,
  "tone": "beep",
  "snoozeMinutes": 9,
  "timeoutMinutes": 10,
  "alarming": false,
  "snoozed": false,
  "snoozeUntilMs": 0
}
```

### PUT `/api/alarm`  
Auth: **Yes**

**Request schema (all optional):**
```json
{
  "enabled": true,
  "hour": 7,
  "minute": 0,
  "daysMask": 62,
  "flashMode": "solid|pulse|off",
  "color": "#RRGGBB",
  "volume": 80,
  "tone": "beep|chime|pulse",
  "snoozeMinutes": 9,
  "timeoutMinutes": 10
}
```

**Responses**
- `200` `{"status":"ok"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`

### POST `/api/alarm/preview`  
Auth: **Yes**

**Request schema**
```json
{
  "tone": "beep|chime|pulse",
  "volume": 80
}
```

**Responses**
- `200` `{"status":"ok"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `503` `{"error":"alarm unavailable"}`

### POST `/api/alarm/trigger`  
Auth: **Yes**

**Responses**
- `200` `{"status":"ok"}`
- `503` `{"error":"alarm unavailable"}`

---

## 3.4 Screen Management

### Screen object (summary)
```json
{
  "id": "clock-1",
  "type": "clock",
  "enabled": true,
  "duration": 10,
  "order": 0,
  "active": true
}
```

### Screen object (detail)
```json
{
  "id": "canvas-1",
  "type": "canvas",
  "enabled": true,
  "duration": 10,
  "order": 2,
  "active": false,
  "settings": {}
}
```

### GET `/api/screens`  
Auth: **No**

**200 sample**
```json
{
  "cycling": true,
  "activeScreen": "clock-1",
  "screens": [
    {"id":"clock-1","type":"clock","enabled":true,"duration":10,"order":0,"active":true}
  ]
}
```

### GET `/api/screens/{id}`  
Auth: **No**

**Responses**
- `200` detailed screen object
- `404` `{"error":"screen not found"}`

### POST `/api/screens`  
Auth: **Yes**

Create a screen.

**Request schema**
```json
{
  "type": "clock|binary_clock|text_ticker|canvas|weather|battery",
  "id": "optional string",
  "enabled": true,
  "duration": 10,
  "settings": {}
}
```

**Responses**
- `201` created screen object
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `400` `{"error":"invalid type"}`
- `500` `{"error":"max screens reached"}`

### PUT `/api/screens/{id}`  
Auth: **Yes**

Patch fields on existing screen.

**Request schema (any subset):**
```json
{
  "enabled": true,
  "duration": 10,
  "settings": {}
}
```

**Responses**
- `200` updated screen object
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `404` `{"error":"screen not found"}`

### DELETE `/api/screens/{id}`  
Auth: **Yes**

**Responses**
- `200` `{"status":"deleted"}`
- `404` `{"error":"screen not found"}`

### POST `/api/screens/{id}/enable`  
Auth: **Yes**

- `200` `{"status":"enabled"}`
- `404` `{"error":"screen not found"}`

### POST `/api/screens/{id}/disable`  
Auth: **Yes**

- `200` `{"status":"disabled"}`
- `404` `{"error":"screen not found"}`

### POST `/api/screens/reorder`  
Auth: **Yes**

**Request schema**
```json
{
  "order": ["id1", "id2", "id3"]
}
```

**Responses**
- `200` `{"status":"reordered"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `400` `{"error":"order array required"}`

### POST `/api/screens/next`  
Auth: **Yes**

**Responses**
- `200` `{"status":"ok"}`

### PUT `/api/screens/cycling`  
Auth: **Yes**

**Request schema**
```json
{ "enabled": true }
```

**Responses**
- `200` `{"cycling":true}` or `{"cycling":false}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`

### GET `/api/screens/defaults`  
Auth: **No**

**200 sample**
```json
{
  "use24h": false,
  "timezone": "MST7MDT,M3.2.0,M11.1.0",
  "font": 1,
  "defaultColor": "#FFFFFF"
}
```

### PUT `/api/screens/defaults`  
Auth: **Yes**

**Request schema (all optional):**
```json
{
  "use24h": false,
  "timezone": "UTC0",
  "font": 1,
  "defaultColor": "#RRGGBB"
}
```

**Responses**
- `200` updated defaults object
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`

---

## 3.5 Canvas API

`{id}` must refer to a screen with `type: "canvas"`.

### POST `/api/canvas/{id}`  
Auth: **Yes**

Push full frame payload.

**Request schema**
```json
{
  "rgb": "hex string of raw RGB bytes (32*8*3 bytes => 768 bytes => 1536 hex chars)"
}
```

**Responses**
- `200` `{"status":"ok"}`
- `400` `{"error":"no body"}`
- `404` `{"error":"canvas screen not found"}`

### POST `/api/canvas/{id}/draw`  
Auth: **Yes**

AWTRIX-style drawing commands.

**Request schema (any subset):**
```json
{
  "cl": true,
  "df": [[x, y, w, h, "#RRGGBB"]],
  "dl": [[x0, y0, x1, y1, "#RRGGBB"]],
  "dp": [[x, y, "#RRGGBB"]]
}
```

**Responses**
- `200` `{"status":"ok"}`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `404` `{"error":"canvas screen not found"}`

### GET `/api/canvas/{id}/frame`  
Auth: **Yes**

Returns current composed frame (effect layer + canvas layer).

**200 sample**
```json
{
  "width": 32,
  "height": 8,
  "pixels": ["#000000", "#00FF00", "..."]
}
```

**Errors**
- `404` `{"error":"canvas screen not found"}`

---

## 3.6 Preview / Icons / Overtake

### POST `/api/preview`  
Auth: **Yes**

Render a temporary preview frame for a screen type.

**Request schema**
```json
{
  "type": "clock|binary_clock|text_ticker|weather|battery",
  "settings": {},
  "nowMs": 0
}
```

**Responses**
- `200` `{ "width":32, "height":8, "pixels":["#RRGGBB", ...] }`
- `400` `{"error":"no body"}`
- `400` `{"error":"invalid json"}`
- `400` `{"error":"type required"}`
- `400` `{"error":"unsupported preview type"}`

### GET `/api/lametric/search?q=<query>&limit=<1..50>`  
Auth: **Yes**

**200 sample**
```json
{
  "results": [
    {"id":389,"title":"battery","type":"picture","url":"...","thumb":"..."}
  ]
}
```

Errors:
- `500` `{"error":"http begin failed"}`
- `502` `{"error":"lametric upstream failed"}`
- `500` `{"error":"lametric parse failed"}`

### GET `/api/lametric/icon?id=<id>&fmt=gif|png`  
Auth: **Yes**

Returns binary image data.

Responses:
- `200` image stream (`image/gif` or `image/png`)
- `400` `{"error":"invalid id"}`
- `500` `{"error":"http begin failed"}`
- `502` `{"error":"lametric icon fetch failed"}`

### POST `/api/overtake/mute`  
Auth: **Yes**

- `200` `{"status":"muted"}`
- `503` `{"error":"overtake unavailable"}`

### POST `/api/overtake/clear`  
Auth: **Yes**

- `200` `{"status":"cleared"}`
- `503` `{"error":"overtake unavailable"}`

---

## 4) Quick API Smoke Examples

```bash
# Status (no auth)
curl http://<ip>/api/status

# Read config (auth)
curl -H "X-API-Key: <token>" http://<ip>/api/config

# Pause screen cycling
curl -X PUT http://<ip>/api/screens/cycling \
  -H "X-API-Key: <token>" \
  -H "Content-Type: application/json" \
  -d '{"enabled":false}'

# Draw one pixel on canvas-1
curl -X POST http://<ip>/api/canvas/canvas-1/draw \
  -H "X-API-Key: <token>" \
  -H "Content-Type: application/json" \
  -d '{"dp":[[0,0,"#00FF00"]]}'
```

---

## 5) Hardware Reference

- MCU: ESP32-D0WD
- Matrix: 32x8 WS2812B
- Buzzer: GPIO 15
- Buttons: GPIO 26 / 14 / 27
- LDR: GPIO 35
- Battery sense: GPIO 34
- I2C: SCL 22, SDA 21

---

## 6) Notes

- Version is currently reported as `0.1.0` by `/api/status`.
- Some UI-only behavior (like export/import helper flow) composes multiple API calls; endpoints above are the underlying contract.
- If auth is enabled and requests fail, confirm `X-API-Key` token first.
