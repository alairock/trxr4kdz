#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${BASE_URL:-http://trxr4kdz.local}"
API_TOKEN="${API_TOKEN:-}"

hdr=(-H "Content-Type: application/json")
if [[ -n "$API_TOKEN" ]]; then
  hdr+=(-H "X-API-Key: $API_TOKEN")
fi

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "missing required command: $1" >&2; exit 2; }
}

need_cmd curl
need_cmd python3

curl_json() {
  local method="$1"; shift
  local url="$1"; shift
  curl -fsS -X "$method" "${hdr[@]}" "$url" "$@"
}

screens_json="$(curl_json GET "$BASE_URL/api/screens")"
weather_id="$(python3 -c 'import json,sys; d=json.load(sys.stdin); w=[s["id"] for s in d.get("screens",[]) if s.get("type")=="weather"]; print(w[0] if w else "")' <<<"$screens_json")"
if [[ -z "$weather_id" ]]; then
  echo "FAIL: no weather screen found in /api/screens"
  exit 1
fi

echo "Weather screen id: $weather_id"

orig_screen="$(curl_json GET "$BASE_URL/api/screens/$weather_id")"
orig_settings="$(python3 -c 'import json,sys; print(json.dumps(json.load(sys.stdin).get("settings",{})))' <<<"$orig_screen")"

payload='{"settings":{"zipCode":"80302","font":1,"flipInterval":4,"tempIconId":20275,"humidityIconId":18191,"colors":{"temp":"#FFAA00","humidity":"#00CCFF","value":"#FFFFFF"}}}'
updated="$(curl_json PUT "$BASE_URL/api/screens/$weather_id" --data "$payload")"

python3 - "$updated" <<'PY'
import json,sys
obj=json.loads(sys.argv[1])
s=obj.get("settings",{})
assert s.get("zipCode") == "80302", f"zipCode mismatch: {s.get('zipCode')}"
assert s.get("flipInterval") == 4, f"flipInterval mismatch: {s.get('flipInterval')}"
assert s.get("tempIconId") == 20275
assert s.get("humidityIconId") == 18191
colors=s.get("colors",{})
for k in ("temp","humidity","value"):
    if colors.get(k) is not None:
        assert isinstance(colors.get(k), str) and colors[k].startswith("#") and len(colors[k]) == 7, f"invalid colors.{k}: {colors.get(k)}"
print("PASS: /api/screens weather settings round-trip looks sane")
PY

weather_debug="$(curl_json GET "$BASE_URL/api/weather/debug")"
python3 - "$weather_debug" <<'PY'
import json,sys
obj=json.loads(sys.argv[1])
required=["hasData","online","tempF","humidity","zipCode","hasZipCode","hasGeocode","needsGeocode","lat","lon","lastError","networkState","requestInFlight","inFlightAgeMs","lastSuccessMs","lastFailureMs","consecutiveFailures","backoffMs","lastOperationLatencyMs","loopUpdateUs","loopUpdateUsMax"]
missing=[k for k in required if k not in obj]
if missing:
    raise SystemExit(f"FAIL: /api/weather/debug missing keys: {missing}")
print("PASS: /api/weather/debug shape looks sane")
PY

restore_payload="$(python3 - <<'PY' "$orig_settings"
import json,sys
settings=json.loads(sys.argv[1])
print(json.dumps({"settings": settings}))
PY
)"
curl_json PUT "$BASE_URL/api/screens/$weather_id" --data "$restore_payload" >/dev/null

echo "PASS: weather sanity check complete"
