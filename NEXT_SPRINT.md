# NEXT_SPRINT — trxr4kdz

## Sprint goal
Stabilize and harden the new firmware line while preserving iteration speed.

## P0 (must do)

- [x] Add minimal auth/guardrail for OTA and sensitive API routes
- [x] Validate defaults API end-to-end (create/update/persist/reboot)
- [x] Add a quick firmware regression pass:
  - [x] boot + Wi-Fi join/captive fallback _(evidence: `artifacts/boot_wifi_captive_validation_20260313_155707.log`, `scripts/BOOT_WIFI_FALLBACK_CHECKLIST.md`, `src/WiFiManager.cpp` fallback path)_
  - [x] screen cycling play/pause behavior
  - [x] binary clock mode correctness
  - [x] weather screen render sanity

## P1 (high value)

- [x] Document current API behavior in README (especially defaults + screen controls)
- [x] Add a small test script/checklist for common API calls _(added `scripts/api_smoke_check.sh` + `scripts/API_SMOKE_CHECKLIST.md`)_
- [x] Review blocking network calls and cap worst-case latency paths _(WeatherService moved to non-blocking state machine + worker task with timeout/backoff; web restart handlers de-blocked; validation: `scripts/validate_network_resilience.sh`, `scripts/NETWORK_RESILIENCE_CHECKLIST.md`)_

## P2 (cleanup)

- [ ] Remove or clearly mark any unimplemented enum/feature stubs
- [ ] Improve version reporting strategy (tie to commit/tag in status endpoint)
- [ ] Tighten upload/build env docs for cross-machine portability

## Definition of done (this sprint)

- Security baseline added for OTA/API
- Defaults behavior proven stable across reboot
- Docs reflect actual shipped behavior
- No regressions in core screen loop and controls
- Pushed over OTA and confirmed by human (Skyler)
