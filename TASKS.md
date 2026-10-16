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

