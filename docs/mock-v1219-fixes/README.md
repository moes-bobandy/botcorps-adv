# mock/v1219-fixes-2026-11-21 (sim-only, not yet tested on the CYD)

Branch off bd6c320. No bin, upload, flash or merge.

## 1. CONTROLbot controls — not a source regression

v1.2.12 (fb46c13) added RECAL TOUCH / ROTATE / EXIT (LNCHR or OFF) and relabelled SCAN to RADAR.
The CONTROLbot panel and screen blobs are **identical** from fb46c13 through 9efd705, 89aef6d,
cda0390, dc98ba3, 514df65, 0513879, c70ef04, e8f6c91, bd6c320 and 61158ee:

- `src/live/controlbot/lv_controlbot_panel.h` blob `3a67ac48775cbced4c06bf9c0e54b614bd1d9b6f`
- `src/live/controlbot/lv_controlbot_screen.h` blob `c15ceb598f389fac23fdeea54d75543b97b079fc`

Host sim of bd6c320 (`page 3` / `go 16`) still shows RADAR RAIN, RECAL TOUCH, ROTATE PORT, EXIT OFF,
SLEEP, BAR HAZRD, BRIGHT. Proof: `/workspace/out/fw-bigbot-1117/v1219-fixes/controlbot-bd6c320/`.
No restore was applied (nothing was lost in git). If the board showed the old CHIME / TIME / TONE
labels, that is not this tree.

## 2. Dojo bottom bar opens TERMINALbot

Cause (unchanged in this branch's first commits): `src/live/lv_morpheus_v13.h` `itw_mo_tap`,
the branch `if (y >= 287) { lv_term_from_mo = true; lv_go_soon(LV_SC_TERM, 0); return "mo-terminal"; }`.

Added in a98a25b (v1.2.6, "Dojo footer opens TERM") when the TERMINALbot pager page was dropped.
The footer terminal is rows y 287..319, x 1..238 (`mo_terminal`). Any tap in that bar, including
empty charcoal, jumps to LV_SC_TERM. The banner/back path `backbot-term-dojo` then returns to the Dojo.

Proposed minimal fix (not applied here; Mobandy has not asked to change this route yet): drop or
gate that `y >= 287` branch so the footer is display-only, and let folder / `< TOWERS` hits stay
as they are (`mo_fold_hit`, `MO_BTN` at {170,247,239,285}).

## 3. Old TERMINALbot bot-screen (gold)

Last pager page: **7db0c6dcadc31c5a663227b0c3e5499321fdf5d3** (v1.2.5, "gold TERMINALbot").
Pager slot **LV_PG_TERMINAL = 3** in a 5-page order: BOTpheus 0, HACKbot 1, GIBSONbot 2,
TERMINALbot 3, CONTROLbot 4 (starts on GIBSONbot). Tapping the page opened LV_SC_TERM
(`pager-terminal`). Home-board chip 4 (`LV_HOME_CHIP[4] = {139,156,207,255}`) also still
routes to LV_SC_TERM in current firmware (`lv_core.h`); that chip path was not removed.

Look at 7db0c6d: metallic-gold remapped old HACKbot face (`lv_pg_gold`) + gold ATK / TRACE
meter panel (`lv_pg_pan_hackbot(..., 1)`). The shared rain routine was **not** applied on
that page (HACKbot had green Matrix rain; BOTpheus/GIBSON had white rain). Pitch-black rain
(`pal {0,0,0}`) was added in **a98a25b** (v1.2.6) to GIBSONbot in the same commit that
**dropped** the TERMINALbot page. There is no committed "gold face + black rain" TERMINALbot
page. Proof render from 7db0c6d: `/workspace/out/fw-bigbot-1117/v1219-fixes/terminalbot-7db0c6d/`.
Home layout was not changed.

## 4. Left/right d-pad restore (this commit)

v1.2.14 **89aef6d81ea4c023788e9ac0c70cef83960d6d62** replaced the v1.2.13 left/right rocker
(`pad_rocker` at 9efd705a559c199606294e78202948cb93f48f12) with a 4-way `pad_cross` on
GIBSON and HACKbot decks.

Restore (inside the strip only):

- GIBSON `pad_draw_joey`: verbatim `pad_rocker(..., 16, 68, ...)` + centre pip from 9efd705.
- GIBSON hit test: `PAD_HIT_L` / `PAD_HIT_R` boxes, not the v1.2.14 diagonal 4-way.
- HACKbot `pad_hc_deck`: the same rocker (A OPEN / B BACK arcade discs kept).
- HACKbot `lv_dl_deck_id`: L/R boxes return 3/4; A/B unchanged.

Kept on purpose (would change pixels or behaviour **outside** the strip, or drop locked
behaviour):

- INFO-tower pull-up (`LV_RD_INFO_TOWER_PULL`) and folder drop law: untouched.
- v1.2.14 folder theme frame on landed GIBSON folders (`lv_rd_fsel`): untouched (it draws
  outside the strip).
- v1.2.14 L/R-within-row `lv_rd_dpad_pick` when unlocked: kept, because restoring the older
  `cycle_next` / `cycle_prev` would change which tower is selected after L/R and paint
  pixels outside the strip.
- HACKbot game deck (`LV_DL_DECK=1`) is kept. A full revert to 9efd705 (panel + terminal,
  mock-only 4-way) would redraw the playfield, which fails the hard gate.

Conflict, reported not guessed: HACKbot used the 4-way UP/DOWN arms for slot pick, pause
menu, open-tower folder cursor, and field row jumps. Those actions stay in the code and
can still be reached by tapping the slot / menu / folder itself; they no longer have an
up/down arm on the rocker. The slots plaque still says "UP/DN PICK" (inside the strip).

Hard gate vs bd6c320: see `gate_dpad_lr_vs_bd6c320.txt`. Outside-strip diff is 0 on every
dumped screen. Renders: `/workspace/out/fw-bigbot-1117/v1219-fixes/dpad-lr-vs-bd6c320/`.
