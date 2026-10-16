# NEXT BRIEF for Super Grok: Slice A (CYD compact ID badge)
Posted by GIBSONbot for Dirt, 2026-10-15 ~8:00 PM ET. Branch: `grok/cyd-touch-port`. Full open list: `TASKS.md`.

## Goal
On CYD, the INT ID card must stop drawing over the 320x240 screen. Replace it with a compact per-bot ID badge on the single screen. This closes TASKS item 2.

## Do
1. Under `#ifdef` for the CYD build, skip `paint_int()`, `paint_int_now()` and `botcorps_id_tick()` entirely. Remove the `setClipRect(220,0,100,60)` hack.
2. Add `paint_cyd_badge()`: a compact ID badge in a reserved region of the single screen (the right-hand panel beside the prompt, where the mock's bot tile sits). It shows the bot glyph, name, LV, and the bot's one fixed color.
3. The badge keeps one fixed layout and orientation. It never flips or reverses when cycling bots.
4. Repaint the badge only when the selected bot or LV changes, not every loop.
5. Board tiles and the Terminal must not draw over the badge region (clip their pushes to it).
6. Leave every Adv path unchanged. CYD changes go in `#ifdef` blocks only.

## Rules (do not break)
- Match the look of `mocks/BOTcorps-Adv-FINAL-v1.html` (ID badge) and `mocks/shots/`. Do not invent a new style.
- No instruction text on screen. Each bot keeps its fixed color. No rain behind the Terminal.
- Keep RAM in check. Report free heap before and after.

## Done when
- Builds for CYD and both Adv envs.
- `REPORT.md` tip and claims are true (say "badge done" only if it is).
- Commit message starts with `CYD slice A:`. Push to `grok/cyd-touch-port`.
- GIBSONbot reviews the push and updates `TASKS.md`.

## Then (in order)
B. Shared OSK rect table, real SPACE/BKSP/ENTER/hide keys, no per-loop repaint (items 5 to 7).
C. Touch focus-then-select, dropdown/top-bar/back hit-tests, no blanket `K_OK` (item 3).
D. Untrack `builds/cyd/botcorps-cyd.bin`, make REPORT honest (items 14 and 20).
E. Restore palette `constexpr` assert (item 1).
