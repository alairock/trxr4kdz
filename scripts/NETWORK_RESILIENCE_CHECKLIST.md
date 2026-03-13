# Network Resilience Validation Checklist

Use this after firmware flash to prove the non-blocking weather/network refactor keeps loop/API responsive.

## Preconditions
- Device reachable at `http://<device-ip>`
- Firmware built from current main
- If API token enabled, export `API_TOKEN`

## 1) Baseline responsiveness

```bash
BASE_URL=http://192.168.0.48 scripts/validate_network_resilience.sh
```

Expected:
- `/api/status` and `/api/weather/debug` p95 latency stays under 400ms
- Script exits with `PASS`

## 2) Degraded-network behavior (requires token)

```bash
BASE_URL=http://192.168.0.48 API_TOKEN=<token> scripts/validate_network_resilience.sh
```

Expected:
- Script temporarily sets weather ZIP to an invalid value
- `/api/weather/debug` shows `consecutiveFailures >= 1`
- `networkState` transitions to `backoff`
- `backoffMs` increases on repeated failures
- `/api/status` remains responsive during failure period
- Original weather settings are restored

## 3) Manual spot checks
- `curl http://192.168.0.48/api/weather/debug` includes:
  - `networkState`, `requestInFlight`, `inFlightAgeMs`
  - `consecutiveFailures`, `backoffMs`, `lastOperationLatencyMs`
  - `loopUpdateUs`, `loopUpdateUsMax`
- Display/button navigation remains responsive while Wi-Fi is flaky/unavailable.
