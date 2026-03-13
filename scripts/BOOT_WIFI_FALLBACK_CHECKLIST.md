# Boot + Wi-Fi Join / Captive Fallback Checklist

Use this with `scripts/validate_boot_wifi_captive.sh` for reproducible coverage.

## Automated (from dev host)

```bash
cd /home/pi/.openclaw/workspace/trxr4kdz
chmod +x scripts/validate_boot_wifi_captive.sh
./scripts/validate_boot_wifi_captive.sh http://192.168.0.48
```

Expected pass criteria:
- `/api/status` reports `sta_connected=true` with valid `sta_ip` and `ap_ip`.
- Captive endpoints (`/`, `/generate_204`, `/hotspot-detect.html`, `/fwlink`, `/connecttest.txt`) all return setup HTML.
- OTA reboot returns and device rejoins STA with AP still active.

## Manual fallback (AP-only) validation

> Needed to truly prove the "bad credentials -> AP fallback" branch.

1. Keep a local copy of current known-good SSID/password.
2. POST intentionally invalid Wi-Fi creds to `/api/wifi` and wait for reboot.
3. Confirm device is no longer reachable at old STA IP (`http://192.168.0.48`).
4. Join `Ulanzi-<last4-mac>` AP from a phone/laptop.
5. Browse to `http://192.168.4.1/` (or any URL) and verify captive page appears.
6. Submit valid Wi-Fi credentials in captive form.
7. Confirm device returns to normal STA IP and `/api/status` shows `sta_connected=true`.
8. Optional: rerun `validate_boot_wifi_captive.sh` to reconfirm normal boot path.

## Evidence to capture

- Saved script log in `artifacts/boot_wifi_captive_validation_*.log`.
- Screenshot/photo of captive page during AP-only fallback.
- One `GET /api/status` JSON before and after fallback recovery.
