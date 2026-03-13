#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${1:-http://192.168.0.48}"
API_KEY="${API_KEY:-}"
LOG_DIR="${LOG_DIR:-./artifacts}"
SKIP_REBOOT="${SKIP_REBOOT:-0}"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/boot_wifi_captive_validation_$(date +%Y%m%d_%H%M%S).log"

exec > >(tee "$LOG_FILE") 2>&1

header_args=()
if [[ -n "$API_KEY" ]]; then
  header_args=(-H "X-API-Key: $API_KEY")
fi

CURL_BODY=""
CURL_STATUS=""

call_api() {
  local method="$1"
  local path="$2"
  local data="${3:-}"
  local out

  echo
  echo ">>> curl -sS -X $method ${header_args[*]} '$BASE_URL$path' ${data:+-H 'Content-Type: application/json' -d '$data'}"

  if [[ -n "$data" ]]; then
    out=$(curl -sS -X "$method" "${header_args[@]}" -H "Content-Type: application/json" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path" -d "$data")
  else
    out=$(curl -sS -X "$method" "${header_args[@]}" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path")
  fi

  CURL_BODY="${out%$'\n'__HTTP_STATUS__:*}"
  CURL_STATUS="${out##*__HTTP_STATUS__:}"

  echo "$CURL_BODY"
  echo "HTTP $CURL_STATUS"
}

json_get() {
  local expr="$1"
  local json="$2"
  JSON_INPUT="$json" python3 - "$expr" <<'PY'
import json,sys,os
expr=sys.argv[1]
obj=json.loads(os.environ['JSON_INPUT'])
cur=obj
for part in expr.split('.'):
    if not part:
        continue
    cur = cur[int(part)] if part.isdigit() else cur[part]
print('true' if cur is True else 'false' if cur is False else cur)
PY
}

assert_status() {
  local expected="$1"
  [[ "$CURL_STATUS" == "$expected" ]] || { echo "ASSERT FAIL: expected HTTP $expected got $CURL_STATUS"; exit 1; }
}

wait_for_device() {
  echo
  echo ">>> waiting for device to return after reboot"
  for i in {1..90}; do
    if curl -sS --connect-timeout 1 "$BASE_URL/api/status" >/dev/null 2>&1; then
      echo "Device reachable after ${i}s"
      return 0
    fi
    sleep 1
  done
  echo "ASSERT FAIL: device did not return within 90s"
  exit 1
}

echo "=== Boot + Wi-Fi join/captive validation ==="
echo "Base URL: $BASE_URL"

call_api GET /api/status
assert_status 200
pre_status="$CURL_BODY"
pre_sta=$(json_get sta_connected "$pre_status")
pre_sta_ip=$(json_get sta_ip "$pre_status")
pre_ap_ip=$(json_get ap_ip "$pre_status")
pre_uptime=$(json_get uptime_s "$pre_status")

[[ "$pre_sta" == "true" ]] || { echo "ASSERT FAIL: expected STA connected before reboot"; exit 1; }
[[ "$pre_sta_ip" != "0.0.0.0" ]] || { echo "ASSERT FAIL: invalid STA IP before reboot"; exit 1; }
[[ "$pre_ap_ip" != "0.0.0.0" ]] || { echo "ASSERT FAIL: invalid AP IP before reboot"; exit 1; }
echo "ASSERT PASS: pre-boot status shows STA joined ($pre_sta_ip) + AP active ($pre_ap_ip), uptime=${pre_uptime}s"

for p in / /generate_204 /hotspot-detect.html /fwlink /connecttest.txt ; do
  call_api GET "$p"
  assert_status 200
  if ! grep -q "trxr4kdz WiFi Setup" <<<"$CURL_BODY"; then
    echo "ASSERT FAIL: captive portal signature missing on $p"
    exit 1
  fi
  echo "ASSERT PASS: captive endpoint $p returned setup page"
done

if [[ "$SKIP_REBOOT" != "1" ]]; then
  if [[ ! -f .pio/build/ulanzi/firmware.bin ]]; then
    echo "ASSERT FAIL: firmware binary missing at .pio/build/ulanzi/firmware.bin (set SKIP_REBOOT=1 to skip OTA reboot step)"
    exit 1
  fi

  echo
  echo ">>> curl -sS -X POST ${header_args[*]} '$BASE_URL/api/update' -F firmware=@.pio/build/ulanzi/firmware.bin"
  ota_out=$(curl -sS -X POST "${header_args[@]}" "$BASE_URL/api/update" -F "firmware=@.pio/build/ulanzi/firmware.bin" -w $'\n__HTTP_STATUS__:%{http_code}')
  CURL_BODY="${ota_out%$'\n'__HTTP_STATUS__:*}"
  CURL_STATUS="${ota_out##*__HTTP_STATUS__:}"
  echo "$CURL_BODY"
  echo "HTTP $CURL_STATUS"
  assert_status 200

  wait_for_device
fi

call_api GET /api/status
assert_status 200
post_status="$CURL_BODY"
post_sta=$(json_get sta_connected "$post_status")
post_sta_ip=$(json_get sta_ip "$post_status")
post_ap_ip=$(json_get ap_ip "$post_status")
post_uptime=$(json_get uptime_s "$post_status")

[[ "$post_sta" == "true" ]] || { echo "ASSERT FAIL: expected STA connected after reboot"; exit 1; }
[[ "$post_sta_ip" == "$pre_sta_ip" ]] || { echo "ASSERT FAIL: STA IP changed unexpectedly pre=$pre_sta_ip post=$post_sta_ip"; exit 1; }
[[ "$post_ap_ip" == "$pre_ap_ip" ]] || { echo "ASSERT FAIL: AP IP changed unexpectedly pre=$pre_ap_ip post=$post_ap_ip"; exit 1; }
if [[ "$SKIP_REBOOT" != "1" ]]; then
  [[ "$post_uptime" -lt 180 ]] || { echo "ASSERT FAIL: uptime (${post_uptime}s) does not look freshly rebooted"; exit 1; }
fi

echo "ASSERT PASS: post-boot status confirms Wi-Fi rejoin + AP captive availability (uptime=${post_uptime}s)"

echo "SUCCESS: boot Wi-Fi join + captive portal behavior validated on reachable boot path"
echo "NOTE: AP-only fallback on bad credentials remains a manual/physical-network check (see scripts/BOOT_WIFI_FALLBACK_CHECKLIST.md)."
echo "Evidence log: $LOG_FILE"
