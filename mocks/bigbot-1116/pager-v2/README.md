# Big-bot pager v2 — BACKbot removed (MOCK)

2026-11-17. Mock only. Same five-page home pager as
`/workspace/out/bigbot-1116/pager/`, with the small pink BACKbot in the
bottom-right corner removed on every page and every theme, so this can
be the new BOTcorps home UI. Nothing is firmware, nothing is flashed,
and `backup/cyd-fw-3locks-2026-11-16` stays at `9e8a7a5`.

Output: `/workspace/out/bigbot-1116/pager-v2/`

## What changed vs pager v1

Only LV_R_BACKBOT `{221,301,239,319}` (19×19, inclusive). 275 px on
every still, grape and HOLO FOIL. 0 px outside that rect. Face, pips,
panels, ticker, pins, and the rest of the chrome are byte-identical to
pager v1.

The fill is composited from the same still’s already-rendered chrome
(no freehand redraw):

- 5V pad body from that still’s 3V3 pad (same kind-0 power pad, +28 px
  = two pin pitches). The two already-visible 5V pad rows (y=299–300)
  matched that body; only the covered rows y=301–304 were written. The
  visible 5V left nub at x=222 is continued through the pad face.
- Right mounting-hole ring from that still’s left hole, horizontally
  mirrored (centres x=8 ↔ x=231, y=311; ring y=308–314).
- Inner rule, theme side-edge, and gold rim from this still’s clean
  right rail (y=290) and from the intact left bottom corner (the model
  for y=316–319).

It reads as if the icon was never there: the 5V pad, hole ring, gold
bottom rim, and purple inner rule sit in the same places as the rest
of the board.

| theme | pages | changed px | bbox (inclusive) | outside formal rect |
|-------|-------|------------|------------------|---------------------|
| NEON GRAPE | all five | 275 | (221,301)–(239,319) | 0 |
| HOLO FOIL  | all five | 275 | (221,301)–(239,319) | 0 |

## Unchanged from pager v1

- Order: BOTpheus, GIBSONbot, CONTROLbot, HACKbot, TERMINALbot.
- Default first page is CONTROLbot (index 2, changeable).
- No wrap; ~18 px edge bounce, 233 ms ease-in-out cubic.
- Pips in y=25–29, theme ink, 5 discs.
- Full-frame 240×320 horizontal slide, 7 frames = 233 ms, ease-in-out cubic.
- Center-tap labels are storyboard callouts only (not on the CYD pixels).
  CONTROLbot still cycles NEON GRAPE → HOLO FOIL for the whole pager in
  the HTML mock.

## Deliverables

1. `swipe_tour@3x.mp4` — 273 frames, 9.100 s, 30 fps, 720×960 nearest-
   neighbour, H.264 yuv420p. Same tour as v1, now without BACKbot.
2. `storyboard.png` — 3752×2490 contact sheet.
3. `pager.html` — self-contained (inlined @3x frames). Ticker swipe +
   arrow keys; CONTROLbot face tap cycles the theme.

Sources: `sim/strip_backbot.py`, plus the v1 pip / tour sources
`sim/pager_pips.cpp` and `sim/render_pager.py`. Pixel proof:
`diff_proof.json`.

## SHA-256

See `SHA256SUMS`.
