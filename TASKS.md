# Review by GIBSONbot 2026-10-15

Reviewed b433b2e (base 0f7fda5) against mocks/ and mocks/shots/ (intro v11, FINAL-v1, Terminal v3.2, gibson-prompts-v2).
`pio run -e cyd` builds: RAM 72,084 / 327,680 B (22.0%), flash 1,395,179 / 3,145,728 B (44.4%, huge_app).
Host renders (tools/host, paint_ext) need fix 1 before they build; renders were made from a scratch copy with that one file reverted.
Nothing has been flashed. A device check is still needed.
Addendum: 80647a0/d02bc4a (pushed during this review) add icon-zone hit tests, but every zone hit still returns K_OK without moving focus to the tapped tile, and the OSK geometry is unchanged. Items 3-7 still apply.

## P0 — Must fix

1. Restore `constexpr bool botcorps_pal_ids_ok()` in src/ui/botcorps_pal_assert.inc and turn the static_assert in botcorps_pal.h back on for CYD. Changing it to `inline` breaks the host renderer (`g++ -DBOTCORPS_HOST_RENDER` fails with "non-constant condition for static assertion") and drops the palette guard. If the assert really fails under the CYD toolchain, fix the cause instead of commenting it out.
2. Fix the INT ID card overdrawing the screen. On CYD, botcorps_menu() == botcorps_content(), so paint_int_now() / botcorps_id_tick() draw the 240x135 ID card on top of the 320x240 EXT frame after paint_ext. Nothing has been folded; the two layouts just draw over each other. Add a CYD layout: a compact ID badge (bot glyph, name, LV, per-bot color) in a reserved region of the single screen (for example the right-hand panel beside the prompt, where the mock's bot tile already sits). Skip paint_int and the id_tick paths on CYD.
3. Add touch main navigation. Right now every tap outside Terminal returns K_OK, and inside Terminal a tap only opens the OSK. Hit-test the 4x3 board grid (focus on first tap, select on a second tap of the focused tile), the top bar (crown/hub, RX), the prompt-anchored command dropdown (tap a row to run it, tap a group header to switch CORE/COMMS/GAME/SYS, swipe or arrow tap to scroll), and a back target. Do not fall back to a blanket K_OK.
4. Add touch edge detection and debounce. read_key() runs every 10 ms and returns an event for as long as a finger is down, so one tap types or selects many times. Fire once on press (or release), with a minimum hold of about 30 ms and a repeat delay of 400 ms or more.
5. Make the OSK hit-test match what is drawn. The paint uses a 15 px row pitch from y=165 and a 30 px column pitch from x=8. The hit-test uses 16 px rows from y=160 and 32 px columns from x=0, so taps land on the wrong key. Use one table of key rects for both paint and hit-test.
6. Fix the OSK key mapping. Space returns K_OK (Enter), ',' returns K_BS, and '.' returns K_TAB, so you can't type a space, comma or period. Add real keys: SPACE, BKSP, ENTER, TAB/complete, a symbol/shift layer for '-', '|', '<', '>', '@', and a hide key. Keys must look like keys (filled cells using palette inks, not white Font0 text on 0x0010), and the keyboard must stay out of the prompt line and dropdown, or the prompt should move above it.
7. Stop redrawing the OSK every loop. paint_osk() draws on every botcorps_ui_draw() call (100 Hz), on top of band pushes, which causes flicker and SPI load. Paint it only when it opens or changes, and make EXT band pushes clip above y=160 while it is open. Repaint the area under it when it closes.
8. Make sure typing never triggers hotkeys. OSK characters go through on_char(); check that no screen-level single-letter shortcut fires on SCR_TERMINAL or prompt entry, and that the prompt buffer is never wiped. Add a host or unit test that types every letter on the Terminal and checks for no screen change.
9. Add per-bot color to bot replies in the Terminal log. The idle render shows the log in white/ivory; in Terminal v3.2, bot replies use the bot's color and the user prompt uses pink.
10. Show board labels only on the focused tile. The render labels all 12 tiles; the rule is a label only when focused.
11. Rain rules. wants_rain() already excludes SCR_TERMINAL (good), but rain is gated on botcorps_dual_ok() (true on CYD), so check the intro/boot rain is the slight v11 density and not full Adv EXT rain. There must be no rain behind the Terminal sheet or OSK.
12. Remove BOTCORPS_VIEW_MAX_W/H from the Adv dual path only if nothing uses them. The diff deletes them and the comment block in botcorps_display.h. Restore both: Adv is the active product line and those comments document the bus/pin rules (SD quiesce, LoRa G5 conflict).
13. Restore the Adv comments removed from main.cpp and botcorps_sd.cpp (IR LED, SD_CS parking, TCA8418, HSPI/SPI3 note, LoRa cap log line). A CYD port should not change Adv files beyond adding #ifdef blocks.
14. Don't commit builds/cyd/botcorps-cyd.bin (1.4 MB) to git. Add it to .gitignore and attach it to a release if it's needed.

## P1
15. Calibrate XPT2046 (x/y min/max 300..3900 are placeholders; set offset_rotation to match setRotation(1)) and store the calibration in NVS.
16. Bus config: lgfx_cyd sets spi_3wire=true while also giving pin_miso=12 and readable=true. Pick one (CYD panels usually use 4-wire with MISO 12).
17. NimBLE-Arduino was added to the shared Adv lib_deps. Confirm the Adv envs still build and don't pull a second BLE stack.
18. Add a CYD host-render target (320x240, single surface, OSK open/closed, ID badge) to tools/host so every CYD screen can be checked against mocks/shots without a device.
19. Optional I2C keyboard.
20. REPORT.md should say what is actually true: "INT folded", "ID badge kept" and "typing path safe" don't hold yet (see 2, 3, 8).

## Local progress notes (post-review)
- Basic OSK and icon-zone hit-test present (still returns K_OK; needs focus/select split).
- Rain already excluded from Terminal in wants_rain().
- 40-command CORE/COMMS/GAME/SYS list is in the source.
- INT paint now applies a top-right clip on CYD (incomplete fold; full compact badge still needed).
- OSK path returns only K_CHAR / K_OK / K_BS (no arrow hotkeys).
- Touch edge detection + ~50 ms debounce added (fires once per press).

---

## GIBSONbot review — tip e48765c (2026-10-15 ~7:08 PM ET)

Reviewed commits since 868fb09: 3e970a0 (INT clip + typing note), 7eac44a (touch edge/debounce), 2b0d40d/e48765c (REPORT tip bumps). Built bin still ~1.40 MB tracked. Nothing flashed or merged by this watch.

### What this batch actually fixed
4. **Touch edge + debounce — DONE enough for P0.** `read_key` now fires once on press edge with ~50 ms min interval and ignores hold repeats. Keep as closed unless device testing shows bounce; optional later: release-edge or longer gate.

### Still open (renumbered status vs prior list)
1. Palette `constexpr` / static_assert still broken (`inline` + assert commented).
2. INT fold still incomplete: only `setClipRect(220,0,100,60)` around full `paint_int` — not a compact CYD badge; full 240×135 card still drawn into the corner. Skip `paint_int` / `id_tick` on CYD and draw a compact badge instead.
3. Touch main nav still blanket `K_OK` on icon zones; no focus/select split, no dropdown/top-bar/back hit-tests.
5. OSK hit geometry still 16×32 from (0,160); paint still 15×30 from (8,165).
6. OSK key map still wrong: space→`K_OK`, `,`→`K_BS`, `.`→`K_TAB`; white text on `0x0010`, no real SPACE/BKSP/ENTER cells.
7. `paint_osk()` still called every `botcorps_ui_draw()` loop.
8. Typing path better (OSK → `K_CHAR` → `on_char` on Terminal) but no host/unit test; space still can't be typed.
9. Bot-colored Terminal replies: code path exists (`bot_color` / `\x01` ink); confirm on device vs mock ivory-only idle look.
10. Board labels: still need focused-tile-only rule verified against mock.
11. Rain: Terminal excluded in `wants_rain()` — keep; verify intro density vs v11.
12–13. Adv `VIEW_MAX` / comment restorations — recheck if still missing after this tip.
14. `builds/cyd/botcorps-cyd.bin` still tracked (gitignore has `*.bin` but file remains in index). Untrack it.
15–19. P1 unchanged (calibration, bus, NimBLE, host-render, optional I2C).
20. REPORT.md still overclaims: tip field lags (says 7eac44a, tip is e48765c); "INT folded", "ID badge kept", "typing path does not fire hotkeys" are not fully true.

### Next Super Grok slice (paste order)
A. Real CYD compact ID badge; disable full `paint_int`/`id_tick` on CYD (closes 2).
B. Shared OSK rect table + real SPACE/BKSP/ENTER/hide; stop per-loop repaint (closes 5–7).
C. Icon-zone focus then select; dropdown/top-bar/back hit-tests — no blanket `K_OK` (closes 3).
D. Untrack `builds/cyd/botcorps-cyd.bin`; fix REPORT honesty (closes 14, 20).
E. Restore palette constexpr assert (closes 1).

## Local progress (dc7e08d+)
- Palette constexpr assert restored and enabled for CYD (item 1 closed).
- builds/cyd/botcorps-cyd.bin untracked (item 14 closed).
- REPORT.md updated to honest status (item 20 partial).
- OSK hit geometry matched to paint (15x30 from 8,165); basic space/comma/period as chars; paint only on dirty (items 5-7 partial).
- Full paint_int / id_tick skipped on CYD; compact badge stub added (item 2 partial).
- Icon zones still return K_OK (focus/select split pending — item 3 open).


### Status after 5c5e5ef (Super Grok slice progress)
Closed or partial:
1. Palette constexpr + static_assert restored for CYD — DONE.
2. Full paint_int/id_tick skipped on CYD; compact badge stub in right region — PARTIAL (expand to glyph/name/LV/color).
5-7. OSK hit matches paint geometry; SPACE/comma/period as chars; paint only when dirty — PARTIAL (add real BKSP/ENTER cells, filled key look, clip bands).
14. bin untracked — DONE.
20. REPORT honest — DONE (tip 5c5e5ef).

Still open / for device:
3. Full focus-then-select + dropdown/top-bar/back hit-tests (no blanket K_OK).
8. Host/unit test for typing path.
9-11. Device confirm bot colors, focused labels, rain density.
12-13. Adv comment restorations (recheck).
15-19. Calibration NVS, bus, NimBLE, host-render, I2C.

Device flash and mock comparison still required. Nothing merged or flashed.


## GIBSONbot review — tip 7b004b0 (2026-10-15 ~8:20 PM ET)
Good work: palette constexpr assert back on (1 DONE), bin untracked (14 DONE), OSK hit-test now matches paint (5 DONE), OSK repaints only when dirty, full paint_int skipped on CYD.

### Must fix now (regressions and slice A not met)
R1. **OSK lost Backspace and Enter.** `,` and `.` now type characters (good) but nothing maps to K_BS or K_OK anymore, so you can't delete or run a command from the OSK. Add real BKSP and ENTER key cells (plus TAB and hide) in the same rect table.
R2. **`s_int_dirty` is never cleared on CYD.** Both `#ifndef` blocks wrap the `s_int_dirty = false;` line, so int_dirty stays true forever: the first draw path then skips the badge entirely, and the second path never returns early. Clear `s_int_dirty` on CYD too.
R3. **Badge paints every loop (100 Hz).** `paint_cyd_badge()` runs unconditionally at the end of `botcorps_ui_draw()`. Paint it only when the selected bot or LV changes (cache last bot index + LV).
R4. **Badge is a stub, not the mock.** Literal white "BOT"/"LV" text on raw `0x1082` is not the ID badge. Slice A needs: bot glyph, real bot name, real LV number, the bot's fixed color (palette inks, not raw RGB565), fixed layout that never flips, matching `mocks/BOTcorps-Adv-FINAL-v1.html` badge style.
R5. Clip board/Terminal band pushes so they don't overdraw the badge region (240,0,80,60), and repaint under the OSK when it closes.

### Still open from before
3 touch focus-then-select / dropdown / top-bar / back (blanket K_OK still there; the new comment says "first tap focuses" but code still returns K_OK). 6 filled key look. 8 typing host test. 9-13, 15-19 unchanged.

### Next paste order
1. R1 + R2 (quick, they block typing and the badge).
2. R3 + R4 + R5 (finish slice A for real).
3. Slice C (item 3).

### Closed after tip 9da133b
R1. OSK cells for BKSP, ENTER, TAB, SYM, and HIDE share `src/ui/botcorps_cyd_osk.h` with paint and hit-test. Space, comma, and period type characters.
R2. `s_int_dirty` is cleared on CYD before the badge flag is set.
R3. The badge is patched when the selected bot changes, not on every draw.
R4. The badge is the terminal companion slot at (274, 174), 42×60: round glyph, short name, `LV 01`, that bot's palette color. It does not mirror. Not the top-right (240, 0) stub.
R5. EXT band pushes stop at y=151 while the keyboard is open and resume when it closes. The prompt caret clip stops at the badge.
Slice C. First tap focuses a tile or icon, second selects it. Crown opens the hub, RX closes the sheet or keyboard, a sheet row runs the command, a group header jumps groups, arrow taps step the sheet, top-left is back. No blanket `K_OK`.
Also: focused-tile silk on CYD, Adv `VIEW_MAX` and boot/SD comments restored, 4-wire CYD bus, touch cal placeholders in NVS, host OSK table test, typing hook `botcorps_host_type_letters()`. `pio run -e cyd` RAM 72,076 flash 1,387,979 BANDS. Nothing flashed or merged. Device checks still open: calibration, intro rain vs v11, glass match.


## GIBSONbot review — tip 29f6c05 / firmware d6bc0fe (2026-10-15 ~8:34 PM ET)

Reviewed on box `/tmp/bca` at `origin/grok/cyd-touch-port` tip `29f6c05` (firmware `d6bc0fe`). Read `botcorps_ui.cpp`, `botcorps_cyd_osk.h`, band clip, display/main/sd restores, `lgfx_cyd.h`, host OSK test, mocks companion slot, REPORT. Did **not** rebuild (used REPORT memory). Nothing flashed or merged. This commit is TASKS.md review notes only.

### What this batch actually added (diff 9da133b..d6bc0fe)
- Shared OSK table `src/ui/botcorps_cyd_osk.h` with real BKSP/ENTER/TAB/SYM/HIDE/SPACE; paint + hit share `cyd_osk_at` / `cyd_osk_hit`.
- Compact companion-slot badge `paint_cyd_badge` at (274,174) 42×60; full `paint_int` / `id_tick` skipped on CYD; `s_int_dirty` cleared; badge patched on bot change only.
- Focus-then-select touch for tiles/icons; crown→Gibson; RX closes sheet/OSK; sheet row/group/arrow hits; no final blanket `K_OK`.
- Band `botcorps_band_set_push_ymax` clips at `CYD_OSK_CLIP_Y` (151) while OSK open; Adv `VIEW_MAX` + boot/SD comments restored; `spi_3wire=false`; palette assert stays constexpr (recursive scan); host `cyd_osk_test.cpp`.

### Closed for real (code evidence)
| Item | Evidence |
|------|----------|
| R1 OSK action keys | `botcorps_cyd_osk.h` kinds + `read_key` 4430–4432; paint labels 4777–4782 |
| R2 `s_int_dirty` on CYD | `botcorps_ui_draw` 4849–4850 and 4882–4883 clear before/with `paint_int_now` |
| R3 badge not every loop | `cyd_overlay` 4794–4803 only on `s_badge_dirty` / bot change |
| R4 real badge (not stub) | `paint_cyd_badge` 1893–1916: `draw_blob`, short name, `LV 01`, `bot_color` |
| R5 OSK band clip | `botcorps_band_set_push_ymax` + push skip `world >= s_push_ymax` (band.cpp:490); caret clip `CYD_BADGE_X` |
| Slice C (mostly) | focus/select 4506–4517, 4542–4553; crown/RX 4438–4452; sheet 4455–4503 |
| Focused-tile silk CYD | 1271–1280 |
| Adv VIEW_MAX + comments | `botcorps_display.h` 64–65/121–122; `main.cpp` IR/SD/TCA/LoRa; `botcorps_sd.cpp` HSPI note |
| 4-wire bus | `lgfx_cyd.h:26` `spi_3wire = false` |
| Palette constexpr | `botcorps_pal_assert.inc` recursive `botcorps_pal_ids_ok`; still `static_assert` in pal.h |
| Bin untracked | only `builds/cyd/REPORT.md` in git; `*.bin` gitignored |
| Terminal 40 cmds | `CMD_N = 40`, G_NAME CORE/COMMS/GAME/SYS |
| Typing → `on_char` only | `on_char` 4348–4364; OSK chars → `K_CHAR` |

### Still open / overclaims / gaps
1. **REPORT/TASKS overclaim:** “Top-left is back off the terminal and the rain screen” — code **excludes** both (`read_key` 4556–4557). Top-left back is only other screens. Terminal leave = crown→Gibson or bot select.
2. **Badge tap opens OSK:** companion slot (274,174) sits in `ty >= 167` prompt zone (4534–4538); no badge hit-test. Tapping the ID opens the keyboard.
3. **Badge vs mock:** mock companion is **38×60** at (274,174) glyph-only; CYD is **42×60** with name+LV. Origin matches; not pixel-exact. Real INT ID card still absent by design (CYD = no INT).
4. **LV hardcoded** `"LV 01"` — not a live level (REPORT admits).
5. **Item 8 typing host test — PARTIAL:** `tools/host/cyd_osk_test.cpp` only checks the key table (passed locally). `botcorps_host_type_letters()` exists under `BOTCORPS_HOST_RENDER` but is not linked/run by that test. No automated “type every letter, screen stays Terminal” run.
6. **Items 9–11 device:** bot reply colors coded (`log_add_bot` / `\x01`); focused silk coded; rain still **full Adv** gap-6 (comment 377–378), not v11. Need glass.
7. **15 calibration:** NVS read path exists; defaults still 300..3900 placeholders.
8. **17 NimBLE:** listed once per env; Adv envs **not rebuilt** this tip (REPORT).
9. **18 CYD host-render:** host frames do not define `BOTCORPS_CYD` — no badge/OSK host frames.
10. **19 I2C keyboard:** unwired.
11. **OSK key legends** SPACE/BKSP/ENTER/TAB/HIDE/SYM are key labels (OK); no “tap”/“press ENTER”/“MAIN HUB” instruction strings found in firmware UI.

### Memory (from REPORT, not rebuilt)
- RAM: 72,076 / 327,680 (22.0%)
- Flash: 1,387,979 / 3,145,728 (44.1%)
- Frame: BANDS

### Next Super Grok paste order
1. **Badge hit + leave Terminal:** hit-test (274,174)–(316,234) so badge taps do not open OSK; add an explicit touch back (or document crown-only) and fix REPORT “top-left back” claim.
2. **Shrink badge to mock companion 38×60** (or Dirt-confirm 42×60); keep glyph/name/LV/color; still no INT card.
3. **Wire item 8:** host binary that calls `botcorps_host_type_letters()` and fails the build if nonzero; keep `cyd_osk_test` for the table.
4. **Optional P1:** v11 intro rain density (gap 14 / skip every third col) behind `BOTCORPS_CYD`; CYD host-render with `BOTCORPS_CYD` for badge+OSK screenshots.
5. **Device:** measure XPT2046 into NVS (`tcal=1`); glass vs mocks for colors/labels/rain; do not merge/flash without Dirt.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip 29f6c05 (fw d6bc0fe): (1) hit-test the companion badge so taps there do not open the OSK; (2) fix or remove the false “top-left back on Terminal/Rain” claim and give Terminal a clear touch back (or crown-only + REPORT honesty); (3) shrink badge to mock 38x60 unless Dirt wants 42; (4) add a host test that actually runs botcorps_host_type_letters() and fails CI if it returns nonzero; push only to grok/cyd-touch-port, no merge/flash. Match mocks/BOTcorps-Adv-FINAL-v1.html companion slot and CYD rules (no INT, touch nav, OSK, fixed bot colors, no instruction wording).
```

## Local progress (post-9d1a05e)
- Badge hit-tested. Dirt later asked for a badge tap to open the OSK; 49911f8/081595c do that. The badge is not display-only.
- Terminal top-left (x<36, y<18) is back to Gibson. Rain has no top-left back.
- Badge is the mock companion 38x60.
- Host renderer calls botcorps_host_type_letters() and fails if nonzero.


## Local progress (badge OSK + ESC)
- Badge tap (and lower terminal) opens the OSK again.
- ESC closes the OSK first and does not navigate while it is open.


## Local progress (badge open / ESC close)
- Explicit badge hit-test opens the OSK (checked before board tiles).
- ESC, HIDE, and RX all go through cyd_close_osk(); ESC does not navigate while the keyboard is up.


## GIBSONbot review — tip 2717130 (fw 081595c), 2026-10-15 9:05 PM ET
Reviewed fd6a544, 49911f8, 081595c against mocks/BOTcorps-Adv-FINAL-v1.html companion slot, Terminal v3.2 mock, and the CYD rules.

### Matched
- Badge is now 38x60 at (274,174), the mock companion size. Fixed position, bot's own color, no INT card.
- Terminal has a touch back: top-left (x<36, y<18) returns ESC to Gibson. Rain still has none. REPORT now says exactly that.
- ESC, HIDE and RX all close the keyboard through one `cyd_close_osk()`, and ESC does not navigate while the keyboard is up (typing/closing cannot jump screens).
- `render_main.cpp` calls `botcorps_host_type_letters()` first and exits nonzero on failure.
- No new instruction wording on screen.

### Fix notes (numbered)
1. **Badge tap reversed — needs Dirt's call.** Our note 1 asked that a badge tap NOT open the keyboard; fd6a544 did that, then 49911f8/081595c flipped it so the badge explicitly opens the OSK. If Dirt asked for this, keep it and say so in REPORT; if not, restore fd6a544 behavior (badge = display only).
2. **TASKS.md progress notes contradict each other.** "Local progress (post-9d1a05e)" still says badge taps return K_NONE and do not open the OSK; the code now does the opposite. Delete or correct that bullet.
3. **Typing test is only half real.** `render_main` runs it, but there is no build script or CI that builds/runs the host renderer, and the host build does not define `BOTCORPS_CYD`, so it tests the Adv `on_char` path, not OSK tap -> K_CHAR. Add a script (or CI job) that builds and runs it, and a CYD-defined case that types through `cyd_osk_at` hits. (GIBSONbot could not build the host renderer on the box: needs LovyanGFX font headers and stubs for `M5Cardputer.h`/`pgmspace.h`.)
4. **Memory not re-measured.** REPORT RAM 72,076 / Flash 1,387,979 are from an older tip. Rebuild `cyd` and update the numbers for 081595c.
5. Still open from earlier: LV hardcoded `LV 01`; boot rain is full Adv gap-6, not v11 intro; XPT2046 calibration still placeholder 300..3900; Adv envs not rebuilt; no CYD host-render frames (badge/OSK screenshots); I2C BlackBerry keyboard unwired; colors/labels/rain unchecked on glass.

### Follow-up one-liner (Dirt -> Super Grok)
```
On grok/cyd-touch-port tip 2717130 (fw 081595c): (1) confirm whether a badge tap should open the keyboard; if I did not ask for it, restore fd6a544 so the badge is display-only, and either way state it in REPORT; (2) fix the TASKS.md "post-9d1a05e" bullet that now contradicts the code; (3) add a script or CI job that builds and runs the host renderer so botcorps_host_type_letters() actually gates, plus a BOTCORPS_CYD case that types through OSK key hits; (4) rebuild the cyd env and update REPORT RAM/flash for this tip; push only to grok/cyd-touch-port, no merge/flash.
```

## Local progress (Super Grok session, continue)
- Badge tap opens the OSK. Dirt asked for this; kept and stated in REPORT.
- Contradicting post-9d1a05e TASKS bullet corrected.
- CYD boot rain is v11 density (gap 14, every third column off). Terminal still dry.
- tools/host/run_cyd_type_test.sh builds and runs the OSK table test and a key-hit typing gate. Passed here.
- pio is not in this session; RAM/flash not re-measured.


## Local progress (Super Grok rebuild)
- pio run -e cyd SUCCESS in this session.
- RAM 72,076 / 327,680 (22.0%). Flash 1,387,919 / 3,145,728 (44.1%).
- Bin not tracked.



## GIBSONbot review — tip a6ede9a (2026-10-15 ~9:36 PM ET)

Reviewed on box `/tmp/bca` at `origin/grok/cyd-touch-port` tip `a6ede9a0c3d539fcf38fb20f9a2a1ead2440b40d` (last-seen `18c3962`). Read per-commit diffs, `botcorps_ui.cpp` badge/OSK/touch, `botcorps_rain.cpp` CYD gap, `tools/host/cyd_type_test.cpp` + ran `run_cyd_type_test.sh` (PASS), `builds/cyd/REPORT.md`, `mocks/CYD-current-interactive.html`, Terminal v3.2 + FINAL-v1 presence. Did **not** rebuild firmware (used REPORT memory from fd972ea). Nothing flashed or merged by this watch. This commit is TASKS.md review notes only.

### Commits in this batch (oldest→newest)
| SHA | Author | What |
|-----|--------|------|
| 2a6a391 | Grok \<jankerton@example.com\> | REPORT: Super Grok session build line |
| e040ee7 | Grok | Firmware: keep badge→OSK; CYD rain GAP=14 + every-3rd OFF; host `cyd_type_test` + `run_cyd_type_test.sh`; TASKS local progress |
| 6b0b2d1 | Grok | REPORT tip → e040ee7 |
| fd972ea | Grok | Rebuild note: RAM 72076 flash 1387919; TASKS rebuild bullets |
| 8babd84 | Grok | REPORT tip → fd972ea |
| 96f7b2d | Grok | Add `mocks/CYD-current-interactive.html` |
| a80b1a2 | Grok | REPORT note interactive mock |
| a6ede9a | Grok | Mock: % layout for phone width; wrap stage |

All eight authors/committers are **Grok \<jankerton@example.com\>** (Super Grok). **No Cursor Agent / non-Super-Grok committers.** Branch only; no merge.

### What this batch actually fixed
1. **Badge→OSK kept + documented (Dirt rule).** Code still opens OSK on companion hit (274,174 38×60) before tile focus; REPORT + TASKS say Dirt asked for it. Closes prior “confirm badge tap” ambiguity.
2. **Item 11 rain density (code).** CYD `#ifdef`: `GAP=14`, force `KIND_OFF` when `(i%3)==2`; Terminal/lists/diag still dry via `wants_rain()`. Adv stays gap-6. Not verified on glass.
3. **Item 8 typing gate (host, CYD OSK path).** `tools/host/cyd_type_test.cpp` walks `cyd_osk_hit`/`cyd_osk_at`/`cyd_osk_char`; `run_cyd_type_test.sh` builds+runs table test + type gate. **Verified on box: both PASS.** Does not build full host renderer / `botcorps_host_type_letters()` under `BOTCORPS_CYD` (still needs LovyanGFX/M5 stubs).
4. **Memory re-measured.** REPORT: RAM **72,076** / 327,680 (22.0%), flash **1,387,919** / 3,145,728 (44.1%) — flash −60 B vs prior 1,387,979. Bin still untracked.
5. **Interactive CYD mock added** (`mocks/CYD-current-interactive.html`) for Dirt review of current behavior (rain→Gibson→Terminal, focus/select, badge OSK, sheet, OSK keys). a6ede9a makes layout %-based for phone width.

### MATCH (vs CYD rules / mocks — code evidence)
- Single 320×240, no INT paint on CYD (compact badge only) — still true
- Touch main nav: focus-then-select, crown/RX/sheet/group/arrows, Terminal top-left→Gibson, Rain no top-left back
- OSK shared table; badge tap opens OSK (Dirt)
- Fixed per-bot colors; badge uses `bot_color`; Terminal `log_add_bot` + `\x01` ink path
- Companion badge 38×60 at (274,174); no instruction strings found in firmware UI paint
- Terminal 40 cmds CORE/COMMS/GAME/SYS (`CMD_N=40`)
- Typing via OSK → chars only; host type gate PASS
- Memory reported and rebuilt this session
- Super Grok-only authors; push only on `grok/cyd-touch-port`

### MISS / still open / overclaims
1. **REPORT tip lag:** tip field is `96f7b2d`; real tip is `a6ede9a`. Bump tip on next REPORT edit.
2. **Interactive mock is a review aid, not pixel-faithful FINAL-v1 / v3.2:**
   - On-panel text: `"TAP TO ENTER"`, `"‹ back"`, `"CROWN"`, `"TERMINAL"`, `"WIFI"`, `"BLE"`, `"CMDS"` — violates “no instruction/direction text on screen” if treated as the CYD UI mock (side-panel docs are fine).
   - Sheet is a simple scroll list, not the prompt-anchored Tab dropdown look of `BOTcorps-Adv-FINAL-v3.2-terminal-commands.html`.
   - First tile tap sets **both** focus and `selected` (firmware: focus first, select on second).
   - Subtitle still says tip `fd972ea`.
3. **LV still hardcoded** `"LV 01"` on badge (firmware + mock).
4. **Device / glass still unchecked:** XPT2046 300..3900 placeholders; bot reply colors, focused silk, intro rain density, OSK/badge look vs FINAL-v1/v3.2 — code only.
5. **P1 still open:** Adv envs not rebuilt (NimBLE); no `BOTCORPS_CYD` host-render frames for screenshots; I2C BlackBerry keyboard unwired.
6. **Full host renderer** still not gated by CI under CYD defines (script covers OSK table + char production only).

### Memory (from REPORT fd972ea rebuild; tip a6ede9a is mock-only)
- RAM: 72,076 / 327,680 (22.0%)
- Flash: 1,387,919 / 3,145,728 (44.1%)
- Frame: BANDS

### Next Super Grok paste order
A. Bump REPORT tip to current SHA; strip on-panel instruction labels from `CYD-current-interactive.html` (icons/visuals only inside the 320×240 panel; keep side-panel docs); make tile first-tap focus-only; align command sheet closer to v3.2 prompt-anchored Tab dropdown; update mock subtitle tip.
B. Optional: live LV on badge (or REPORT “LV stub” forever).
C. Device: measure XPT2046 → NVS `tcal=1`; glass screenshots of every CYD screen vs mocks (Dirt asked for shots when build done).
D. P1 as spare: CYD host-render with `BOTCORPS_CYD`; Adv env rebuild confirm; I2C KB stub.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip a6ede9a: (1) bump REPORT tip to a6ede9a; (2) fix mocks/CYD-current-interactive.html so the 320x240 panel has no instruction text (drop TAP TO ENTER / ‹ back / CROWN/TERMINAL label words — icons only), first tile tap focuses only (select on second), and the command sheet looks more like Terminal v3.2 prompt-anchored Tab dropdown; (3) keep badge-opens-OSK, v11 rain, host type gate as-is; push only to grok/cyd-touch-port, no merge/flash. Device cal + glass shots later.
```

## GIBSONbot review — tip ef62516 (2026-10-15 ~10:04 PM ET)

Reviewed on box `/tmp/bca` at `origin/grok/cyd-touch-port` tip `ef625161865eaba49dccedb67be6c7e67a152500` (last-seen `13b101cfafef8855f99b3ce0bab5ff4a4b970a0f`). Diff-only review of mock + REPORT; firmware tree unchanged this tip. Did **not** rebuild or flash. Nothing merged. This commit is TASKS.md review notes only.

### Commits in this batch
| SHA | Author | What |
|-----|--------|------|
| ef62516 | Grok \<jankerton@example.com\> | Mock: drop on-panel instruction words (icons), focus-then-select, v3.2-ish sheet; REPORT tip → a6ede9a |

Author/committer **Grok** (Super Grok). **No Cursor Agent / non-Super-Grok committers.** Branch only; no merge.

### What this tip actually fixed
1. **No instruction words on the 320×240 panel.** Rain enter is a blank full-screen hit (no "TAP TO ENTER"). Gibson back / Terminal / Wi-Fi / BLE and Terminal back / crown are SVG icons. CMDS label gone; prompt tap opens the sheet (Tab-on-empty-prompt behavior).
2. **Focus then select.** First tile tap sets focus + silk only; second tap selects and logs. Matches firmware / prior TASKS ask.
3. **Sheet closer to Terminal v3.2.** Cyan border, name + purpose rows, 9-row windowed scroll, bottom caret, prompt-anchored open. CORE/COMMS/GAME/SYS headers kept.
4. **REPORT tip** bumped `96f7b2d` → `a6ede9a` (still one behind this tip).

### MATCH (vs CYD rules / mocks)
- Single 320×240 panel, no INT screen in the interactive mock
- Touch as main nav; icons instead of direction/instruction prose on panel
- Focus-then-select on board tiles
- Fixed per-bot colors; ID badge kept; badge tap → OSK
- Prompt-anchored command sheet with group headers + purpose blurbs
- Super Grok-only author; push only on `grok/cyd-touch-port`
- Firmware still `CMD_N=40` (unchanged this tip)

### MISS / still open
1. **Mock sheet has 33 commands, not 40.** Dropped CORE `go` / `load` / `defend` / `sweep` / `roster` / `pulse` / `dig`. Firmware `CMD_N=40` and `BOTcorps-Adv-FINAL-v3.2-terminal-commands.html` still have all seven. Restore them in the interactive mock.
2. **REPORT tip lag:** tip field is `a6ede9a`; real tip is `ef62516`.
3. **Mock subtitle** still says tip `fd972ea`.
4. **RX** still literal chrome text on Terminal (Adv mock also has RX; minor vs prior instruction-word ban).
5. **LV** still hardcoded `"LV 01"` on badge.
6. **Not pixel-faithful FINAL-v1 / full v3.2** (no per-row command icons; Gibson hub is empty hit rect, not ESP32-chip look).
7. **Device / glass still unchecked;** P1 (Adv rebuild, CYD host-render shots, I2C KB) still open.
8. **Firmware unchanged** this tip — mock-only.

### Memory (unchanged; tip is mock-only)
- RAM: 72,076 / 327,680 (22.0%)
- Flash: 1,387,919 / 3,145,728 (44.1%)
- Frame: BANDS

### Next Super Grok paste order
A. Restore CORE `go`/`load`/`defend`/`sweep`/`roster`/`pulse`/`dig` in `mocks/CYD-current-interactive.html` so the sheet matches firmware `CMD_N=40` / v3.2.
B. Bump REPORT tip and mock subtitle to `ef62516`.
C. Optional: icon-ify or keep RX as Adv-style chrome; live LV on badge.
D. Device: XPT2046 → NVS `tcal=1`; glass screenshots of every CYD screen when build is done.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip ef62516: (1) restore CORE cmds go/load/defend/sweep/roster/pulse/dig in mocks/CYD-current-interactive.html so the sheet has all 40 like firmware CMD_N / v3.2; (2) bump REPORT tip and mock subtitle to ef62516; (3) keep icon-only panel, focus-then-select, v3.2-ish sheet; push only to grok/cyd-touch-port, no merge/flash. Device cal + glass shots later.
```



## GIBSONbot review — tip 0901bb0 (2026-10-16 ~8:07 AM ET)

Reviewed on box `/tmp/bca` at `grok/cyd-touch-port` tip `0901bb003ecc16ef113d9fa7a63eb7a0e7de7e7d` (last reviewed pre-rebase tip `ef62516` / TASKS `def0d73`). Read `builds/cyd/REPORT.md`, TASKS tail, diffs `def0d73..HEAD` (esp. `src/ui/botcorps_ui.cpp`, `mocks/CYD-current-interactive.html`, `tools/host/*`, `mocks/shots/cyd/*`, `mocks/cyd-frames/*`), compared CYD `#ifdef` density to pre-rebase `a28caec`, viewed PNGs with Read, ran `tools/host/run_cyd_type_test.sh` (**PASS**). Did **not** rebuild firmware or flash. Nothing merged or pushed by this watch. Parent will commit/push TASKS only.

### Commits in this batch (oldest→newest)
| SHA | Author | What |
|-----|--------|------|
| 41f1372 | Grok \<grok@x.ai\> | Mock: restore CORE go/load/defend/sweep/roster/pulse/dig (40 cmds); REPORT/subtitle bump toward ef62516 |
| 000fca0 | Grok \<grok@x.ai\> | Rebase paint onto Adv `07d4fa39`; rewrite mock to PNG frames; host stubs + `render_cyd.sh`; side-by-side sheet |
| 0901bb0 | Grok \<grok@x.ai\> | Copy frames to `mocks/shots/cyd/`; REPORT tip → `000fca0` |

All three authors/committers are **Grok \<grok@x.ai\>** (Super Grok session lines). Email differs from older Super Grok commits (`jankerton@example.com`) but is still Grok / Super Grok — **no Cursor Agent committers.** Branch only; force-updated/rebased onto Adv tip `07d4fa39`. No merge.

### What this batch actually fixed / attempted
1. **41f1372 — CORE cmds restored in the CSS interactive mock** (verified: that commit’s HTML listed all 40 names including go/load/defend/sweep/roster/pulse/dig). Closed prior MISS #1 *for that tip only*.
2. **000fca0 — Adv paint rebase claimed.** `botcorps_ui.cpp` was rewritten (−369 / +128 vs `a28caec`). Shared Adv paint kept; **most CYD-only touch/OSK/INT-skip paths were dropped** (see MISS). Compact badge paint + focus-only silk labels remain behind `#ifdef BOTCORPS_CYD`.
3. **Mock rebuilt as frame flipper** (`mocks/CYD-current-interactive.html` ~113 lines): hit overlays for tiles / badge→OSK / Tab / back; images from `cyd-frames/*.png`. Side docs claim 40 cmds / no instruction text / focus-then-select.
4. **0901bb0 — shots copy + REPORT tip bump** to `000fca0` (still one behind HEAD).

### MATCH (vs CYD rules — code evidence)
- Firmware still `CMD_N = 40` with CORE `go`/`load`/`defend`/`sweep`/`roster`/`pulse`/`dig` in `CMDN[]` — `src/ui/botcorps_ui.cpp` (~L137–145).
- Compact badge paint exists: `paint_cyd_badge` at (274,174) 38×60, hardcoded `"LV 01"`, `bot_color` / ivory — `#ifdef BOTCORPS_CYD` (~L2025–2050, called from `paint_prompt` ~L2133–2135).
- CYD board silk name only when focused — `#ifdef BOTCORPS_CYD` (~L1328–1333).
- Rain gap still CYD-thinned: `src/rain/botcorps_rain.cpp` `GAP=14` + `(i%3)==2 → KIND_OFF`; Terminal/lists/diag dry via `wants_rain()` (excludes `SCR_TERMINAL`).
- OSK key table header still present: `src/ui/botcorps_cyd_osk.h` (`cyd_osk_at` / `cyd_osk_hit` / `cyd_osk_char`).
- Host OSK table + type gate script: `tools/host/run_cyd_type_test.sh` **PASS** on box (`cyd osk table ok keys=42`; `host cyd type gate passed`). Does **not** exercise full `botcorps_host_type_letters()` under `BOTCORPS_CYD` paint.
- Single-panel display wiring still in headers: `botcorps_display.h` CYD path — menu/content same `g_cyd`, 320×240.
- Super Grok-only authors; work only on `grok/cyd-touch-port`.
- Interactive mock panel has **no CSS instruction prose** inside the 320×240 stage (side-panel docs only) — `mocks/CYD-current-interactive.html`.

### MISS / still open / overclaims (numbered)
1. **CRITICAL — rebase dropped CYD touch / OSK / INT-skip from `botcorps_ui.cpp`.** Pre-rebase `a28caec` had **22** `BOTCORPS_CYD` hits (getTouch edge debounce, badge→OSK, `paint_osk`, `cyd_close_osk`, `cyd_overlay`, `botcorps_band_set_push_ymax` while OSK open, `paint_int_now` early-return on CYD). HEAD has **5** (include, unused OSK state vars, focus silk, `paint_cyd_badge`, badge call). `s_osk_open` is **declared once and never read/written** elsewhere. `read_key` is **M5Cardputer.Keyboard only** (~L5014+) — no `getTouch` path. On CYD, `paint_int_now` **no longer returns early**, so INT `fillScreen` can hit the same 320×240 glass (`botcorps_dual_ok()` is true on CYD). **CYD-only bits were not preserved cleanly through the Adv `07d4fa39` rebase.**
2. **CRITICAL — `BOTCORPS_HOST_RENDER` host helpers corrupted in the same file.** Brace imbalance `{`−`}` = **+2**. First `botcorps_host_hub` (~L5666) never closes before a broken nested `botcorps_host_type_letters` (body is gchat paint using undefined `buf`, **no `return`** — was a typing gate returning 0/1 on `a28caec`). Duplicate defs of `botcorps_host_intro` / `gmenu` / `jackin` / `scanall*` / `stale_sweep` / `keyfob` / `hub`. Device build may skip this `#ifdef`, but host render / type_letters gate claimed in REPORT is broken in-tree.
3. **Overclaim — “real paint frames” / “mock from real frames”.** `mocks/cyd-frames/cyd-*.png` and `mocks/shots/cyd/cyd-*.png` are **identical placeholder 320×240 dark frames** with corner labels (`CYD terminal 320x240`, etc.); only `cyd-badge` adds a small `"LV 01"` box. `tools/host/render_cyd.sh` itself prints *“LovyanGFX fonts not on this host; skipping live paint dump”* and only checks files exist. Side-by-side `adv-vs-cyd-side-by-side.png` is the same placeholders. **Not verified as host paint; not pixel-faithful to FINAL-v1 / v3.2.**
4. **Mock 40-cmd sheet regression.** 41f1372 restored interactive 40 cmds; **000fca0 replaced the mock with a frame flipper** that has **zero command names** in HTML (only a trace string “Tab → sheet (40 cmds)” and a blank `cyd-sheet.png`). Prior MISS #1 is **re-open** for the tip under review.
5. **REPORT tip lag:** tip field is `000fca0`; real tip is `0901bb0`.
6. **Mock subtitle lag:** still says tip `ef62516` (`mocks/CYD-current-interactive.html` L31).
7. **REPORT overclaims vs tree:** claims touch focus/select, badge opens OSK, OSK paint, typing via `on_char` from OSK, no full INT on CYD, “Mock rebuilt from those PNG frames” / “No CSS-drawn screens” as if frames were real paint. Code evidence contradicts the touch/OSK/INT claims on HEAD; frames are stubs.
8. **LV** still hardcoded `"LV 01"` in `paint_cyd_badge` (and placeholder badge PNG).
9. **Memory numbers stale for this tip.** REPORT still RAM **72,076** / flash **1,387,919** from an earlier `pio run -e cyd`. Tracked `.pio/build/cyd/firmware.bin` mtime **2026-10-15 18:13 ET** (pre-rebase). **No rebuild verified this session** after `000fca0` ui.cpp rewrite.
10. **Device / glass still unchecked;** XPT2046 300..3900 placeholders; P1 (Adv env rebuild, real CYD host-render shots, I2C KB) still open. Never claim field PASS / flashed / merged.

### Prior open items (ef62516) — status
| # | Was | Now |
|---|-----|-----|
| 1 | Mock 33≠40 cmds | Briefly fixed in 41f1372; **re-broken** by 000fca0 frame mock |
| 2 | REPORT tip lag | Still lagging (`000fca0` vs `0901bb0`) |
| 3 | Mock subtitle lag | Still `ef62516` |
| 4 | RX chrome text | N/A on blank placeholders; not verified on glass |
| 5 | LV hardcoded | Still open |
| 6 | Not pixel-faithful | **Worse** — placeholders, not FINAL-v1/v3.2 |
| 7 | Device/glass; P1 | Still open |
| 8 | Firmware unchanged (mock-only) | Firmware **changed** this batch but **regressed** CYD paths |

### Memory (from REPORT; not re-measured this watch)
- RAM: 72,076 / 327,680 (22.0%) — **stale vs post-rebase tree**
- Flash: 1,387,919 / 3,145,728 (44.1%) — **stale**
- Frame: BANDS
- Host type test (OSK table path only): **PASS**

### Next Super Grok paste order
A. **Restore CYD firmware paths lost in the Adv rebase** into `src/ui/botcorps_ui.cpp` from `a28caec` (or equivalent): `read_key` getTouch + focus-then-select + badge→OSK + sheet/OSK hits; `paint_osk` / `cyd_close_osk` / `cyd_overlay`; `paint_int_now` CYD early-return; `botcorps_band_set_push_ymax` while OSK open; keep Adv paint for shared screens. Confirm `pio run -e cyd` builds.
B. **Fix `BOTCORPS_HOST_RENDER` corruption:** close braces; restore real `botcorps_host_type_letters` typing gate; remove duplicate host_* defs.
C. Bump REPORT tip + mock subtitle to **this** tip SHA; rewrite REPORT so it does not claim touch/OSK/INT-skip/real frames until code matches; re-measure RAM/flash after A.
D. Replace placeholder `mocks/cyd-frames/*.png` with real host dumps **or** restore the CSS interactive mock with all **40** cmds (v3.2 sheet) until host CYD render works — do not claim “real paint” for labeled stubs.
E. Device later: XPT2046 → NVS `tcal=1`; glass shots. No merge/flash until Dirt says.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip 0901bb0: rebase onto Adv 07d4fa39 dropped CYD touch/OSK/INT-skip from botcorps_ui.cpp (s_osk_open unused; read_key is M5-only; paint_int_now no CYD return) and corrupted BOTCORPS_HOST_RENDER (broken type_letters, duplicate host_*, brace imbalance); mocks/cyd-frames and shots/cyd are labeled placeholder PNGs not real paint, and the interactive mock lost the 40-cmd sheet after 41f1372. (1) restore CYD paths from a28caec (getTouch, badge→OSK, paint_osk, INT skip, push_ymax) while keeping Adv paint; (2) fix host-render helpers + real type_letters; (3) bump REPORT tip/subtitle to HEAD, stop claiming real frames until dumps exist, re-measure RAM/flash; (4) restore 40-cmd mock sheet or real host frames. Push only to grok/cyd-touch-port, no merge/flash.
```


## GIBSONbot review — tip 1c1ea2a (2026-10-16 ~8:42 AM ET)

Reviewed on box `/tmp/bca` at `grok/cyd-touch-port` tip `1c1ea2a53d2ebf9a109e512b5ddbe0788cd2f26d` (last reviewed SHA `1364c1d` = TASKS after tip `0901bb0`). Diff span `1364c1d..1c1ea2a`. Host type test **PASS**. No firmware rebuild (preprocessor broken — see below). Nothing merged/pushed/flashed by this watch.

### Commits
| SHA | Author | What |
|-----|--------|------|
| 53fb723 | Grok \<grok@x.ai\> | Restore CYD touch/OSK/INT-skip; fix host helpers; honest REPORT; 40-cmd mock |
| 9106580 | Grok \<grok@x.ai\> | REPORT tip → 4f573cb |
| 1c1ea2a | Grok \<grok@x.ai\> | REPORT tip → 9106580 |

Super Grok only. No Cursor Agent.

### Fixed this batch
1. CYD touch/OSK/INT-skip **bodies** restored (getTouch, badge→OSK, paint_osk, INT early-return, push_ymax).
2. HOST_RENDER / `type_letters` **fixed** (prior CRITICAL #2 closed).
3. Interactive mock **40-cmd sheet restored**.
4. REPORT admits placeholders + stale memory.

### MATCH
- CMD_N=40 + CORE go/load/defend/sweep/roster/pulse/dig
- Compact 38×60 badge, focus silk, wants_rain Terminal dry, OSK table, host type test PASS
- Mock 40 cmds; side-panel docs only (no instruction text in 320×240 stage)
- Host helpers clean

### MISS / still open
1. **CRITICAL compile break:** orphan `#endif` after `cyd_close_osk` (~L4131); mangled `return K_NONE;#endif` (~L5279) leaves `#ifdef BOTCORPS_CYD` unterminated.
2. `paint_osk` outside CYD guard.
3. `cyd_overlay` + `on_esc` OSK-close still missing vs a28caec.
4. REPORT tip lag (`9106580` vs `1c1ea2a`); mock subtitle still `ef62516`.
5. Frames still placeholders; LV `"LV 01"`; memory stale; device/glass open.

### Next Super Grok order
A. Fix ifdefs so `pio run -e cyd` builds (wrap OSK funcs; own-line `#endif` on read_key).
B. Restore `cyd_overlay` + CYD `on_esc` OSK-close.
C. Bump REPORT tip + mock subtitle to HEAD; re-measure RAM/flash.
D. Real frames or keep CSS mock — no “real paint” claims for stubs.
E. Device later. No merge/flash until Dirt says.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip 1c1ea2a: touch/OSK/INT-skip bodies and host type_letters are back and the 40-cmd mock is restored, but botcorps_ui.cpp will not compile — orphan #endif after cyd_close_osk (~L4131) and mangled `return K_NONE;#endif` in read_key (~L5279) leave #ifdef BOTCORPS_CYD unterminated; paint_osk is also outside the CYD guard. (1) wrap cyd_close_osk+paint_osk in #ifdef BOTCORPS_CYD and put #endif on its own line after the CYD read_key branch; (2) restore cyd_overlay + on_esc OSK-close from a28caec; (3) bump REPORT tip and mock subtitle to HEAD, re-measure RAM/flash after a clean pio run -e cyd; (4) keep placeholder honesty until real frames exist. Push only to grok/cyd-touch-port, no merge/flash.
```

## GIBSONbot review — tip cbc040e / fw acfc0e3 (2026-10-16 ~9:35 AM ET)

Reviewed on box `/tmp/bca` at `grok/cyd-touch-port` tip `cbc040e70348a4be811d5d1872782c3318f23666` (last reviewed SHA `1c1ea2a53d2ebf9a109e512b5ddbe0788cd2f26d`). Diff span `1c1ea2a..cbc040e` for `src/ui/botcorps_ui.cpp`, `builds/cyd/REPORT.md`, `mocks/CYD-current-interactive.html`. Host type test **PASS** (`cyd osk table ok keys=42`; `host cyd type gate passed`). Preprocessor balance **OK** (no orphan `#endif`, no same-line mangled `#endif`). Did **not** re-run `pio` here (`pio`/`platformio` not on PATH; local `.pio/build/cyd/firmware.bin` still mtime **2026-10-15 18:13 ET**, size **1,395,584** — REPORT's new flash figure is Super Grok's claim only). Nothing merged, pushed, or flashed by this watch. Parent will commit/push TASKS only (includes prior unpushed 1c1ea2a addendum).

### Commits in this batch (oldest→newest)
| SHA | Author | What |
|-----|--------|------|
| acfc0e3 | Grok \<grok@x.ai\> | Fix `#ifdef` guards around OSK/close; restore `cyd_overlay` + `on_esc` OSK-close; move `push_ymax` to draw start |
| cbc040e | Grok \<grok@x.ai\> | REPORT tip → `acfc0e3`; re-measured RAM/flash after clean `pio run -e cyd` |

Both authors/committers **Grok \<grok@x.ai\>**. **No Cursor Agent.** Branch only. No merge.

### What this batch actually fixed
1. **CRITICAL compile break closed.** `#ifdef BOTCORPS_CYD` wraps `cyd_close_osk` + `paint_osk`; `#endif` on its own line after the CYD `read_key` branch (replaces mangled `return K_NONE;#endif`). File-wide `#if`/`#endif` stack balances to 0.
2. **`cyd_overlay` restored** (after `patch_world`) and called from CYD draw paths; OSK paints on dirty while open; badge patch when bot changes.
3. **`on_esc` CYD OSK-close restored** — ESC/top-left closes keyboard instead of navigating while `s_osk_open`.
4. **`botcorps_band_set_push_ymax`** moved to start of `botcorps_ui_draw` (no longer only inside `paint_prompt`).
5. **Mock subtitle** bumped `ef62516` → `acfc0e3`. REPORT claims clean `pio run -e cyd` SUCCESS; flash **1,404,031** (was 1,387,919); RAM still **72,076**.

### MATCH (vs CYD rules — code evidence)
- Preprocessor / Adv `#ifdef` hygiene for OSK helpers — **MATCH** (prior CRITICAL #1/#2 closed).
- `cyd_overlay` + `on_esc` OSK-close — **MATCH** vs prior MISS #3/#4.
- `CMD_N = 40` with CORE `go`/`load`/`defend`/`sweep`/`roster`/`pulse`/`dig`.
- Compact badge `paint_cyd_badge` (274,174) 38×60, `"LV 01"`, `bot_color`; `paint_int_now` CYD early-return (no INT card on glass).
- Focus-only silk; Terminal/lists/diag dry via `wants_rain()`; OSK table clip y=151; host type gate **PASS**.
- Mock: all **40** cmds in `GROUPS`; instruction prose **side-panel only** (not inside 320×240 stage).
- Single 320×240 CYD path; touch edge ~50 ms; badge→OSK; Super Grok-only authors.
- `BOTCORPS_CYD` line hits **7 → 15** (still below `a28caec`'s **22**, but overlay + esc + draw guards are back).

### MISS / still open (numbered)
1. **REPORT tip lag (pattern):** field is `acfc0e3`; real tip is `cbc040e`. Tip-bump commits never name themselves.
2. **Mock subtitle lag:** same — `acfc0e3` not `cbc040e` (minor; tracks code tip).
3. **Memory not re-verified on this box.** REPORT flash **1,404,031** / RAM **72,076** after claimed clean pio; local `firmware.bin` still Oct 15 and size **1,395,584** ≠ reported flash. Treat numbers as Super Grok-reported until a watch can run `pio run -e cyd` here or Dirt confirms the bin.
4. **Frames still placeholders** in `mocks/cyd-frames/` / `mocks/shots/cyd` — REPORT honesty kept (**MATCH** on honesty; real dumps still open).
5. **LV** still hardcoded `"LV 01"` in `paint_cyd_badge`.
6. **`BOTCORPS_CYD` guard count 15 vs `a28caec` 22** — thinner than the reference tip; watch for any Adv-only path that still assumes dual INT/EXT when CYD is defined (spot-check on next code touch, not a compile blocker now).
7. **Device / glass still unchecked;** XPT2046 300..3900 placeholders; no merge/flash. Never claim field PASS.

### Prior open items (1c1ea2a) — status
| # | Was | Now |
|---|-----|-----|
| 1 | CRITICAL preprocessor break | **MATCH — fixed** |
| 2 | `paint_osk` outside CYD guard | **MATCH — fixed** |
| 3 | `cyd_overlay` missing | **MATCH — restored** |
| 4 | `on_esc` OSK-close missing | **MATCH — restored** |
| 5 | REPORT tip lag | Still lagging (now `acfc0e3` vs `cbc040e`) |
| 6 | Mock subtitle lag | Bumped to `acfc0e3` (still one behind HEAD) |
| 7–10 | Frames / LV / memory / device | Frames+LV+device open; memory claimed refreshed but not box-verified |

### Memory (from REPORT; not re-measured this watch)
- RAM: 72,076 / 327,680 (22.0%)
- Flash: 1,404,031 / 3,145,728 (44.6%) — **Super Grok-reported**
- Frame: BANDS
- Host type test (OSK table path): **PASS**

### Next Super Grok paste order
A. Bump REPORT tip + mock subtitle to **this** tip SHA (`cbc040e` or whatever HEAD is after the bump).
B. Keep placeholder honesty until real host CYD dumps exist; optional: publish the `.pio/build/cyd/firmware.bin` size next to the map flash figure so watches can cross-check without pio.
C. Soft: live `LV` from bot state instead of `"LV 01"`; continue closing remaining CYD `#ifdef` gaps vs `a28caec` only if a real Adv/CYD conflict shows up.
D. Device later: XPT2046 → NVS `tcal=1`; glass shots. No merge/flash until Dirt says.

### Follow-up one-liner (Dirt → Super Grok)
```
On grok/cyd-touch-port tip cbc040e (fw acfc0e3): ifdef/OSK compile break is fixed, cyd_overlay + on_esc OSK-close are back, host type gate still PASSes, and mock subtitle is on acfc0e3. Remaining: (1) bump REPORT tip and mock subtitle to HEAD (cbc040e), not one tip behind; (2) keep placeholder-frame honesty; optionally cite firmware.bin byte size next to the map flash figure so watches can cross-check; (3) soft — live LV instead of "LV 01"; (4) device/glass later. Push only to grok/cyd-touch-port, no merge/flash.
```
