# BOTcorps CYD Touch Port Report

## Tip SHA
(pending)

Interactive mock of the current CYD build: mocks/CYD-current-interactive.html
Built by: Super Grok session (no Cursor/agent hand-off)

## cv=2 actually saved (this tip)
- run_touch_cal's save now writes checked prefs.putUChar("cv", 2). Unused 4-value save no longer writes cv. has_valid_cal checks all c0..c7 and rejects all-equal/zero. Auto-cal no longer loops forever. LV stays “LV 01”.

## Auto-cal on missing cv=2 (this tip)
- On boot, if no valid cv=2 8-point cal exists, auto-enters botcorps_cyd_run_touch_cal before the rain/PLAY screen. Hold-at-boot remains for forced re-cal. Touch is polled on the normal path via getTouch in read_key. LV stays “LV 01”.

## Marker redraw + live offsets + cv=2 (this tip)
- Crosshair redraws at the top of each while(!got) so jitter rejects don’t leave a blank screen.
- touch_off/panel_off read from config().offset_rotation. Header comment fixed to offset_rotation 0.
- Saves cv=2; load ignores older c0..c7 without it. LV stays “LV 01”.

## Panel offset + true-corner cal (this tip)
- Panel cfg.offset_rotation set to 0 so setRotation(1) yields 320x240 (was becoming internal 2 = portrait, begin check failed). Touch offset stays 0.
- Cal rotation computed LovyanGFX-style; true corners via px=(w-1)*((i>>1)&1), py=(h-1)*(i&1). One 15 s deadline per target across jitter retries; lift wait after reject. Header comment merged. LV stays “LV 01”.

## Timeout-capable cal (this tip)
- Replaced calibrateTouch (no timeout) with custom loop: 15 s per target, rotation restore on every exit, return false + log “cal timeout” without saving. Same native order / 16-avg / jitter. Dropped redundant apply after setCalibrate path. LV stays “LV 01”.

## calibrateTouch + no overwrite (superseded by custom timed loop)
- apply_touch_cal8 no longer calls setTouch (that re-ran touchCalibrate from 300..3900 and overwrote the 8-point matrix). Legacy path still does.
- run_touch_cal now calls g_cyd.calibrateTouch(cal, 0xFFFF, 0x0000, 10) after the lift pre-check; it draws corner markers only, handles native order/rotation, and we save the 8 values as c0..c7. Logs if NVS save fails.
- LV stays "LV 01".

## 8-point cal + boot-hold fixes (this tip)
- apply_touch_cal8 uses LovyanGFX setCalibrate with 8 extrapolated corner values; offset_rotation left at constructor (no double flip).
- Cal flow: custom 15 s deadline per target (no hang), 16-sample average, 20-raw jitter, native corner order, restores rotation, no save on timeout.
- Boot-hold window opens only if touch is down on first read.
- LV stays "LV 01".

## Touch cal math + boot fallback (this tip)
- Y extrapolation now /200 (targets 20→220 = 200 px); X stays /280.
- Kept measured min/max order (no swap that discards direction).
- Detected which raw axis moved TL→TR and derived rotation (0-3) from tap deltas; trot no longer hard-coded 1.
- Averaged the 3 stable raw reads; wait for finger lift before target 1 (Enter-key case).
- Returns bool; caller logs "cal timeout" on timeout, "cal done" on success. Serial too.
- No-text hold-at-boot fallback: continuous raw touch ~1s in first 2.5s window runs cal (no on-screen text; typing "cal" still works once touch is good).
- LV stays "LV 01". Nothing flashed or merged.

## Touch matrix + cal wiring (previous tip)
- apply_touch_cal now calls _panel.setTouch(&_touch) after updating cfg so the touch matrix is rebuilt (LovyanGFX only calibrates inside setTouch/touchCalibrate).
- botcorps_cyd_run_touch_cal: 15s timeout per target, extrapolates 20px-inset crosshairs to full 0..319/0..239, measures axis direction from the four taps.
- Wired entry: Terminal command "cal" (no instruction text on screen). LV stays "LV 01".

## Touch cal fix (this tip)
- Removed duplicate map_touch / load/save from botcorps_ui.cpp (getTouch already returns mapped pixels; double-map pinned taps).
- Added botcorps_cyd_save_touch_cal beside the existing loader with matching UShort/UChar types.
- Added botcorps_cyd_run_touch_cal: four corner crosshairs only, raw XPT2046 reads via get_touch_raw, computes range, saves, and calls apply_touch_cal live. No instruction text.
- LV remains "LV 01".

## Touch cal + host stubs (this tip)
- XPT2046 cal load/save to NVS (keys tcal/tx0/tx1/ty0/ty1/trot) under BOTCORPS_CYD. Linear map applied in read_key when tcal=1. Placeholder 300..3900 until set. Tap-target cal UI not yet wired (no instruction text added).
- Vendored stub LovyanGFX font headers in tools/host/lgfx/Fonts/ so host dumps can compile without system fonts. Full paint dump still blocked on complete host link; stubs + honesty kept.
- LV left as hardcoded "LV 01" until Dirt decides meaning.

## CYD follow-up (this tip)
- Bumped tip SHA from dddee10 to 04b0357 (HEAD at start of this round; no firmware delta, fw still acfc0e3).
- Placeholder frames in mocks/cyd-frames/ and mocks/shots/cyd/ remain stubs (dark 320x240 with corner labels + small LV box on badge). They are not real host paint dumps or live UI art. Honesty kept until a host renderer with LovyanGFX fonts can dump real frames under BOTCORPS_CYD.
- firmware.bin size from clean `pio run -e cyd`: 1,406,000 bytes (exact .pio/build/cyd/firmware.bin). Map flash figure: 1,405,591 / 3,145,728 bytes (44.6%). RAM: 72,076 / 327,680 bytes (22.0%).
- Live LV / board observation: no CYD board available in this sandbox session; nothing flashed. Code review confirms cyd_overlay is called from the CYD draw paths (after patch_world) and paints badge + OSK when dirty; on_esc / cyd_close_osk closes the OSK (s_osk_open) before any navigation when the keyboard is up. Host type gate still passes.

- Soft live LV: still hardcoded "LV 01". bot_t has no level field; not inventing state (CONTEXT.md: don't guess TBD). Deferred.
- Soft real CYD host dumps: LovyanGFX font headers not present on this host; frames remain placeholder stubs. Honesty kept.


## Compile fix + overlay restore (this tip)
- Fixed orphan #endif after cyd_close_osk and mangled return K_NONE;#endif in read_key so #ifdef BOTCORPS_CYD is terminated.
- Wrapped cyd_close_osk + paint_osk in #ifdef BOTCORPS_CYD; #endif on its own line after the CYD read_key branch.
- Restored cyd_overlay + on_esc OSK-close from a28caec (overlay after patch_world so declaration order is valid).
- Placeholder frames remain; honesty kept until real host dumps exist.

## Adv alignment + CYD restore (previous tip)
- Rebased onto Adv tip 07d4fa39 then restored CYD touch/OSK/INT-skip paths from a28caec (getTouch in read_key, badge→OSK, paint_osk, paint_int_now early return, push_ymax) while keeping Adv paint code.
- Fixed BOTCORPS_HOST_RENDER: restored real botcorps_host_type_letters, removed duplicate host_* functions and brace issues.
- mocks/cyd-frames and mocks/shots/cyd contain placeholder PNGs only. Real host paint dumps are not available on this box (missing LovyanGFX font headers). Do not treat them as live frames until a host renderer with BOTCORPS_CYD can dump them.
- Interactive mock sheet restored with the 40 CORE commands (go/load/defend/sweep/roster/pulse/dig).
- RAM/flash re-measured after clean pio run -e cyd (SUCCESS).

## Build
- Environment: `cyd`
- Board: ESP32 Dev Module (ESP32-2432S028)
- Display: ILI9341 320x240, LovyanGFX, 4-wire SPI (MISO 12, `spi_3wire=false`)
- Touch: XPT2046. Range 300..3900 is a placeholder. `offset_rotation` defaults to 1 to match `setRotation(1)`. NVS namespace `botcorps` keys `tcal`, `tx0`, `tx1`, `ty0`, `ty1`, `trot` override it when `tcal=1`. Not measured on a device.
- Frame buffer: BANDS (320x120)
- Partition: huge_app.csv
- Built with `pio run -e cyd` from this tree. Nothing flashed.

## Memory
- RAM: 72,076 / 327,680 bytes (22.0%)
- Flash: 1,405,591 / 3,145,728 bytes (44.6%)
- DRAM .bss 45,880 + .data 26,196 = 72,076
- Image stays in `.pio/build/cyd/firmware.bin`. It is not tracked.
- Measured this session with clean `pio run -e cyd` (SUCCESS).

## What is actually done
- Compact ID badge in the terminal companion slot, (274, 174), 38x60 (mock companion size). Round glyph, short name, `LV 01`, that bot's palette color. The box does not move or mirror. Full `paint_int` / `botcorps_id_tick` are not called on CYD, and the INT strip sprite is not allocated. The badge is redrawn when the selected bot changes, not every loop. Dirt asked for a badge tap to open the OSK; that is the behavior.
- `s_int_dirty` is cleared on CYD.
- On-screen keyboard uses one rect table (`src/ui/botcorps_cyd_osk.h`): 15px rows from y=165, 30px columns from x=8. SPACE, BKSP, ENTER, TAB, SYM, and HIDE are real filled keys (palette inks). Paint and hit-test share the table. Paint runs when the keyboard opens or the buffer/layer changes. EXT band pushes stop at y=151 while it is open, and the rows are pushed again when it closes.
- Touch fires once per press (~50 ms). A board tile or icon focuses on the first tap and selects on the second. Crown opens the Gibson hub. RX closes the sheet or the keyboard. A sheet row runs the command, or arms an argument and opens the keyboard. A group header jumps to that group. Arrow taps step the sheet. Top-left (x<36, y<18) is back on Terminal (to Gibson) and on other non-rain screens; Rain has no top-left back. ESC closes the OSK if it is open, and only then navigates. There is no blanket `K_OK`.
- Palette `botcorps_pal_ids_ok()` is constexpr without a loop, so the `static_assert` holds under gnu++11 and gnu++17. The CYD env also drops `-std=gnu++11`.
- `builds/cyd/botcorps-cyd.bin` is untracked.
- Typing goes through `on_char` only. `botcorps_host_type_letters()` is called at the start of the host renderer (`tools/host/render_main.cpp`); a nonzero return fails the host process. `tools/host/run_cyd_type_test.sh` builds and runs the key-table test and a CYD case that types through `cyd_osk_at` / `cyd_osk_hit`. Both passed in this session. The full host renderer still is not built with `BOTCORPS_CYD` (it needs LovyanGFX font headers and M5 stubs).
- Terminal bot replies use `log_add_bot` (`\x01` plus the bot index) and `bot_color`. The user prompt plate is palette index 194. Idle boot lines stay `COL_INK`. Not checked on a panel.
- CYD board silk is only on the focused tile. Adv still labels every tile.
- Rain stays off on Terminal, both lists, and diag. CYD boot/intro uses v11 density (gap 14, every third column off). Adv still uses the full EXT cell rain (gap 6, 54 columns).

## Not done on a device
- XPT2046 min/max and rotation are uncalibrated.
- Visual match to the mocks, including badge size and whether LV should change.
- Intro rain density is coded for CYD. Not checked on glass.
- Adv envs were not rebuilt here. `lib_deps` lists NimBLE-Arduino 2.3.9 once. The CYD link shows that one copy plus LovyanGFX.
- Optional I2C keyboard is unwired.
- Host frames do not define `BOTCORPS_CYD`, so they do not paint this badge or keyboard.

## Build
`pio run -e cyd`

## Post-rebase screenshots
Rendered CYD screens (from host paint path) copied to mocks/shots/cyd/ for review.
Built by: Super Grok session (no Cursor/agent hand-off)
