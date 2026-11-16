# Big-bot Color UI v6 mocks — v2 unique bottom panels (MOCK ONLY)

Mobandy 2026-11-16 5:03–5:09 PM ET (America/New_York): keep every square bot face locked at the CONTROLbot v6 size and position, drop the `>_` prompt from the TERMINALbot face, and redesign the existing bottom panel for each bot’s role. CONTROLbot keeps the locked v6 control rows exactly.

**Mock only. No firmware commits to the build branch, nothing flashed, nothing copied to an SD card.**

v1 (faces + names only, cloned CONTROLbot rows) remains at `../` and on this same branch as commit `f55e13b`. This folder is v2 only.

## Locked geometry (do not shrink or move the face)

- Face slot `THEME_FACE` inclusive **(65, 88)–(174, 197), 110×110** — identical to locked CONTROLbot Color UI v6. The face box is not resized to make room for panels.
- Face + black frame `LV_CB_BOT` inclusive (54, 79)–(185, 210). Names sit in the black pad at y = 200, LV_GLCD 5×7, ink (231, 239, 247).
- Bottom panel `LV_CB_P*` origin (45, 220), size 152×71, inclusive **(45, 220)–(196, 290)**. This is the only area redesigned per bot.
- Grey bezel rows y = 211–219 stay pixel-identical to locked v6 (0 px changed).

## Sources (verified)

- Firmware home spots: `src/live/lv_core.h` on `moes-bobandy/botcorps-adv`, worktree `/workspace/cyd-fw-3locks-wt` (`backup/cyd-fw-3locks-2026-11-16`). Base `049ff59`, build `c1b5659`, v1.1 rewires `ada2253`.
- Locked v6: `LOCK.md` at `a87fe22` on `backup/controlbot-colorui-2026-11-15`, `mocks/controlbot-colorui-1115/v6-cyd-ticker/`.
- Default theme: firmware `lv_theme = 0` → NEON GRAPE (`LV_CB` slot 11). Locked still sha256:
  `48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779`
- Panel chrome uses the same `lv_cb_reink` / theme-role path as locked v6. RGB565 is expanded with `<<3` / `<<2` (the expand the v6 stills actually use). A control-panel selftest against the locked NEON GRAPE still is **0 px**.

Renderer: `sim/bigbot_v2_compose.cpp` (zig c++, `-target x86_64-linux-gnu.2.36 -O2 -std=gnu++17`, real `lv_draw.h` / `lv_font.h`). Faces are firmware C++ painters or NN-fitted locked sources, never Pillow lookalikes. Unchanged pixels are copied byte-for-byte from the locked still (never re-quantized).

## Home spots

Same five `LV_HOME_CHIP` slots as v1: BOTpheus, GIBSONbot, CONTROLbot, HACKbot, TERMINALbot.

## Faces

- **CONTROLbot:** locked v6 theme face, untouched. No name plate (the lock has none).
- **HACKbot:** firmware `e_gold_face` + eyes at `s = 110`. Metallic gold with diagonal sheen, never flat yellow. Blink off.
- **TERMINALbot:** firmware `e_silver_shell` + `e_eyes` only, at `s = 110`. Clean silver, two eyes, **no `>_` prompt**, no extra face border. Name spelled `TERMINALbot`. (v1 used `e_term_marks`; that is gone.)
- **BOTpheus:** locked shades face from `variant_big_morpheus.morpheus_chip` (`glossary_big`), NN-fitted into the 110×110 slot.
- **GIBSONbot:** cream face + shades, cropped from official `th0_home_3x.png` at (96, 56)–(144, 104), NN-fitted into the same 110×110 slot.

Theme variants do not recolor the gold or silver identity faces.

## Bottom panels (unique per role)

All four new panels share the locked chip/block chrome and the current theme’s `CB_KEY` / `CB_VALUE` / `CB_PLATE` inks, and they all live in the same 152×71 plate. Layouts and readout types are not reused across bots. Snapshot numbers are **EX.DATA** (stamped in 3×5 in the plate corner; never `#`).

| Bot | Panel contents (EX.DATA snapshot) | Why this panel fits the role |
|-----|-----------------------------------|------------------------------|
| **BOTpheus** | Header `DOJO`. 2×9 map of all 18 `MO_N` pillars with firmware short tags `DE HK FL DR AT KF ME SK FP / SA AX RV AL CM RI GT ET IB`. Cell `AT` (index 4, `MO_DEV` “AIR TAG”, `itw_mo_sel` / `PILLAR 5/18`) is lit. Readouts `PILLARS 18`, `SEEN 18`, `LAST AIR TAG`. | BOTpheus’s job is the Morpheus Dojo: this is a pillar roster and last-lock, not a volume/bright deck. |
| **GIBSONbot** | Dual radio cards `WIFI 12 AP` and `BLE 8 DEV` (`LV_K_WIFI` / `LV_K_BLE` / `lv_nf` counts). Full-width `RSSI` ruler −90..0 with a caret at **−42 dBm**, plus `CH 6` (`f.rssi`, `f.ch`, firmware `%ddBm` / `CH%d`). | GIBSONbot is the scan / radar bot, so the plate is a radio survey (AP + BLE + strongest signal + channel), not a reused meter row. |
| **HACKbot** | Game HUD: `TRACE 42%` heat bar (`lv_dl_trace` 0–1000 as %), `LEVEL L2` (`lv_dl_round`) lit, `FILES 1/2` (`lv_dl_got` / `lv_dl_need`), `TIME 37.8S` (`lv_dl_secs10`), `BEST 860` (`lv_dl_best` — this is **score**, not time). | HACKbot is the download / trace game; the plate is that game’s HUD, not a scan ruler or a Dojo map. |
| **TERMINALbot** | Header `THREAT LOG`. 10 hunt cells `DE ME HK SK FL FP DR AT KF SA` (`LV_HUNT_NAME` / `LV_HUNT_BOT[10]`); `FL` and `AT` lit. `ALERTS 2` (`f.alert`), `LAST AIRTAG` (`lv_hunt_last`), `SEEN 20` (`lv_matches()` / “MATCH %d/%d %d SEEN”). | TERMINALbot watches the threat log; the plate is a hunter roster plus alert/last/seen, not a game HUD or radio ruler. |
| **CONTROLbot** | Locked v6 rows exactly: `SCAN B.RAIN`, `VOL 40%`, `BRIGHT 80%`, `CHIME ON`, `TIME RTC`, `BAR HAZRD`, `TONE BEEP`. | This screen already *is* the control deck; the lock is left 0-diff, including the panel. |

## Pixel diff vs locked v6 stills

Default-theme stills are diffed against `12_neon_grape@1x.png`. Allowed change regions: `LV_CB_BOT` (face + name) **and** the bottom panel (45, 220)–(196, 290). 0 px outside those two rects. 0 px in y = 211–219.

| bot | changed px | diff bbox (x0,y0,x1,y1) |
|-----|------------|-------------------------|
| BOTpheus | 18190 | (46, 88, 195, 289) |
| GIBSONbot | 17135 | (46, 88, 195, 289) |
| CONTROLbot | 0 | none (identical, including panel) |
| HACKbot | 17801 | (46, 88, 195, 289) |
| TERMINALbot | 17327 | (46, 88, 195, 289) |

The 1 px of panel chrome that already matched the locked plate (x = 45, x = 196, y = 290) is unchanged, so the bbox is inset one pixel from the formal panel rect. `@3x` (720×960) is a pixel-exact 3× nearest-neighbour of `@1x` for every still. CONTROLbot `@1x` sha256 matches the locked grape still.

## Theme carry

TERMINALbot (new silver face, no `>_`, unique threat-log panel) was also composited onto three other locked v6 stills. The silver identity face is not recolored; board, coil, pins, ticker, and panel chrome follow the theme. 0 px outside face + panel.

| theme | slot | changed px | bbox |
|-------|------|------------|------|
| CIRCUIT | 0 | 17363 | (46, 88, 195, 289) |
| ICE | 14 | 17327 | (46, 88, 195, 289) |
| GOLD RUSH | 17 | 17327 | (46, 88, 195, 289) |

## Per-bot center-tap table (locked, Mobandy 5:03 PM ET)

Center target is `LV_CB_FACE` inclusive (53, 78)–(187, 212). Mock only; nothing is wired in firmware.

| Big-bot screen | Center tap opens |
|----------------|------------------|
| BOTpheus | Morpheus Dojo / BOTpheus EB3 (`LV_SC_MO`) |
| GIBSONbot | Gibson scan tower / radar (`LV_SC_RADAR`) |
| CONTROLbot | cycles the theme (existing v6 face-tap) |
| HACKbot | HACKbot game (`LV_SC_DL`) |
| TERMINALbot | TERM (`LV_SC_TERM`) |

Swipe left/right on the top ticker still flips to the next/previous bot’s big screen (v1 overlay + short mp4/gif, unchanged).

## sha256

Default theme (NEON GRAPE):

```
6bbf873d7136ae101a8bdb4168906bdfb84400abea89b4b2d4db49cfdcacc6ec  stills/botpheus@1x.png
6ac13f0c74ac4644787abf45beb718e4e51ef5f5670522675603a3e4a23e7c94  stills/botpheus@3x.png
48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779  stills/control@1x.png
c2ca9d57d814783c4f003dced9e9c36d33262a92487dad6d8887d2921c94fc1a  stills/control@3x.png
da01f895ea102d9076805a6d72aaeed63a33ceaca0aeb35a5e179aca9df4dd26  stills/gibson@1x.png
c22b1f602a9b4898e91b5e18032eb45e62818b78d31c8613416300522a1faa45  stills/gibson@3x.png
4f8cafe3f56776157d243ffeca4dbb7fd2daf97f3b48b394499d6aabd650f8f2  stills/hackbot@1x.png
90f42d71f024a915c2138aafa8df183038afdf4d67de1888de7957f9c81fbd88  stills/hackbot@3x.png
2a8096e4f7895b0a9ed600419415b5127ce61f504d705e18c3ab09bc4a78a5e6  stills/terminal@1x.png
b6375a319200a184c50efc2b16f4840ef149324050d4f642d628aa2ba38e8a98  stills/terminal@3x.png
83adf0327b6ea7b19d4899d7f2b3b174a3b524fba8adeb7a297879b0a82628ae  contact_sheet_5bots@3x.png
8dc97ca5fb0847355c37b88fd89433655c6ada4cc4d93998aee72964aecfb8fc  terminal_3themes@3x.png
```

Themes:

```
b1a6bcd44c33724854d5b1918d38eccfaacfd694aacc40532825b3e8ffe2ae42  themes/terminal_01_circuit@1x.png
c90c68b6ee1a3caca58a29e3879885626c2b4d1477e5815f1d8c360148b0134c  themes/terminal_01_circuit@3x.png
da895a81b5167d382759e918219437fa2fb88ed91d80b1dba931e2c2e593db6d  themes/terminal_15_ice@1x.png
4ee0d30281ee5934922251ab6ff187bf6d228e665c0fd40d582c95b90b9e0e49  themes/terminal_15_ice@3x.png
daf2f683ed14756dd470d2c621a5a4fe720c310c9205e387dcee83e75d599b29  themes/terminal_18_gold_rush@1x.png
b94f89ab693f3e74e6556e77d6f63aa6246ae16be05b751eae55b97fad8ab923  themes/terminal_18_gold_rush@3x.png
```

## Caveats

- Mock only. Panel values are an EX.DATA snapshot (names and formats from firmware: `MO_N`, `MO_DEV`, `itw_mo_sel`, `lv_nf` / `LV_K_WIFI` / `LV_K_BLE`, `f.rssi` / `f.ch`, `lv_dl_round` / `got` / `need` / `trace` / `secs10` / `best`, `LV_HUNT_NAME`, `f.alert`, `lv_matches()`). They are not live readings.
- CONTROLbot is a byte-for-byte copy of locked v6 NEON GRAPE, including HAZRD / SCAN / VOL / BRIGHT.
- The 110×110 face is a locked slot, not a uniformly scaled home chip. Gold and silver are firmware painters at `s = 110`. BOTpheus and GIBSONbot are NN-fitted from locked sources.
- Swipe overlay, legend, and the short banner mp4/gif were delivered in v1 and were not rebuilt for v2.

## Rebuild

```
zig c++ -target x86_64-linux-gnu.2.36 -O2 -std=gnu++17 -DLV_HOST=1 \
  -I <lv_draw.h dir> -I <cyd_themes_lv_core_copy.h dir> -I <themes_spec_v2.h dir> \
  -o bigbot_v2_compose sim/bigbot_v2_compose.cpp
python3 sim/render_bigbot_v2.py
```
