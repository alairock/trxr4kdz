#!/usr/bin/env bash
set -euo pipefail

# Common API smoke checks for the new firmware line.
#
# Usage:
#   scripts/api_smoke_check.sh --host 192.168.0.48 [--token "$API_TOKEN"] [--mutating]
#   BASE_URL=http://trxr4kdz.local scripts/api_smoke_check.sh
#
# Defaults:
# - Non-mutating mode only validates read endpoints + auth posture for a protected route.
# - --mutating adds create/delete + cycling pause/resume restore checks.

BASE_URL="${BASE_URL:-}"
HOST=""
API_TOKEN="${API_TOKEN:-}"
MUTATING=0

usage() {
  cat <<'EOF'
Common API smoke checks for trxr4kdz firmware.

Options:
  --host <ip-or-hostname>  Device host (builds BASE_URL as http://<host>)
  --base-url <url>         Full base URL (e.g. http://192.168.0.48)
  --token <token>          API token for protected endpoints
  --mutating               Run create/delete + cycling state restore checks
  -h, --help               Show this help

Environment:
  API_TOKEN, BASE_URL
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --host)
      HOST="$2"; shift 2 ;;
    --base-url)
      BASE_URL="$2"; shift 2 ;;
    --token)
      API_TOKEN="$2"; shift 2 ;;
    --mutating)
      MUTATING=1; shift ;;
    -h|--help)
      usage; exit 0 ;;
    *)
      echo "Unknown argument: $1" >&2
      usage
      exit 2 ;;
  esac
done

if [[ -z "$BASE_URL" ]]; then
  if [[ -n "$HOST" ]]; then
    BASE_URL="http://$HOST"
  else
    BASE_URL="http://trxr4kdz.local"
  fi
fi

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || { echo "missing required command: $1" >&2; exit 2; }
}

need_cmd curl
need_cmd python3

HDR_JSON=(-H "Accept: application/json" -H "Content-Type: application/json")
HDR_AUTH=()
if [[ -n "$API_TOKEN" ]]; then
  HDR_AUTH=(-H "X-API-Key: $API_TOKEN")
fi

CURL_BODY=""
CURL_STATUS=""

call_json() {
  local method="$1"; shift
  local path="$1"; shift
  local data="${1:-}"
  local out

  if [[ -n "$data" ]]; then
    out=$(curl -sS -X "$method" "${HDR_JSON[@]}" "${HDR_AUTH[@]}" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path" --data "$data")
  else
    out=$(curl -sS -X "$method" "${HDR_JSON[@]}" "${HDR_AUTH[@]}" -w $'\n__HTTP_STATUS__:%{http_code}' "$BASE_URL$path")
  fi

  CURL_BODY="${out%$'\n'__HTTP_STATUS__:*}"
  CURL_STATUS="${out##*__HTTP_STATUS__:}"
}

assert_http_any() {
  local got="$1"; shift
  local ok=" $* "
  [[ "$ok" == *" $got "* ]] || {
    echo "FAIL: HTTP $got not in expected set: $*"
    return 1
  }
}

json_query() {
  local expr="$1"
  JSON_INPUT="$CURL_BODY" python3 - "$expr" <<'PY'
import json, os, sys
expr=sys.argv[1]
obj=json.loads(os.environ['JSON_INPUT'])
cur=obj
for part in expr.split('.'):
    if not part:
        continue
    cur = cur[int(part)] if part.isdigit() else cur[part]
if isinstance(cur, bool):
    print('true' if cur else 'false')
elif cur is None:
    print('null')
else:
    print(cur)
PY
}

pass() { echo "PASS: $*"; }
fail() { echo "FAIL: $*"; exit 1; }

echo "=== trxr4kdz common API smoke check ==="
echo "Base URL: $BASE_URL"
echo "Token: $([[ -n "$API_TOKEN" ]] && echo provided || echo not-provided)"
echo "Mutating checks: $([[ "$MUTATING" == "1" ]] && echo enabled || echo disabled)"

# 1) Core open/read endpoints
call_json GET /api/status
assert_http_any "$CURL_STATUS" 200 || fail "/api/status request failed"
status_version=$(json_query version)
status_heap=$(json_query heap)
pass "GET /api/status -> version=$status_version heap=$status_heap"

call_json GET /api/screens
assert_http_any "$CURL_STATUS" 200 || fail "/api/screens request failed"
screens_count=$(JSON_INPUT="$CURL_BODY" python3 - <<'PY'
import json, os
obj=json.loads(os.environ['JSON_INPUT'])
print(len(obj.get('screens', [])))
PY
)
cycling=$(json_query cycling)
pass "GET /api/screens -> screens=$screens_count cycling=$cycling"

call_json GET /api/screens/defaults
assert_http_any "$CURL_STATUS" 200 || fail "/api/screens/defaults request failed"
def_use24h=$(json_query use24h)
def_tz=$(json_query timezone)
pass "GET /api/screens/defaults -> use24h=$def_use24h timezone=$def_tz"

call_json GET /api/weather/debug
assert_http_any "$CURL_STATUS" 200 || fail "/api/weather/debug request failed"
weather_online=$(json_query online)
pass "GET /api/weather/debug -> online=$weather_online"

# 2) Protected route behavior check (should be 200 with token or 401/403 without, depending on config)
call_json GET /api/config
assert_http_any "$CURL_STATUS" 200 401 403 || fail "unexpected status from /api/config"
if [[ "$CURL_STATUS" == "200" ]]; then
  token_set=$(json_query api_token_set)
  pass "GET /api/config allowed (api_token_set=$token_set)"
else
  pass "GET /api/config denied without/invalid auth as expected (HTTP $CURL_STATUS)"
fi

if [[ "$MUTATING" == "1" ]]; then
  # Capture baseline for restore
  call_json GET /api/screens
  assert_http_any "$CURL_STATUS" 200 || fail "failed reading screens before mutating checks"
  initial_cycling=$(json_query cycling)

  # 3) Create + delete throwaway screen to verify write path
  call_json POST /api/screens '{"type":"clock","duration":5}'
  assert_http_any "$CURL_STATUS" 201 401 403 || fail "unexpected status from POST /api/screens"

  if [[ "$CURL_STATUS" == "201" ]]; then
    tmp_id=$(json_query id)
    pass "POST /api/screens created $tmp_id"

    call_json DELETE "/api/screens/$tmp_id"
    assert_http_any "$CURL_STATUS" 200 || fail "DELETE /api/screens/$tmp_id failed"
    pass "DELETE /api/screens/$tmp_id"
  else
    pass "POST /api/screens blocked without auth (HTTP $CURL_STATUS); skipping create/delete"
  fi

  # 4) Cycling toggle + restore if write authorized
  call_json PUT /api/screens/cycling '{"enabled":false}'
  assert_http_any "$CURL_STATUS" 200 401 403 || fail "unexpected status from PUT /api/screens/cycling"
  if [[ "$CURL_STATUS" == "200" ]]; then
    pass "PUT /api/screens/cycling false"

    call_json PUT /api/screens/cycling "{\"enabled\":$initial_cycling}"
    assert_http_any "$CURL_STATUS" 200 || fail "failed restoring cycling state"
    pass "Restored /api/screens/cycling to $initial_cycling"
  else
    pass "PUT /api/screens/cycling blocked without auth (HTTP $CURL_STATUS); no state changed"
  fi
fi

echo "SUCCESS: common API smoke checks completed"
