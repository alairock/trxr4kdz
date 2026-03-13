#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://trxr4kdz.local}"
API_TOKEN="${API_TOKEN:-}"
SAMPLES="${SAMPLES:-20}"

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "missing: $1" >&2; exit 2; }; }
need_cmd curl
need_cmd python3

hdr=()
if [[ -n "$API_TOKEN" ]]; then
  hdr+=(-H "X-API-Key: $API_TOKEN")
fi

curl_json() {
  local method="$1"; shift
  local url="$1"; shift
  curl -fsS -X "$method" "${hdr[@]}" -H "Content-Type: application/json" "$url" "$@"
}

measure_endpoint() {
  local path="$1"
  local out
  out="$(for _ in $(seq 1 "$SAMPLES"); do
    curl -o /dev/null -sS -w "%{time_total}\n" --max-time 1 "$BASE_URL$path"
  done)"
  python3 - "$path" "$out" <<'PY'
import sys,statistics
path=sys.argv[1]
s=[float(x) for x in sys.argv[2].splitlines() if x.strip()]
s.sort()
p95=s[max(0,int(len(s)*0.95)-1)]
print(f"{path}: n={len(s)} min={min(s):.3f}s median={statistics.median(s):.3f}s p95={p95:.3f}s max={max(s):.3f}s")
if p95 > 0.4:
    raise SystemExit(f"FAIL: {path} p95 too high ({p95:.3f}s)")
PY
}

echo "== Baseline API responsiveness =="
measure_endpoint "/api/status"
measure_endpoint "/api/weather/debug"

if [[ -n "$API_TOKEN" ]]; then
  echo "== Degraded network behavior check (invalid ZIP) =="
  screens="$(curl_json GET "$BASE_URL/api/screens")"
  weather_id="$(python3 -c 'import json,sys; d=json.load(sys.stdin); w=[s["id"] for s in d.get("screens",[]) if s.get("type")=="weather"]; print(w[0] if w else "")' <<<"$screens")"
  if [[ -z "$weather_id" ]]; then
    echo "WARN: no weather screen found; skipping degraded behavior check"
  else
    orig="$(curl_json GET "$BASE_URL/api/screens/$weather_id")"
    orig_settings="$(python3 -c 'import json,sys; print(json.dumps(json.load(sys.stdin).get("settings",{})))' <<<"$orig")"

    curl_json PUT "$BASE_URL/api/screens/$weather_id" --data '{"settings":{"zipCode":"00000"}}' >/dev/null || true
    sleep 8

    dbg1="$(curl_json GET "$BASE_URL/api/weather/debug")"
    python3 - "$dbg1" <<'PY'
import json,sys
obj=json.loads(sys.argv[1])
print("weather state after invalid zip:", obj.get("networkState"), "failures=", obj.get("consecutiveFailures"), "backoffMs=", obj.get("backoffMs"), "err=", obj.get("lastError"))
if obj.get("consecutiveFailures",0) < 1:
    raise SystemExit("FAIL: expected at least one weather failure in degraded mode")
PY

    measure_endpoint "/api/status"

    restore_payload="$(python3 - <<'PY' "$orig_settings"
import json,sys
print(json.dumps({"settings": json.loads(sys.argv[1])}))
PY
)"
    curl_json PUT "$BASE_URL/api/screens/$weather_id" --data "$restore_payload" >/dev/null || true
  fi
else
  echo "INFO: API_TOKEN not set, skipping mutating degraded-network check"
fi

echo "PASS: network resilience validation complete"
