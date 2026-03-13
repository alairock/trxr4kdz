# Screen Cycling Play/Pause/Scrub Checklist

## Scope
Validate end-to-end behavior for global screen cycling controls on the new firmware line.

## Automated API validation
Run from repo root:

```bash
scripts/validate_screen_cycling.py --host 192.168.0.48
# or if API token is configured
scripts/validate_screen_cycling.py --host 192.168.0.48 --token "$API_TOKEN"
```

Expected result: `PASS: screen cycling play/pause/scrub validated`.

The script validates:
1. Pause (`PUT /api/screens/cycling {enabled:false}`) holds active screen beyond duration.
2. Scrub (`POST /api/screens/next`) advances while paused.
3. Paused state still holds after scrub.
4. Resume (`PUT /api/screens/cycling {enabled:true}`) advances after duration.

## Manual hardware sanity (buttons)
On device:
1. Wait until normal screen loop is running.
2. Double-press middle button → should show transient `paused` and stop auto-advance.
3. While paused, short-press right button → move to next screen.
4. While paused, short-press left button → move to previous screen.
5. Double-press middle button again → should show transient `playing` and auto-advance resumes.

## Evidence capture
- Save automated script output to `artifacts/screen_cycling_validation.txt`.
- Note any visible button behavior mismatch in sprint notes before closing task.
