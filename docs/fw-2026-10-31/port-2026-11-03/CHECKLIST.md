# Port checklist: three LOCKED mocks into cyd_livescan (2026-11-03)

- [x] ui_main_bot_rain (a45ed32): main-bot rain on GIBSON (HOME), SCANbot (SCAN), TERMINALbot (TERM). Sim vs lock: 0 px differ in RGB565 on every owned pixel, all 10 steps, green (theme 2) and orange (theme 5).
- [x] ui_home_tap_glow variant O (b144df6): MORPHEUS chip, CONTROL chip + TERMINAL pins, GIBSON cream shaded face + small pins, breathing face glow on MORPHEUS / CONTROL / GIBSON / SCAN / TERMINAL. Sim vs lock O frames 7..147: lock-drawn pixels are exact (10604/10604); glow drawn over the live clip is within 3/63 levels in RGB565 (lock codec noise in its base).
- [x] ui_scan_glow (dfcfbca): only the WiFi face is orange #ff7a00, only the BT face is blue #2f6bff; bezel, label, pins and ring stay grey; halo glow on WIFI / BLE / GIB at AMAX 1.0, 9 px, 1.4 s. The old whole-chip tint is replaced. Sim vs lock peak and trough: every face and halo pixel is within 2 levels in RGB565.
- [x] Not ported: the unlocked TERMINAL and MORPHEUS screen-glow mocks.
- [x] Glow cost: frame-timed. Rain runs repaint only on a step change (140 ms); glow runs repaint only on a phase change (30 fps, 42-frame cycle). No full redraws. Both freeze when BG MOTION is OFF, like the clip.
- [x] The locked clip blob (src/lock/live/lock_blob_live.bin) is unchanged.
- [x] pio run -e cyd_livescan: SUCCESS with no new warnings (warning set identical to the HEAD build).
- [x] Gate: identical to the HEAD baseline (see REPORT.md).
- [x] Bins and sha256s: FLASH.md and SHA256SUMS.
- [ ] NOT FLASHED. Flash it yourself with FLASH.md.
- [ ] HOLD: SCAN lock-on tower cycling ("just cycle thru towers, don't change the look"). Not started on purpose: the owner is picking one of three earlier designs. The scan/radar lock code is untouched in this build.
- [ ] On-device check after flashing: the HOME / SCAN / TERM glow and rain match the sim renders, the press flash still works on the MORPHEUS chip, and the WIFI / BLE busy pulse still shows while scanning.
