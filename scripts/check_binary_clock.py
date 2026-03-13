#!/usr/bin/env python3
"""Quick regression checks for BinaryClockScreen time/mode behavior.

This mirrors the firmware logic in src/screens/BinaryClockScreen.cpp:
- 12h vs 24h hour handling
- PM detection source (h24 >= 12)
- BCD digit splitting HH:MM:SS

Run:
  python3 scripts/check_binary_clock.py
"""

from dataclasses import dataclass
from datetime import datetime


@dataclass
class Case:
    label: str
    iso: str
    use24h: bool
    expected_digits: tuple[int, int, int, int, int, int]
    expected_pm: bool


def firmware_digits(dt: datetime, use24h: bool):
    h24 = dt.hour
    is_pm = h24 >= 12

    h = h24
    if not use24h:
        h = h24 % 12
        if h == 0:
            h = 12

    digits = (
        h // 10,
        h % 10,
        dt.minute // 10,
        dt.minute % 10,
        dt.second // 10,
        dt.second % 10,
    )
    return digits, is_pm


def run():
    cases = [
        Case("midnight-12h", "2026-03-13T00:00:00", False, (1, 2, 0, 0, 0, 0), False),
        Case("midnight-24h", "2026-03-13T00:00:00", True, (0, 0, 0, 0, 0, 0), False),
        Case("noon-12h", "2026-03-13T12:00:00", False, (1, 2, 0, 0, 0, 0), True),
        Case("noon-24h", "2026-03-13T12:00:00", True, (1, 2, 0, 0, 0, 0), True),
        Case("late-pm-12h", "2026-03-13T23:59:58", False, (1, 1, 5, 9, 5, 8), True),
        Case("late-pm-24h", "2026-03-13T23:59:58", True, (2, 3, 5, 9, 5, 8), True),
        Case("morning-12h", "2026-03-13T09:08:07", False, (0, 9, 0, 8, 0, 7), False),
    ]

    failures = 0
    for c in cases:
        dt = datetime.fromisoformat(c.iso)
        digits, is_pm = firmware_digits(dt, c.use24h)
        ok = digits == c.expected_digits and is_pm == c.expected_pm
        status = "PASS" if ok else "FAIL"
        print(f"[{status}] {c.label}: digits={digits} pm={is_pm}")
        if not ok:
            print(f"       expected digits={c.expected_digits} pm={c.expected_pm}")
            failures += 1

    if failures:
        raise SystemExit(1)

    print("\nAll binary clock conversion checks passed.")


if __name__ == "__main__":
    run()
