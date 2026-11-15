# towers3: 3 new tower bodies (mock, Mobandy 7:03 PM ET)

**Mock only.** Nothing was flashed or merged. The v5f outputs in `../` are untouched. Everything here was built from a copy of the v5f tree and sources in /tmp/t3, by `src/towers3_body.py`.

**What changed:** only the T3 body (the 13 non-Matrix towers): front face, sides, cap, bands and edges, behind `#if MO_BODY`.

**What did not change:** the 5 Matrix towers (DEAUTH, HACK, SKIMMER, FLIPPER, EVIL TWIN: black glass with the CYD code rain), emblems, file rows, folders, open sequence, depth, drift and camera. These all run the v5f code. I did not run a pixel diff outside the bodies.

**Renders:** real 240x320 sim (lv_radar.h engine). Each look has:
- `idle_3x.png`, the same idle time as v5f;
- `closeup_3x.png`, FLOCK opened, 3.15 s after the tap;
- `idle_then_open_3x.mp4`, idle 4.3 s, then FLOCK opened, then back.

| look | body | what each part means |
|---|---|---|
| A obsidian monolith | Polished black stone with a 2 px chrome bevel on the lit side | The right edge and the cap's back edge are lit in the threat colour (HIGH red, MED amber, LOW green) |
| B server rack | Gunmetal chassis | The right rail is an LED column. The number of lit LEDs is the device's example signal level, and they flicker at 1 + 7 x signal Hz. The top LED is a steady threat lamp. |
| C crystal pillar | Cyan glass with a lit left facet | A light band rises through the pillar; its speed (15 + 60 x signal px/s) and brightness (0.35 + 0.65 x signal) come from the device's example signal. The top rim is the threat colour. |
