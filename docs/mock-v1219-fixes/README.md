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

### Layer: LV_PG_TERMINAL_PAGE (default 1) — applied on this branch

Own droppable flag. Restores the 7db0c6d gold TERMINALbot page as slot 3 in the 5-page
order (BOTpheus, HACKbot, GIBSONbot, TERMINALbot, CONTROLbot). Face slot already was
x65..174 y88..197 (110×110) on gold, CONTROLbot and HACKbot — not scaled; metallic
bevel kept. Pitch-black rain cloned from a98a25b GIBSON (`pal {0,0,0}`, `skip=1`).
Centre tap → `pager-terminal` / LV_SC_TERM. Other pages 0 px vs branch head except
the 5-pip row and the first-page left arrow chip (HACKbot gains it, BOTpheus loses it).
Strip {1,247,238,319} stays 0. Details: `TERMINALBOT.md`, `gate_terminal_page_vs_head.txt`.
Proofs: `/workspace/out/fw-bigbot-1117/v1219-fixes/terminalbot-page/`.


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

## 5. Centre tower-style buttons (separate layer, LV_STRIP_TOWERS)

Own flag `LV_STRIP_TOWERS` (default 1; set 0 or revert this commit to drop it).
The plates clone the GIBSON folder-plate chrome (`lv_rd_fold1`): 3D lip, dark fill,
4 px scanlines, accent frame, short tab, 3x5 label — shrunk to the strip centre.

### Room (px) and what was placed

Strip bbox on pad screens: **x 1..238, y 247..319** (238 x 73). Plaque is y 249..264.
Left rocker housing: about x 6..94, y 268..316. A/B discs: 40 x 40 at (196,272)-(235,311)
and (152,272)-(191,311). Centre gap: **x 102..147 (46 px) x y 268..316 (49 px)**.

| Screen | Centre already occupied? | Buttons added | Existing actions | Touch size |
| --- | --- | --- | --- | --- |
| GIBSON | SPEED already sits in that gap (PAD_MID x106 w38, y270..314, 38 x 45). A LOCK / B BACK are 40 x 40 discs on the right. | **None.** Covering SPEED would drop an existing control. No leftover 36 px pocket beside it (~8 px each side). | — | — |
| HACKbot | After the rocker restore the gap only had L/R text. | **UP** plate `{102,271,147,291}` (46 x 21, tab y 265..270) and **DN** `{102,294,147,316}` (46 x 23). | The existing 4-way UP/DOWN actions: slot pick, pause menu, open-tower folder cursor, field row jump. A OPEN / B BACK stay on the right discs. L/R stays on the rocker. | Short axis 21 / 23 px, **under** the 36-40 px resistive floor. The 40 x 40 A/B discs remain the large targets. |
| Dojo | Idle: y 247..286 is empty black; footer terminal is y 287..319 (**33 px tall**, under the 36 px floor). Open pillar: `mo_panel` already fills **x 2..238, y 247..285** (236 x 39) with the pillar name, status, and existing `< TOWERS` (63 x 34 at 173,249). | **None.** Idle has no existing strip action except the y>=287 TERM route (not promoted). Open-pillar centre is already the name/status; folder actions already live on the right-hand folders (outside the strip). Adding plates would cover `mo_panel`. | — | Footer 33 px cannot host a 36 px target without extending into the floor (y<=246), which would fail the hard gate. |

Hard gate vs bd6c320: `gate_strip_towers_vs_bd6c320.txt`. Outside-strip 0 px on same-state frames.
Renders: `/workspace/out/fw-bigbot-1117/v1219-fixes/strip-towers-vs-bd6c320/`.

## 6. Dojo idle no-tear / faster pull-forward

The split happens during idle pillar sway (strip-by-strip SPI of a moving scene), not
at pull-forward. `LV_MO_COHERENT_PRESENT` (default 1) fences each Dojo frame so the
device-timeline panel latches one coherent image at present-complete. `MO_PUSH_FAST`
(default 1) shortens the pull ease 400→240 ms; completed poses stay 0 px vs c70ef04.
Details: `DOJO_COHERENT.md`, `gate_dojo_coherent_vs_c70ef04.txt`.
Proofs: `/workspace/out/fw-bigbot-1117/v1219-fixes/dojo-coherent/`.

