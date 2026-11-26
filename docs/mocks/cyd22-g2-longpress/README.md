# MOCK 2026-11-26 (CYD-22): G2 long-press back + LIVE device list

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash, no SD, no merge.
Branch `mock/cyd22-g2-longpress-2026-11-26` on `build/v1.2.23-2026-11-26` (`a87eb83`).
Flag: `LV_CYD22_G2` (default 0). With 0 the firmware path and every render are byte-identical to v1.2.23.
Hold time: `LV_G2_HOLD_MS` = 1000 (1 second).

## Back control
The top-left Stripe Slot back button is removed on G2. That rect is filled with the underlying CRT black (`#000000`), which is what `lv_tps_g2_scene` already paints before the slot was drawn. A long-press (~1 s hold) anywhere that is not another control exits G2 to the same target as the old back button (`lv_back()`: pager home, or the Dojo if G2 was opened from there). The TERM pill keeps its tap (opens the live typed terminal). A hold on the TERM pill does not exit. Short taps on the body do not exit. The release tap after a long-press exit is swallowed so it cannot fire on the destination screen.

## Why the list was empty on the CYD (LIVE)
G2 always builds its chart from `lv_f[]` / `lv_nf`. The host sim tags the first line `SAMPLE` and the canned fake radio fills `lv_f[]` after 1.2 s (Wi-Fi) / 5 s (BLE), so the list looks full in sim. The CYD tags the first line `LIVE` and starts empty: `lv_tps_enter` kicks off `lv_job_start(LV_J_ALL)`, and finds only arrive once the live scan returns. With `lv_nf==0` the typist still drew `gibson> scan all LIVE`, the headers, and a final `gibson>` — but no device rows and no empty-state line. That is the field report (Mobandy 3:24 PM ET): "Gibson> prompt and then listed nothing." Late-arriving finds could also miss a dirty-row blit.

## Fix (same flag)
- When `n_dev==0` and a scan is still running: `[ .. ] SCANNING...`
- When `n_dev==0` and the job has finished: `[ -- ] NO DEVICES ON GRID`
- Every device row is drawn, and CH/RSSI print even when the value is 0 (BLE rows no longer blank the CH column).
- A change in `lv_nf` / job / counts forces a full G2 push (`lv_tps_full=1`) so late LIVE results actually paint.

Proved on the device LIVE path in the host sim with `-DLV_TPS_DATA_TAG=\\"LIVE\\"` (not the SAMPLE tag): a frozen radio shows the SCANNING empty-state; a completed scan shows the full NAME/CLASS/RSSI/CH chart with LIVE on the command line.

Renders (Vapor Grid, theme 22) are in `/workspace/out/fw-bigbot-1117/cyd22-g2-longpress/`.

Gate (`gate_c22.txt`): OFF vs a87eb83: 0 px on home + G2 mid + G2 full. ON vs base: 0 px outside the Stripe Slot rect (x1-32,y1-23) and the device-list band (y90-306). Home is 0 px. The removed slot is CRT black.
