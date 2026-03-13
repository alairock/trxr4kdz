# Common API Smoke Checklist

Scope: quick confidence pass for common REST endpoints on the **new firmware line**.

## Script

```bash
cd /home/pi/.openclaw/workspace/trxr4kdz
chmod +x scripts/api_smoke_check.sh
scripts/api_smoke_check.sh --host 192.168.0.48
```

If API token is configured:

```bash
scripts/api_smoke_check.sh --host 192.168.0.48 --token "$API_TOKEN" --mutating
```

## What it validates

Non-mutating mode:
1. `GET /api/status` (device status shape)
2. `GET /api/screens` (screen list + cycling state)
3. `GET /api/screens/defaults` (defaults readability)
4. `GET /api/weather/debug` (weather debug readability)
5. `GET /api/config` auth posture (`200` when allowed, `401/403` when protected)

Mutating mode (`--mutating`):
6. `POST /api/screens` create throwaway clock screen (if authorized)
7. `DELETE /api/screens/{id}` cleanup
8. `PUT /api/screens/cycling` pause + restore original state

The script is safe to run repeatedly; mutating operations restore prior state.

## Evidence capture

```bash
mkdir -p artifacts
scripts/api_smoke_check.sh --host 192.168.0.48 --token "$API_TOKEN" --mutating | tee artifacts/api_smoke_$(date +%Y%m%d_%H%M%S).log
```

Expected final line:

`SUCCESS: common API smoke checks completed`
