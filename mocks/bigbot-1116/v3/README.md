# Big-bot Color UI v6 mocks — v3 smooth instrument panels (MOCK ONLY)

Mobandy 2026-11-16 5:14–5:25 PM ET (America/New_York): keep every square bot face locked at the CONTROLbot v6 size and position, and redesign the four role panels as smooth, anti-aliased instrument graphics that fill the existing bottom plate. No more grid cells or boxed text cards. CONTROLbot keeps the locked v6 control rows exactly.

**Mock only. No firmware commits to the build branch, nothing flashed, nothing copied to an SD card.**

v2 (blocky role cards) remains at `../v2/` on this same branch as commit `5e9f8a1`. This folder is v3 only.

## Locked geometry (do not shrink or move the face)

- Face slot `THEME_FACE` inclusive **(65, 88)–(174, 197), 110×110** — identical to locked CONTROLbot Color UI v6.
- Face + black frame `LV_CB_BOT` inclusive (54, 79)–(185, 210). Names sit in the black pad at y = 200, LV_GLCD 5×7, ink (231, 239, 247).
- Bottom panel `LV_CB_P*` origin (45, 220), size 152×71, inclusive **(45, 220)–(196, 290)**. This is the only area redesigned per bot.
- Grey bezel rows y = 211–219 stay pixel-identical to locked v6 (0 px changed).

## Sources (verified)

- Firmware home spots: `src/live/lv_core.h` on `moes-bobandy/botcorps-adv`, worktree `/workspace/cyd-fw-3locks-wt` (`backup/cyd-fw-3locks-2026-11-16`). Base `049ff59`, build `c1b5659`, v1.1 rewires `ada2253`.
- Locked v6: `LOCK.md` at `a87fe22` on `backup/controlbot-colorui-2026-11-15`, `mocks/controlbot-colorui-1115/v6-cyd-ticker/`.
- Default theme: firmware `lv_theme = 0` → NEON GRAPE (`LV_CB` slot 11). Locked still sha256:
  `48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779`
- Instrument ink is sampled from the same `lv_cb_reink` roles as locked v6 (key / accent / dim), expanded with `<<3` / `<<2`. A control-panel selftest against the locked NEON GRAPE still is **0 px**.

Renderer: `sim/bigbot_v3_compose.cpp` (zig c++, `-target x86_64-linux-gnu.2.36 -O2 -std=gnu++17`, real `lv_draw.h` / `lv_font.h`). Faces are firmware C++ painters or NN-fitted locked sources, never Pillow lookalikes. Instruments are drawn with signed-distance coverage and **3× supersample** into a float buffer, then box-filtered to 1×, then alpha-blended; glow is additive in theme colours. Labels are crisp LV_GLCD, minimum 7 px tall (scale 1), with primary numbers at 14 px or 21 px. Unchanged pixels are copied byte-for-byte from the locked still.

## Faces

Unchanged from v2: CONTROLbot is the locked v6 theme face with no name plate. HACKbot is firmware `e_gold_face` + eyes at `s = 110` (metallic gold with sheen, never flat yellow). TERMINALbot is `e_silver_shell` + `e_eyes` only — clean silver, two eyes, **no `>_`**, name spelled `TERMINALbot`. BOTpheus and GIBSONbot are NN-fitted from the locked shades / official home crop. Theme variants do not recolor the gold or silver identity faces.

## Bottom panels (smooth instruments, unique per role)

All four new panels share a dark plate and a soft theme-coloured rim (no inner grid of boxes). Snapshot numbers are **EX.DATA** (stamped in 5×7 in a clear corner; never `#`).

| Bot | Panel (EX.DATA snapshot) | Why this panel fits the role |
|-----|--------------------------|------------------------------|
| **BOTpheus** | Circular radar orbit of 18 smooth pillar dots around a thinner guide ring; the locked pillar (index 4, `MO_DEV` “AIR TAG”, `itw_mo_sel`) glows with a faint beam. Center `18` / `SEEN`. One line underneath: `LAST AIR TAG`. | BOTpheus’s job is the Morpheus Dojo. This is a radar of the 18 pillars and the last lock, not a roster of boxed tags. |
| **GIBSONbot** | Large semicircular RSSI gauge, gradient arc with ticks at −90/−60/−30/0 dBm, needle at **−42 dBm**, rounded end caps. A short scan-activity sparkline in the left bowl. `WIFI 12` and `BLE 8` as large short numbers. | GIBSONbot is the scan / radar bot, so the plate is a signal instrument plus AP/BLE counts, not a pair of radio cards. |
| **HACKbot** | 270° TRACE power ring, green → amber → red, filled to **42%** (`lv_dl_trace`) with a glowing tip; `42` / `TRACE` in the well. Slim glowing `FILES 1/2` capsule (`lv_dl_got` / `need`). `L2` and `37.8S` small (`lv_dl_round`, `lv_dl_secs10`). | HACKbot is the download / trace game; this is a power-meter HUD, not a grid of level/time boxes. |
| **TERMINALbot** | Vertical threat VU tube (glass capsule, liquid fill at 0.40, red danger zone at the top — alerts stay red), four scale ticks, large `ALERTS 2`, and a smooth alert waveform along the bottom with two spikes. | TERMINALbot watches the threat log; the plate is a VU + waveform, not a hunter-cell table. |
| **CONTROLbot** | Locked v6 rows exactly: `SCAN B.RAIN`, `VOL 40%`, `BRIGHT 80%`, `CHIME ON`, `TIME RTC`, `BAR HAZRD`, `TONE BEEP`. | This screen already *is* the control deck; the lock is left 0-diff, including the panel. |

## Pixel diff vs locked v6 stills

Default-theme stills are diffed against `12_neon_grape@1x.png`. Allowed change regions: `LV_CB_BOT` (face + name) **and** the bottom panel (45, 220)–(196, 290). 0 px outside those two rects. 0 px in y = 211–219.

| bot | changed px | diff bbox (x0,y0,x1,y1) |
|-----|------------|-------------------------|
| BOTpheus | 21113 | (45, 88, 196, 290) |
| GIBSONbot | 20809 | (45, 88, 196, 290) |
| CONTROLbot | 0 | none (identical, including panel) |
| HACKbot | 20026 | (45, 88, 196, 290) |
| TERMINALbot | 20348 | (45, 88, 196, 290) |

`@3x` (720×960) is a pixel-exact 3× nearest-neighbour of `@1x` for every still. CONTROLbot `@1x` sha256 matches the locked grape still.

## Theme carry

TERMINALbot (silver face, no `>_`, VU + waveform) was also composited onto three other locked v6 stills. The silver identity face is not recolored; board, coil, pins, ticker, and instrument chrome follow the theme. The VU’s danger zone stays red. 0 px outside face + panel.

| theme | slot | changed px | bbox |
|-------|------|------------|------|
| CIRCUIT | 0 | 20340 | (45, 88, 196, 290) |
| ICE | 14 | 20350 | (45, 88, 196, 290) |
| GOLD RUSH | 17 | 20348 | (45, 88, 196, 290) |

## Per-bot center-tap table (locked, Mobandy 5:03 PM ET)

Center target is `LV_CB_FACE` inclusive (53, 78)–(187, 212). Mock only; nothing is wired in firmware.

| Big-bot screen | Center tap opens |
|----------------|------------------|
| BOTpheus | Morpheus Dojo / BOTpheus EB3 (`LV_SC_MO`) |
| GIBSONbot | Gibson scan tower / radar (`LV_SC_RADAR`) |
| CONTROLbot | cycles the theme (existing v6 face-tap) |
| HACKbot | HACKbot game (`LV_SC_DL`) |
| TERMINALbot | TERM (`LV_SC_TERM`) |

## sha256

Default theme (NEON GRAPE):

```
5636ed2b78ccf3c79399f1032a1a839ce2bb1777ea9782d50c5115c87e7ab49b  stills/botpheus@1x.png
6e6b59f9e5f875d5bd432b72bc00dc79a7cb1a98708c3e61421922ddb61ca285  stills/botpheus@3x.png
48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779  stills/control@1x.png
c2ca9d57d814783c4f003dced9e9c36d33262a92487dad6d8887d2921c94fc1a  stills/control@3x.png
79ebfca3196a516eefa9747c2d03d4e278e96acfebaf26871d13e4af68251010  stills/gibson@1x.png
d2ae1c306984a70c09ea876015440856f0d1ecb7d82fa8b05b99a55a0ea0dd41  stills/gibson@3x.png
3ce8e74bb32cf1780387376639b5df7145c266e75d03c45e94170aedb762730f  stills/hackbot@1x.png
e204a7d837fa7bad99b433b90d27252f8c1c29e156b374422659d746b541c642  stills/hackbot@3x.png
fe80471928829a6468c57ff7408aeb3d73cccbccb94fb32265a773d6b8fe9c57  stills/terminal@1x.png
429491189ed1339e45b93ca77afc3fcfbd0bce45bb16b877c01a38eded9113fa  stills/terminal@3x.png
56ca032c6290ea9478b199ceb3b9701543e54d00a858aaef032b12f1983ad5e1  contact_sheet_5bots@3x.png
257f483a8910e410b195124c9dce57e8bc98cfeb59eda15cc09409139b4ff51f  terminal_3themes@3x.png
```

Themes:

```
356eadbe2702611fe19cafcbec7226e9c6e90a899e993b3c680096a21b865d1d  themes/terminal_01_circuit@1x.png
280ba43630a2e0e28874deda65bb9f442edf9d5293be64c12f0efdb495a18784  themes/terminal_01_circuit@3x.png
fb39cfad6f9de2af8067d653479f6d236e8bf0e518ed08a4d70db76567c56d6f  themes/terminal_15_ice@1x.png
ad1c629171cdd586097c97ba69234675aa599d1cb5e6c6f9a3c9429f45c90ccb  themes/terminal_15_ice@3x.png
15fc5fe65b928f43a2aafffaa45679f15f1744653ba5f2eb57fc851653e26d66  themes/terminal_18_gold_rush@1x.png
bd38a1dc7e8bfdf513ff079e07f0973295a2e72fcd4acd63a1193cfc0586139b  themes/terminal_18_gold_rush@3x.png
```

## Caveats

- Mock only. Panel values are an EX.DATA snapshot (names and formats from firmware: `MO_N`, `MO_DEV`, `itw_mo_sel`, `lv_nf` / `LV_K_WIFI` / `LV_K_BLE`, `f.rssi`, `lv_dl_round` / `got` / `need` / `trace` / `secs10`, `f.alert`). They are not live readings.
- CONTROLbot is a byte-for-byte copy of locked v6 NEON GRAPE, including HAZRD / SCAN / VOL / BRIGHT.
- The 110×110 face is a locked slot, not a uniformly scaled home chip.
- Instruments are anti-aliased at native 240×320 (3× supersample, then box filter). `@3x` stills are exact nearest-neighbour of that 1× frame, which is the CYD scale rule — they are not a separately bilinear-smoothed 720×960 render.
- Swipe overlay, legend, and the short banner mp4/gif were delivered in v1 and were not rebuilt for v3.

## Rebuild

```
zig c++ -target x86_64-linux-gnu.2.36 -O2 -std=gnu++17 -DLV_HOST=1 \
  -I <lv_draw.h dir> -I <cyd_themes_lv_core_copy.h dir> -I <themes_spec_v2.h dir> \
  -o bigbot_v3_compose sim/bigbot_v3_compose.cpp
python3 sim/render_bigbot_v3.py
```
