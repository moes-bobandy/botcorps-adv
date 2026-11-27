# MOCK 2026-11-26 (CYD-22): G2 long-press back + LIVE device list + script typist

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash, no SD, no merge.
Branch `mock/cyd22-g2-longpress-2026-11-26` on `build/v1.2.23-2026-11-26` (`a87eb83`).
Flag: `LV_CYD22_G2` (default 0). With 0 the firmware path and every render are byte-identical to v1.2.23.
Hold time: `LV_G2_HOLD_MS` = 1000 (1 second).

## Back control
The top-left Stripe Slot back button is removed on G2. A long-press (~1 s hold) anywhere that is not another control exits G2 to the same target as the old back button (`lv_back()`: pager home, or the Dojo if G2 was opened from there). The gold TERM pill is gone; a dim `term` glyph label sits in the same corner and keeps the same hit rect (tap still opens the live typed terminal). A hold on `term` does not exit. Short taps on the body do not exit. The release tap after a long-press exit is swallowed so it cannot fire on the destination screen.

## Script typist (Mobandy 9:20 PM ET)
G2 is still a typist terminal, not a GUI. The chart chrome (headers, gold pill, boxed status lines) is replaced with a tight even script:

- Official FIGlet "big" `BOT` wordmark in the theme accent, plus a smaller `corps` on the baseline
- `v1.2.23` in dim grey; `LIVE` / `SAMPLE` on the right (LIVE is bright accent)
- Armbian-style dim-grey section headers (`scan`, `devices`) and column labels (`name`, `class`, `rssi`, `ch`)
- Dim labels, bright values (`wifi` / `ble` / `deauth` counts; RSSI and CH on every device row, including 0)
- 8 px pitch from y=4 (the old back-button row is now the logo), 5x7 only, cap 22 rows + `+K more`
- Empty LIVE states: `SCANNING...` if the job is still running, `no devices on grid` if it finished with nothing
- Faster even typist; no gold box, icons, or panels
- F2 and flag-off keep the old `lv_tps2_build` / pill path

## Why the list was empty on the CYD (LIVE)
G2 always builds its chart from `lv_f[]` / `lv_nf`. The host sim tags the first line `SAMPLE` and the canned fake radio fills `lv_f[]` after 1.2 s (Wi-Fi) / 5 s (BLE), so the list looks full in sim. The CYD tags `LIVE` and starts empty: `lv_tps_enter` kicks off `lv_job_start(LV_J_ALL)`, and finds only arrive once the live scan returns. The typist used to draw the prompt and headers with no device rows and no empty-state line. That is the field report (Mobandy 3:24 PM ET): "Gibson> prompt and then listed nothing." Late-arriving finds could also miss a dirty-row blit.

## Fix (same flag)
- When `n_dev==0` and a scan is still running: `SCANNING...`
- When `n_dev==0` and the job has finished: `no devices on grid`
- Every device row is drawn, and CH/RSSI print even when the value is 0 (BLE rows no longer blank the CH column).
- A change in `lv_nf` / job / counts forces a full G2 push (`lv_tps_full=1`) so late LIVE results actually paint.

Proved on the device LIVE path in the host sim with `-DLV_TPS_DATA_TAG=\"LIVE\"` (not the SAMPLE tag): a frozen radio shows the SCANNING empty-state; a completed scan shows the full name/class/rssi/ch list with LIVE on the meta line.

Renders (Vapor Grid, theme 22) are in `/workspace/out/fw-bigbot-1117/cyd22-g2-longpress/`. `cyd22_01`–`03` are the pre-restyle chart; `cyd22_04`–`06` are the script typist (SAMPLE full, LIVE full, LIVE empty).

Gate (`gate_c22.txt`): OFF vs a87eb83: 0 px on home + G2 mid + G2 full. ON vs base: home 0 px. G2 typist diffs are the script restyle (expected). Long-press and `term` tap/hold still behave as above.

## Scan chart variations (Mobandy 2026-11-27)
Four labeled stills of the top `scan` info, same device list and typist face. The FIGlet `BOT` letter gap is one column tighter in every variation (glyph shapes and `corps` unchanged). Default typist (no `SIM_C22_VAR`) keeps the stacked `wifi` / `ble` / `deauth` list, now with that tighter wordmark.

- **Var A** — spreadsheet: types as columns, one value row (`wifi` `ble` `deauth` / `9` `11` `0`)
- **Var B** — ruled typist table: `typ` / `n` header, dash rule, one row per type
- **Var C** — suggested: scan counts hang to the right of the `T` stem (neofetch-style), reclaiming the vertical scan block
- **Var D** — compact key=value ledger: `wifi=9  ble=11  deauth=0  tot=20`

Reproduce in the host sim with `SIM_C22_VAR=A|B|C|D` (LIVE path). Renders: `cyd22_07`–`10` in `/workspace/out/fw-bigbot-1117/cyd22-g2-longpress/`. On-image `VAR A`–`D` labels are dim typist glyphs in the bottom-left corner. Mock only; pick one before it becomes the default typist.
