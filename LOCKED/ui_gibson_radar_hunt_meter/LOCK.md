# LOCKED: ui_gibson_radar_hunt_meter (Gibson radar hunting meter, A thermometer bands)

- **Locked:** 2026-11-05 ~10:17 AM ET. Mobandy picked "Pick A thermometer bands" for the CYD Gibson radar hunting meter.
- **Source:** `c83a27e7867106734d1cdad9cef2e0791966c53a` on `backup/cyd-livescan-2026-10-30` (botcorps-adv), `docs/mocks/radar-hunt-meter/A/` (and `/workspace/out/radar-hunt-meter/A/`).
- **This is a mock only.** No firmware was built and nothing was flashed. Porting it into firmware needs Mobandy's word.
- Adds to the locked `ui_gibson_radar` / `ui_gibson_radar_charge`; charge lock D4 (`ui_gibson_radar_charge`) is unchanged.

## What is locked
- Thermometer-band hunting meter inside the hunted tower glass: horizontal segments stacked green (base) → yellow (mid) → red (tip of fill).
- Top 11-bar hunt meter cleared; HUNT / name / dBm / STEADY kept.
- Fill levels: idle 22% / mid 55% / full 100%.
- Clip: hunt_meter_A_3x.mp4, 60 frames, 2.000 s, 720x960 (3x), H.264 yuv420p faststart, sha256 cbbea992ec5d7963a47e54205ee2e7bb0ce9ca36838835dd40c13da8b8a993a7.

## Rules
- Never change this screen; only add.
- Charge lock D4 (`ui_gibson_radar_charge`) unchanged.
