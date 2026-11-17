# Big-bot pager — the new home screen (MOCK)

2026-11-17. Mock only. The five locked v3.1 big-bot stills become a swipeable
home pager. Nothing here is firmware, nothing is flashed, and
`backup/cyd-fw-3locks-2026-11-16` stays at `9e8a7a5`.

Output: `/workspace/out/bigbot-1116/pager/`
(also `mocks/bigbot-1116/pager/` on `backup/bigbot-mock-2026-11-16`).

The v3.1 stills at `/workspace/out/bigbot-1116/v3.1/` are locked as-is.
Grape pager pages are those stills with only the five page-indicator pips
added. Faces, panels, ticker text, pins, and chrome are otherwise
byte-identical. All CYD pixels come from the real C++ renderer (v3 compose +
`pager_pips.cpp`); there are no Pillow lookalike redraws of the board. @3x
stills and the mp4 are nearest-neighbour 720×960.

## Order, default, wrap

Left → right:

1. BOTpheus
2. GIBSONbot
3. CONTROLbot  ← default first page (changeable; `DEFAULT = 2` in
   `sim/render_pager.py` and in `pager.html`)
4. HACKbot
5. TERMINALbot

Swipe left on the top ticker / announcement border (y = 0–29, including the
pip band) goes to the next page. Swipe right goes to the previous page.
Arrow keys do the same in the HTML mock.

**No wrap.** At BOTpheus (left end) a right-swipe, or at TERMINALbot (right
end) a left-swipe, does an ~18 px edge bounce and springs back. Same 233 ms
ease-in-out cubic as a real slide. This was the pick: a wrap would jump
TERMINALbot to BOTpheus and hide that you had reached the end of the set.

## Page indicator

Five small AA discs in theme ink, in the 5-px black gap under the ticker
hatch and before the coil well: y = 25–29 inclusive, centres
(104, 27), (112, 27), (120, 27), (128, 27), (136, 27). Current page is a
slightly larger, brighter pip (~52 % of sampled ticker ink for the dim
pips; on-radius 1.75 px, off-radius 1.35 px). Pips only blend over
near-black pixels in that band.

The bottom chip row already has a decorative 13-square strip, so pips were
not placed there. They also stay off the ticker glyphs.

This is the only change versus the locked v3.1 grape stills. Measured at
native 240×320:

| page        | lit pip | changed px | bbox (inclusive)   | outside y=25–29 | in face box | in panel |
|-------------|---------|------------|--------------------|-----------------|-------------|----------|
| BOTpheus    | 0       | 98         | (99,25)–(137,29)   | 0               | 0           | 0        |
| GIBSONbot   | 1       | 98         | (102,25)–(137,29)  | 0               | 0           | 0        |
| CONTROLbot  | 2       | 98         | (102,25)–(137,29)  | 0               | 0           | 0        |
| HACKbot     | 3       | 98         | (102,25)–(137,29)  | 0               | 0           | 0        |
| TERMINALbot | 4       | 98         | (102,25)–(140,29)  | 0               | 0           | 0        |

Union of all pip pixels: (99,25)–(140,29). Face box (65,88)–(174,197) and
bottom panels LV_CB_P* (45,220)–(196,290) are untouched. BOTpheus’s bbox
starts at x=99 because pip 0 is lit (its glow extends slightly left);
TERMINALbot’s ends at x=140 because pip 4 is lit.

## Transition

A **full-frame** 240×320 horizontal slide, 7 frames at 30 fps = **233 ms**,
ease-in-out cubic (same timing for the edge bounce).

Why this, on a 240×320 ILI9341: each v3.1 / themed still is a complete
locked board (ticker, pins, chrome, face, panel). Sliding the whole frame
keeps that board attached to the page. A slide of only the well between the
pin rails would leave a seam after a theme cycle, when the ticker and pins
recolour with the page. Seven frames is long enough to read as a slide and
short enough that nearest-neighbour @3x (and native 240×320) stay crisp —
no interpolated blur of the pixel art.

## Center taps (storyboard callouts only — not painted on the CYD screens)

Target is still LV_CB_FACE (53,78)–(187,212). Locked table:

| page        | center tap                                      |
|-------------|-------------------------------------------------|
| BOTpheus    | Dojo EB3 (LV_SC_MO)                             |
| GIBSONbot   | Gibson scan tower / radar (LV_SC_RADAR)         |
| CONTROLbot  | cycles the theme (whole pager recolours)        |
| HACKbot     | HACKbot game (LV_SC_DL)                         |
| TERMINALbot | terminal screen (LV_SC_TERM)                    |

Next theme after NEON GRAPE (lv_theme=0, LV_CB slot 11) is HOLO FOIL
(slot 12), per locked v6 / `LV_CB_FW`. Holo pages keep each bot’s locked
v3.1 identity face (not reinked) and recolour the rest of the board,
including pips, from the real C++ composer + pip pass.

In the interactive HTML mock, a center tap on CONTROLbot actually cycles
grape ↔ holo so the whole pager recolours. The other four taps are labeled
only on the storyboard.

## Deliverables

1. `swipe_tour@3x.mp4` — 273 frames, 9.100 s, 30 fps, 720×960 nearest-
   neighbour, H.264 yuv420p. Sequence: hold CONTROLbot (grape) → swipe
   through HACKbot and TERMINALbot → edge bounce → back through HACKbot,
   CONTROLbot, GIBSONbot, BOTpheus → edge bounce → back to CONTROLbot →
   center-tap flash on the bot well → 200 ms dissolve to HOLO FOIL
   CONTROLbot → peek HACKbot holo and GIBSONbot holo → end on holo
   CONTROLbot.

2. `storyboard.png` — 3752×2490 contact sheet of the five grape pages
   (default badge and tap callouts in the caption row only, never on the
   CYD pixels) plus mid-swipe, edge bounce, and the grape → holo theme
   cycle including a recoloured HACKbot page.

3. `pager.html` — self-contained (inlined @3x PNGs as data URIs). Open it
   on an iPhone. Touch-swipe on the ticker area, or use the arrow keys, to
   flip pages. Bounce at the ends. On CONTROLbot, tap the center face to
   cycle the theme; every page in the strip recolours.

Sources: `sim/pager_pips.cpp` (C++ pip painter), `sim/render_pager.py`
(orchestrator, mp4, storyboard, HTML). Pixel proof: `diff_proof.json`.

## SHA-256

See `SHA256SUMS`.
