#!/usr/bin/env bash
set -euo pipefail

BASE_URL="${1:-http://192.168.0.48}"
API_KEY="${API_KEY:-}"
LOG_DIR="${LOG_DIR:-./artifacts}"
mkdir -p "$LOG_DIR"
LOG_FILE="$LOG_DIR/defaults_api_validation_$(date +%Y%m%d_%H%M%S).log"

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

  echo
  echo ">>> curl -sS -X $method ${header_args[*]} -H 'Content-Type: application/json' '$BASE_URL$path' ${data:+-d '$data'}"

  local out
  if [[ -n "$data" ]]; then
    out=$(curl -sS -X "$method" "${header_args[@]}" -H "Content-Type: application/json" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path" -d "$data")
  else
    out=$(curl -sS -X "$method" "${header_args[@]}" -H "Content-Type: application/json" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path")
  fi

  CURL_BODY="${out%$'\n'__HTTP_STATUS__:*}"
  CURL_STATUS="${out##*__HTTP_STATUS__:}"

  echo "$CURL_BODY"
  echo "HTTP $CURL_STATUS"
}

assert_status() {
  local expected="$1"
  if [[ "$CURL_STATUS" != "$expected" ]]; then
    echo "ASSERT FAIL: expected HTTP $expected, got $CURL_STATUS"
    exit 1
  fi
}

json_get() {
  local expr="$1"
  local json="$2"
  JSON_INPUT="$json" python3 - "$expr" <<'PY'
import json,sys,os
expr=sys.argv[1]
obj=json.loads(os.environ["JSON_INPUT"])
cur=obj
for part in expr.split('.'):
    if part=='':
        continue
    if part.isdigit():
        cur=cur[int(part)]
    else:
        cur=cur[part]
if isinstance(cur,bool):
    print('true' if cur else 'false')
elif cur is None:
    print('null')
else:
    print(cur)
PY
}

wait_for_device() {
  echo
  echo ">>> Waiting for device to come back after reboot..."
  for i in {1..60}; do
    if curl -sS --connect-timeout 1 "$BASE_URL/api/status" >/dev/null 2>&1; then
      echo "Device reachable after $i s"
      return 0
    fi
    sleep 1
  done
  echo "ASSERT FAIL: device did not return within 60s"
  exit 1
}

echo "=== Defaults API end-to-end validation ==="
echo "Base URL: $BASE_URL"

# Capture originals
call_api GET /api/screens/defaults
assert_status 200
orig_defaults="$CURL_BODY"
orig_use24h=$(json_get use24h "$orig_defaults")
orig_timezone=$(json_get timezone "$orig_defaults")
orig_font=$(json_get font "$orig_defaults")
orig_color=$(json_get defaultColor "$orig_defaults")

echo "Original defaults: use24h=$orig_use24h timezone=$orig_timezone font=$orig_font defaultColor=$orig_color"

# Update defaults
new_defaults='{"use24h":true,"timezone":"UTC0","font":3,"defaultColor":"#12AB34"}'
call_api PUT /api/screens/defaults "$new_defaults"
assert_status 200

call_api GET /api/screens/defaults
assert_status 200
use24h=$(json_get use24h "$CURL_BODY")
timezone=$(json_get timezone "$CURL_BODY")
font=$(json_get font "$CURL_BODY")
color=$(json_get defaultColor "$CURL_BODY")
[[ "$use24h" == "true" ]] || { echo "ASSERT FAIL: use24h not updated"; exit 1; }
[[ "$timezone" == "UTC0" ]] || { echo "ASSERT FAIL: timezone not updated"; exit 1; }
[[ "$font" == "3" ]] || { echo "ASSERT FAIL: font not updated"; exit 1; }
[[ "$color" == "#12AB34" ]] || { echo "ASSERT FAIL: defaultColor not updated"; exit 1; }
echo "ASSERT PASS: defaults updated"

# Create new screen; validate it inherits updated defaults
call_api POST /api/screens '{"type":"clock"}'
assert_status 201
new_screen_id=$(json_get id "$CURL_BODY")
echo "Created screen: $new_screen_id"

call_api GET "/api/screens/$new_screen_id"
assert_status 200
screen_use24h=$(json_get settings.use24h "$CURL_BODY")
screen_timezone=$(json_get settings.timezone "$CURL_BODY")
screen_font=$(json_get settings.font "$CURL_BODY")
[[ "$screen_use24h" == "true" ]] || { echo "ASSERT FAIL: new screen use24h did not inherit defaults"; exit 1; }
[[ "$screen_timezone" == "UTC0" ]] || { echo "ASSERT FAIL: new screen timezone did not inherit defaults"; exit 1; }
[[ "$screen_font" == "3" ]] || { echo "ASSERT FAIL: new screen font did not inherit defaults"; exit 1; }
echo "ASSERT PASS: create semantics (new clock inherited defaults)"

# Reboot via OTA with current firmware image to validate persisted behavior across reboot
echo
echo ">>> curl -sS -X POST ${header_args[*]} '$BASE_URL/api/update' -F firmware=@.pio/build/ulanzi/firmware.bin"
ota_out=$(curl -sS -X POST "${header_args[@]}" "$BASE_URL/api/update" -F "firmware=@.pio/build/ulanzi/firmware.bin" -w $'\n__HTTP_STATUS__:%{http_code}')
CURL_BODY="${ota_out%$'\n'__HTTP_STATUS__:*}"
CURL_STATUS="${ota_out##*__HTTP_STATUS__:}"
echo "$CURL_BODY"
echo "HTTP $CURL_STATUS"
assert_status 200

wait_for_device

call_api GET /api/screens/defaults
assert_status 200
reboot_use24h=$(json_get use24h "$CURL_BODY")
reboot_timezone=$(json_get timezone "$CURL_BODY")
reboot_font=$(json_get font "$CURL_BODY")
reboot_color=$(json_get defaultColor "$CURL_BODY")
[[ "$reboot_use24h" == "true" ]] || { echo "ASSERT FAIL: use24h not persisted across reboot"; exit 1; }
[[ "$reboot_timezone" == "UTC0" ]] || { echo "ASSERT FAIL: timezone not persisted across reboot"; exit 1; }
[[ "$reboot_font" == "3" ]] || { echo "ASSERT FAIL: font not persisted across reboot"; exit 1; }
[[ "$reboot_color" == "#12AB34" ]] || { echo "ASSERT FAIL: defaultColor not persisted across reboot"; exit 1; }
echo "ASSERT PASS: defaults persisted across reboot"

call_api GET "/api/screens/$new_screen_id"
assert_status 200
post_reboot_use24h=$(json_get settings.use24h "$CURL_BODY")
post_reboot_timezone=$(json_get settings.timezone "$CURL_BODY")
post_reboot_font=$(json_get settings.font "$CURL_BODY")
[[ "$post_reboot_use24h" == "true" ]] || { echo "ASSERT FAIL: screen use24h not persisted"; exit 1; }
[[ "$post_reboot_timezone" == "UTC0" ]] || { echo "ASSERT FAIL: screen timezone not persisted"; exit 1; }
[[ "$post_reboot_font" == "3" ]] || { echo "ASSERT FAIL: screen font not persisted"; exit 1; }
echo "ASSERT PASS: created screen persisted across reboot"

# Cleanup: remove created screen and restore original defaults
call_api DELETE "/api/screens/$new_screen_id"
assert_status 200

restore_payload=$(ORIG_USE24H="$orig_use24h" ORIG_TZ="$orig_timezone" ORIG_FONT="$orig_font" ORIG_COLOR="$orig_color" python3 - <<'PY'
import json, os
obj = {
    "use24h": os.environ["ORIG_USE24H"].lower() == "true",
    "timezone": os.environ["ORIG_TZ"],
    "font": int(os.environ["ORIG_FONT"]),
    "defaultColor": os.environ["ORIG_COLOR"],
}
print(json.dumps(obj, separators=(',', ':')))
PY
)

call_api PUT /api/screens/defaults "$restore_payload"
assert_status 200

call_api GET /api/screens/defaults
assert_status 200
restored_use24h=$(json_get use24h "$CURL_BODY")
restored_timezone=$(json_get timezone "$CURL_BODY")
restored_font=$(json_get font "$CURL_BODY")
restored_color=$(json_get defaultColor "$CURL_BODY")
[[ "$restored_use24h" == "$orig_use24h" ]] || { echo "ASSERT FAIL: restore use24h mismatch"; exit 1; }
[[ "$restored_timezone" == "$orig_timezone" ]] || { echo "ASSERT FAIL: restore timezone mismatch"; exit 1; }
[[ "$restored_font" == "$orig_font" ]] || { echo "ASSERT FAIL: restore font mismatch"; exit 1; }
[[ "$restored_color" == "$orig_color" ]] || { echo "ASSERT FAIL: restore defaultColor mismatch"; exit 1; }

echo "ASSERT PASS: cleanup restored original defaults"
echo "SUCCESS: defaults API create/update/persist/reboot semantics validated"
echo "Evidence log: $LOG_FILE"
