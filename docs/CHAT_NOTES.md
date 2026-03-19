# Chat Notes (trxr4kdz-firmware, living)

Purpose: retain high-value chat context while keeping this doc clean and current.
This is **not append-only**: rewrite, merge, and reorganize as needed.

## Conventions
- Keep a short **Rolling summary** at the top (current state, open items, next actions).
- Keep detailed entries in **Chronological notes** (newest first).
- Move durable architecture/product decisions into `docs/DECISIONS.md`.
- Archive stale detail instead of letting this become a raw transcript dump.

## Rolling summary (current truth)
- Binary clock now renders compact per-digit bit heights for HH:MM:SS: `2 / 4 / 3 / 4 / 3 / 4`.
- Firmware with this change was built and OTA-deployed successfully to device `192.168.0.48`.
- Optional future enhancement: runtime/API toggle for compact mode vs full 4-row mode.

## Entry template

```md
## YYYY-MM-DD HH:MM TZ — <topic>
- **Asked:**
- **Decided/Done:**
- **Outcome:**
- **Follow-up:**
```

---

## Chronological notes (newest first)

## 2026-03-17 10:34 MDT — Binary clock column usage
- **Asked:** Show only binary dots actually used per time digit (include seconds).
- **Decided/Done:** Implemented variable bit heights for HH:MM:SS columns: `2 / 4 / 3 / 4 / 3 / 4`.
- **Outcome:** Firmware built and pushed OTA to `192.168.0.48`; device rebooted and came back online.
- **Follow-up:** Optional API/config switch for compact vs full 4-row mode.

## Archive

_(Move older/no-longer-useful detail here or into separate dated files when this grows.)_
