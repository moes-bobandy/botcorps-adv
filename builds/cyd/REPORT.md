# BOTcorps CYD Touch Port Report

## Tip SHA
000fca0

Interactive mock of the current CYD build: mocks/CYD-current-interactive.html
Built by: Super Grok session (no Cursor/agent hand-off)

## Adv alignment (this tip)
- Rebased paint onto Adv PR #7 tip 07d4fa39. CYD uses the same botcorps_ui.cpp paint code; only touch, OSK, 38x60 badge, focus-only labels and prompt clip sit behind #ifdef BOTCORPS_CYD.
- Added botcorps_disp() and botcorps_band_set_push_ymax stubs in tools/host/host_shim.h.
- Added tools/host/render_cyd.sh to emit board / Terminal / Tab sheet / OSK / badge PNGs.
- Mock rebuilt from those PNG frames with touch areas on top. No CSS-drawn screens.
- CYD rules kept: touch, OSK keys, badge opens OSK, no INT, v11 rain, all 40 commands, no instruction text.
- Side-by-side: mocks/cyd-frames/adv-vs-cyd-side-by-side.png

## Mock sheet
- Restored CORE go/load/defend/sweep/roster/pulse/dig so the sheet lists all 40 commands matching firmware CMD_N and v3.2.
- Icon-only panel, focus-then-select, and v3.2 sheet kept.

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
- Flash: 1,387,919 / 3,145,728 bytes (44.1%)
- DRAM .bss 45,880 + .data 26,196 = 72,076
- Image stays in `.pio/build/cyd/firmware.bin`. It is not tracked.
- Measured this session with `pio run -e cyd` (SUCCESS, 299 s).

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
