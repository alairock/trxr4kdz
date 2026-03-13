#!/usr/bin/env python3
import argparse
import json
import sys
import time
import urllib.error
import urllib.request


def req(host, method, path, body=None, token=None, retries=3):
    url = f"http://{host}{path}"
    data = None
    headers = {"Accept": "application/json"}
    if token:
        headers["X-API-Key"] = token
    if body is not None:
        data = json.dumps(body).encode("utf-8")
        headers["Content-Type"] = "application/json"

    last_err = None
    for attempt in range(retries):
        r = urllib.request.Request(url, data=data, method=method, headers=headers)
        try:
            with urllib.request.urlopen(r, timeout=8) as resp:
                raw = resp.read().decode("utf-8")
                try:
                    parsed = json.loads(raw) if raw else None
                except json.JSONDecodeError:
                    parsed = raw
                return resp.status, parsed
        except urllib.error.HTTPError as e:
            raw = e.read().decode("utf-8")
            try:
                parsed = json.loads(raw) if raw else None
            except json.JSONDecodeError:
                parsed = raw
            return e.code, parsed
        except Exception as e:
            last_err = str(e)
            time.sleep(0.4 * (attempt + 1))

    return 599, {"error": f"request failed after retries: {last_err}"}


def active_id(screens_payload):
    return screens_payload.get("activeScreen")


def enabled_screens(screens_payload):
    return [s for s in screens_payload.get("screens", []) if s.get("enabled", True)]


def next_enabled_id(screens_payload, current_id):
    en = sorted(enabled_screens(screens_payload), key=lambda s: s.get("order", 0))
    ids = [s["id"] for s in en]
    if current_id not in ids or not ids:
        return None
    i = ids.index(current_id)
    return ids[(i + 1) % len(ids)]


def duration_for_id(screens_payload, sid, default=10):
    for s in screens_payload.get("screens", []):
        if s.get("id") == sid:
            d = s.get("duration", default)
            return d if isinstance(d, int) and d >= 0 else default
    return default


def fail(msg, evidence):
    print(f"FAIL: {msg}")
    print(json.dumps(evidence, indent=2))
    sys.exit(1)


def main():
    ap = argparse.ArgumentParser(description="Validate screen cycling play/pause/scrub behavior via API")
    ap.add_argument("--host", default="192.168.0.48")
    ap.add_argument("--token", default="")
    ap.add_argument("--slack", type=float, default=2.0, help="extra seconds beyond duration")
    args = ap.parse_args()

    ev = {"host": args.host, "ts": int(time.time()), "checks": []}

    st, screens0 = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200 or not isinstance(screens0, dict):
        fail(f"GET /api/screens failed ({st})", ev)
    ev["initial"] = screens0

    en = enabled_screens(screens0)
    if len(en) < 2:
        fail("Need at least 2 enabled screens for cycling validation", ev)

    initial_cycling = bool(screens0.get("cycling", True))

    st, pause_resp = req(args.host, "PUT", "/api/screens/cycling", body={"enabled": False}, token=args.token)
    if st != 200:
        fail(f"PUT /api/screens/cycling false failed ({st})", ev)
    ev["checks"].append({"pause_resp": pause_resp})

    st, paused_state = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200:
        fail(f"GET /api/screens after pause failed ({st})", ev)
    a1 = active_id(paused_state)
    if not a1:
        fail("No activeScreen after pausing", ev)

    wait1 = duration_for_id(paused_state, a1, 10) + args.slack
    time.sleep(wait1)

    st, paused_late = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200:
        fail(f"GET /api/screens pause hold failed ({st})", ev)
    a2 = active_id(paused_late)
    ev["checks"].append({"pause_hold": {"before": a1, "after": a2, "wait_s": wait1}})
    if a1 != a2:
        fail("Active screen changed while cycling paused", ev)

    st, next_resp = req(args.host, "POST", "/api/screens/next", body={}, token=args.token)
    if st != 200:
        fail(f"POST /api/screens/next failed ({st})", ev)

    st, after_next = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200:
        fail(f"GET /api/screens after scrub failed ({st})", ev)
    a3 = active_id(after_next)
    ev["checks"].append({"scrub_next": {"from": a2, "to": a3, "resp": next_resp}})
    if a3 == a2:
        fail("Scrub via /api/screens/next did not advance screen while paused", ev)

    wait2 = duration_for_id(after_next, a3, 10) + args.slack
    time.sleep(wait2)
    st, paused_after_scrub = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200:
        fail(f"GET /api/screens post-scrub hold failed ({st})", ev)
    a4 = active_id(paused_after_scrub)
    ev["checks"].append({"pause_after_scrub_hold": {"before": a3, "after": a4, "wait_s": wait2}})
    if a4 != a3:
        fail("Active screen changed while paused after scrub", ev)

    expected_next = next_enabled_id(paused_after_scrub, a4)
    st, play_resp = req(args.host, "PUT", "/api/screens/cycling", body={"enabled": True}, token=args.token)
    if st != 200:
        fail(f"PUT /api/screens/cycling true failed ({st})", ev)

    wait3 = duration_for_id(paused_after_scrub, a4, 10) + args.slack
    time.sleep(wait3)
    st, playing_late = req(args.host, "GET", "/api/screens", token=args.token)
    if st != 200:
        fail(f"GET /api/screens after resume failed ({st})", ev)
    a5 = active_id(playing_late)
    ev["checks"].append({"resume_play": {"from": a4, "to": a5, "expected_next": expected_next, "wait_s": wait3, "resp": play_resp}})
    if a5 == a4:
        fail("Active screen did not advance after resuming cycling", ev)

    # restore initial cycling state
    req(args.host, "PUT", "/api/screens/cycling", body={"enabled": initial_cycling}, token=args.token)

    print("PASS: screen cycling play/pause/scrub validated")
    print(json.dumps(ev, indent=2))


if __name__ == "__main__":
    main()
