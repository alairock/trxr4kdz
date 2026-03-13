# TRXR4KDZ — Custom Firmware for Ulanzi TC001

Custom ESP32 firmware for the **Ulanzi TC001** desktop LED matrix (32x8 WS2812B pixels).

Current shipped firmware line includes:
- Modular screen system with persisted ordering
- Auto-cycling with API + button play/pause controls
- Screen defaults API (`/api/screens/defaults`) persisted in `screens.json`
- Canvas push/draw APIs
- OTA update endpoint
- NTP time sync + weather service integration

## Hardware

- **MCU**: ESP32-D0WD (240MHz dual-core, 320KB RAM, 4MB flash)
- **Display**: 32x8 WS2812B LED matrix (zigzag row layout)
- **Buzzer**: Piezo on GPIO 15 (strapping pin — may whine during serial flash)
- **Buttons**: Up (GPIO 26), Down (GPIO 14), Select (GPIO 27)
- **Sensors**: LDR (GPIO 35), Battery (GPIO 34)
- **I2C**: SCL (GPIO 22), SDA (GPIO 21)

## Building & Flashing

Requires [PlatformIO](https://platformio.org/).

```bash
# Build
pio run

# Flash firmware via serial
pio run -t upload

# Flash filesystem (LittleFS) via serial
pio run -t uploadfs

# OTA firmware update (device must be on network)
curl -X POST http://<device-ip>/api/update -F "firmware=@.pio/build/ulanzi/firmware.bin"

# Serial monitor
pio device monitor
```

## Boot Sequence (current behavior)

1. GPIO 15 forced LOW (silence buzzer), serial + config init
2. Display starts with `BOOT`
3. Wi-Fi starts (AP always, STA if configured)
4. Weather service + HTTP server start
5. ~2s: buzzer beep + IP scroll
6. After IP scroll: screen manager starts and cycling begins

## Screen System

### Supported screen types

- `clock`
- `binary_clock`
- `text_ticker`
- `canvas`
- `weather`

### Common screen fields

Each screen object uses:
- `id` (string)
- `type` (string)
- `enabled` (bool)
- `duration` (seconds)
- `order` (int)
- `settings` (object, type-specific)

`duration: 0` means no auto-advance while that screen is active.

### Global screen state

Persisted in `/screens.json`:
- `cycling` (global play/pause state)
- `defaults` (applied when creating new screens)
- `screens` array

### Defaults API behavior (important)

`/api/screens/defaults` controls global defaults:
- `use24h` (bool)
- `timezone` (POSIX TZ string)
- `font` (uint8)
- `defaultColor` (`#RRGGBB`)

Semantics:
- Updating defaults affects **newly created** screens.
- Existing screens keep their current settings unless explicitly updated.
- Defaults are persisted to `/screens.json` and survive reboot.

### Physical button controls (screen playback)

- **Select double-press**: toggle cycling play/pause
- **Up/Down short press**: previous/next screen **only when paused**
- **Select long-press**: open settings menu

## REST API

Base URL: `http://<device-ip>`

### Authentication

If `api_token` is set in config:
- Send `X-API-Key: <token>` header
- Or `?token=<token>` query param

If no token is configured, protected endpoints are open.

### Device endpoints

| Method | Path | Auth | Notes |
|---|---|---|---|
| GET | `/api/status` | No | version, hostname, AP/STA IP, RSSI, heap, uptime |
| GET | `/api/config` | Yes | returns masked passwords + `api_token_set` |
| POST | `/api/config` | Yes | update `brightness`, `hostname`, `ap_password`, `api_token` |
| POST | `/api/wifi` | Yes | body: `ssid` required, optional `password`; saves + restarts |
| POST | `/api/update` | Yes | multipart form field `firmware`; OTA then restart |
| GET | `/api/weather/debug` | No | weather service diagnostics |

### Screen endpoints

| Method | Path | Auth | Behavior |
|---|---|---|---|
| GET | `/api/screens` | No | summary list + `cycling` + `activeScreen` |
| GET | `/api/screens/{id}` | No | full screen including `settings` |
| POST | `/api/screens` | Yes | create screen (`type` required) |
| PUT | `/api/screens/{id}` | Yes | patch `enabled`, `duration`, `settings` |
| DELETE | `/api/screens/{id}` | Yes | delete screen |
| POST | `/api/screens/{id}/enable` | Yes | set `enabled=true` |
| POST | `/api/screens/{id}/disable` | Yes | set `enabled=false` |
| POST | `/api/screens/reorder` | Yes | body `{ "order": ["id1","id2",...] }` |
| POST | `/api/screens/next` | Yes | immediate next enabled screen |
| PUT | `/api/screens/cycling` | Yes | body `{ "enabled": true/false }` |
| GET | `/api/screens/defaults` | No | read defaults |
| PUT | `/api/screens/defaults` | Yes | update persisted defaults |

#### Screen control semantics (API)

- `PUT /api/screens/cycling {"enabled":false}` pauses auto-rotation.
- While paused, active screen remains until:
  - `POST /api/screens/next`, or
  - physical Up/Down short press.
- `POST /api/screens/next` advances to next enabled screen and resets switch timer.
- Reorder changes cycle order by writing each screen’s `order` then sorting.

### Canvas endpoints

| Method | Path | Auth | Behavior |
|---|---|---|---|
| POST | `/api/canvas/{id}` | Yes | body `{ "rgb": "<hex RGB bytes>" }` |
| POST | `/api/canvas/{id}/draw` | Yes | AWTRIX-style draw commands |

Draw payload keys:
- `cl`: clear framebuffer
- `df`: filled rects `[[x,y,w,h,"#RRGGBB"], ...]`
- `dl`: lines `[[x0,y0,x1,y1,"#RRGGBB"], ...]`
- `dp`: pixels `[[x,y,"#RRGGBB"], ...]`

## API examples

```bash
# Status
curl http://192.168.0.48/api/status

# Read defaults
curl http://192.168.0.48/api/screens/defaults

# Update defaults (auth required when token set)
curl -X PUT http://192.168.0.48/api/screens/defaults \
  -H "Content-Type: application/json" \
  -H "X-API-Key: <token>" \
  -d '{"use24h":true,"timezone":"UTC0","font":3,"defaultColor":"#12AB34"}'

# Create a clock that inherits current defaults
curl -X POST http://192.168.0.48/api/screens \
  -H "Content-Type: application/json" \
  -H "X-API-Key: <token>" \
  -d '{"type":"clock"}'

# Pause cycling
curl -X PUT http://192.168.0.48/api/screens/cycling \
  -H "Content-Type: application/json" \
  -H "X-API-Key: <token>" \
  -d '{"enabled":false}'

# Step once while paused
curl -X POST http://192.168.0.48/api/screens/next -H "X-API-Key: <token>"

# Reorder screens
curl -X POST http://192.168.0.48/api/screens/reorder \
  -H "Content-Type: application/json" \
  -H "X-API-Key: <token>" \
  -d '{"order":["clock-1","weather-1","canvas-1"]}'
```

## API smoke test (common calls)

For a fast confidence pass against common endpoints:

```bash
# Read-only checks
scripts/api_smoke_check.sh --host 192.168.0.48

# Include auth/write-path checks when token is set
scripts/api_smoke_check.sh --host 192.168.0.48 --token "$API_TOKEN" --mutating
```

Checklist and evidence capture flow: `scripts/API_SMOKE_CHECKLIST.md`.

## Notes

- `/api/screens/defaults` is intentionally read-open and write-protected.
- Some enum values exist in code (`CRYPTO`, `CUSTOM`) but are not creatable via current API and are not part of shipped behavior.
- `FIRMWARE_VERSION` is currently static (`0.1.0`) in `WebServerManager.h`.

## Dependencies

| Library | Version | Purpose |
|---|---|---|
| FastLED | ^3.7 | WS2812B LED driver |
| FastLED NeoMatrix | ^1.2 | Matrix layout mapping |
| Adafruit GFX | ^1.11 | Graphics + font rendering |
| ArduinoJson | ^7.0 | JSON parsing/serialization |
| LittleFS | built-in | Persistent config/screen storage |
| WebServer | built-in | HTTP server |
| WiFi | built-in | Connectivity |
| Update | built-in | OTA firmware updates |
