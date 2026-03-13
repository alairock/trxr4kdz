# NEXT_SPRINT — trxr4kdz

## Sprint goal
Stabilize and harden the new firmware line while preserving iteration speed.

## P0 (must do)

- [x] Add minimal auth/guardrail for OTA and sensitive API routes
- [x] Validate defaults API end-to-end (create/update/persist/reboot)
- [ ] Add a quick firmware regression pass:
  - [ ] boot + Wi-Fi join/captive fallback
  - [ ] screen cycling play/pause behavior
  - [x] binary clock mode correctness
  - [ ] weather screen render sanity

## P1 (high value)

- [ ] Document current API behavior in README (especially defaults + screen controls)
- [ ] Add a small test script/checklist for common API calls
- [ ] Review blocking network calls and cap worst-case latency paths

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
