# Decisions Log (living, curated)

Capture important technical/product decisions and keep this document current.
This is **not append-only**: refactor, merge, and supersede entries as needed.

## Conventions
- Order: **newest first**.
- Status values: `proposed` | `accepted` | `superseded`.
- When superseding, link to the newer entry in **Refs**.
- Keep entries concise; prefer pointers to code/commits over long prose.

## Entry template

```md
## YYYY-MM-DD — <short title>
- **Status:** proposed | accepted | superseded
- **Context:**
- **Decision:**
- **Consequences:**
- **Refs:** PR/commit/issues/chat
```

---

## Active decisions (current truth)

## 2026-03-17 — Binary clock uses variable per-column bit heights
- **Status:** accepted
- **Context:** Binary clock columns were fixed at 4 rows, leaving unused dots for digits that never use all bits (e.g., hour tens, minute/second tens).
- **Decision:** Render HH:MM:SS columns with per-digit heights: `2 / 4 / 3 / 4 / 3 / 4`.
- **Consequences:** Cleaner visual layout with no dead indicator positions; preserves standard BCD semantics per digit.
- **Refs:** `src/screens/BinaryClockScreen.cpp`, OTA applied to device `192.168.0.48`.

## Superseded / historical

_(Move older/superseded decisions here as they age out of “current truth”.)_
