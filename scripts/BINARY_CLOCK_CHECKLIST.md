# Binary Clock Regression Checklist

Scope: `src/screens/BinaryClockScreen.*` on the new firmware line.

## 1) Local logic regression (host)

```bash
python3 scripts/check_binary_clock.py
```

Expected: all PASS.

Covers:
- 12h conversion edge cases (`00:xx -> 12:xx`, `12:xx -> 12:xx`)
- 24h mode hours unchanged
- PM indicator source (`h24 >= 12`)
- HH:MM:SS digit splitting

## 2) On-device API mode sanity (optional but recommended)

Assume device is reachable at `$IP`.

### Enable binary clock screen
```bash
curl -s http://$IP/api/screens | jq
```
Confirm a `type: "binary_clock"` screen exists and is enabled.

### Verify mode persistence (settings echo)
```bash
curl -s -X PUT http://$IP/api/screens/<binary-id> \
  -H 'Content-Type: application/json' \
  -d '{"settings":{"use24h":true,"indicatorMode":"rainbow_static","onMode":"purple"}}' | jq

curl -s http://$IP/api/screens/<binary-id> | jq
```
Expected settings:
- `use24h: true`
- `indicatorMode: "rainbow_static"`
- `onMode: "purple"`

### Visual quick checks
- Set current time near `00:xx`, confirm hour shows `00` in 24h mode and `12` in 12h mode.
- Set current time near `12:xx`, confirm PM indicator is ON.
- Set current time near `09:08:07` and confirm 6 binary columns encode `09 08 07`.

## Supported mode strings

From firmware parser (`modeFromString`):
- `white`, `red`, `green`, `blue`, `pink`, `purple`
- static rainbow aliases: `rainbow`, `rainbow_static`, `static_rainbow`
- animated rainbow aliases: `rainbow_animated`, `animated_rainbow`
