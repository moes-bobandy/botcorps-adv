# LOCKED: ui_scan_glow (SCANbot screen tap glow)

- **Locked:** 2026-11-03 10:56 PM ET. Mobandy: "Yes lock that" (about the stronger halo-only SCAN glow, bfbef79).
- **Source:** `bfbef79d32282d5c03a7c19d345ea2c386838a2c` on `backup/cyd-livescan-2026-10-30` (botcorps-adv), `docs/mocks/screen-glow/`.
- **This is a mock only.** No firmware was built and nothing was flashed.

## What is locked
- WiFi chip face tinted orange #ff7a00 (mid 255,122,0 / hi 255,174,0 / shadow 112,53,0); Bluetooth chip face tinted blue #2f6bff (mid 47,107,255 / hi 67,152,255 / shadow 21,47,112). Bezel, label, pins and ring stay grey.
- Breathing halo only, on WIFI, BLE and GIB: AMAX 1.00, reach 9 px at 1x, 1.4 s breath at 30 fps, all in sync. Halo colours are each face colour (GIB 247,242,230).
- Faces never change in any frame (face_px_changed 0, max_lift_px 0); nothing outside the glow windows changes (px_outside_changed 0). See scan_stats.json.
- Clip: 168 frames, 30 fps, 720x960 (3x), H.264 yuv420p faststart.

## Rules
- Never change this screen; only add.
- Firmware today tints the whole chip areas at a 5/8 mix; porting this face-only tint and halo into firmware needs Mobandy's word.
