# hub5 REPORT — landscape symmetry + readability (Mobandy 4:13 PM ET)

America/New_York local work completed Mon Oct 26, 2026 ~4:24 PM ET.

## Request
ALL landscape screens (home A/B/C, TERMINALbot, SCANbot, CONTROLbot):
1. Symmetric dev-board layout (center module horizontally centered; L/R mirrored chips; even-grid traces; equal side headers; aligned silk)
2. Readability on 320×240 — biggest fitting font (≥ portrait hub chip names / Font0 ~6px); no 4–5px silk for important labels; drop unreadable decorative silk; faces stay big
3. Keep prior hub5 rules (GIBSON slightly larger square; BOOTbot = GIBSON inverse same shape not tappable; SCAN metallic gold; TERM silver; CONTROL themed; 3-line plate; pads ≥28; no overlaps)

## Layout (logical 320×240)

### Symmetry check (center module left/right offset; mirrored chip x)

| Screen | Center module | Center x | Offset vs 160 | Mirrored chip x-pairs |
|--------|---------------|----------|---------------|------------------------|
| home A L | GIBSON 132,74 56×56 | 160 | **0 px** | y34: 40↔232; y134: 40↔232 |
| home B L | GIBSON 132,74 56×56 | 160 | **0 px** | y34: 40↔232; y134: 40↔232 |
| home C L | GIBSON 132,76 56×56 | 160 | **0 px** | y80: 40↔236; N/S solos at x=138 (center 160) |
| TERMINALbot L | TERM 132,74 56×56 | 160 | **0 px** | y34: 40↔232; CTRL solo bottom center x=136 |
| SCANbot L | SCANBOT 124,32 72×86 | 160 | **0 px** | y36: WIFI 40↔BLE 252; mid: GIBSON 40 ↔ bay 200 |
| CONTROLbot L | CONTROL 123,34 74×96 | 160 | **0 px** | y40: 40↔244; y100: 40↔244; bottom: 40↔252, 72↔220 |

Measured face centers in PNG: SCAN gold cx=159.5, homeC GIBSON white cx=159.0, CONTROL purple cx=159.5.

GIBSON / TERM / SCAN module size vs corner chips: 56/48 ≈ **1.17×** (A/B/T); C 56/44 ≈ **1.27×**.

### Readability check (smallest **important label** font per screen)

Important labels = chip/module names (BOOT/SCAN/CTRL/TERM/GIBSON/SCANBOT/CONTROL/WIFI/BLE/cat pads). Decorative pin-header silk and dropped NET labels are not counted as important.

| Screen | Important label font | Notes |
|--------|----------------------|-------|
| home A/B/C L | **Font0 / 6 px** | Single-line chip names via `use_px(6)`; portrait still tiny 3×5 two-line |
| TERMINALbot L | **Font0 / 6 px** | Same hub painter |
| SCANbot L | **Font0 / 6 px** | SCANBOT module + WIFI/BLE/GIBSON + cat pads + MORE |
| CONTROLbot L | **Font0 / 6 px** | CONTROL + theme short name; peripheral `txt`/`cb_lab` |

Decorative landscape silk: hub `NET-H` dropped; `3V3`/`GND`/`BUS0`/`RING` promoted to Font0 or omitted when they would stay tiny. Pin-header labels remain board chrome (same as portrait edge).

Faces: L silk budget reduced so die/face stay large (hub `sm` 18, SCAN silk_budget 14, CONTROL dmax mh−18−24).

## Renders
All under `checkpoints/hub5/` (also `/workspace/out/cyd-full-board-build/checkpoints/hub5/`):

- `home-A-L-idle.png` + `@3x.png` (960×720 NEAREST)
- `home-B-L-idle.png` + `@3x.png`
- `home-C-L-idle.png` + `@3x.png`
- `termbot-L-idle.png` + `@3x.png`
- `scan-L-idle.png` + `@3x.png`
- `control-L-idle.png` + `@3x.png`
- `landscape-L-contact@3x.png` (2×3 contact of all L @3x)
- `boot-gibson-shape-proof-L.png` (BOOT|GIBSON crop from home-A L @3x)

Portrait hub5 shots retained from prior pass.

## Build / gate / IRAM
- `pio run -e cyd` SUCCESS; `.iram0.text` = 119271 (CYD, not Adv-capped)
- `pio run -e cardputer-adv` SUCCESS; `.iram0.text` = **94423 Δ0**
- dual / dual-launcher / dual-livescan SUCCESS
- `tools/gate.sh`: **1042/1057 PASS, 15 FAIL, 4 N/A** (baseline unchanged; gate.py not edited)
- RAM: all four caps PASS after dual rebuild
- bins: `bins-hub5/{cyd,cardputer-adv,cardputer-adv-dual,cardputer-adv-dual-launcher,cardputer-adv-dual-livescan}/`
- MERGED-PROOF.txt written
- **No push / merge / flash / Downloads**

## Code touchpoints (BOTCORPS_CYD only)
- `src/ui/botcorps_hubbot_layout.inc` — HA/HB/HC/HT `*_CHIP_L` + routes centered/mirrored; C_L overlap fixed
- `src/ui/botcorps_cyd_hubbot.inc` — L Font0 chip/module silk; bigger L faces; drop unread rich silk
- `src/ui/botcorps_cyd_portrait.inc` — scan-L geometry; Font0 scanchip/catpad/MORE/dB; bigger SCAN face
- `src/ui/botcorps_cyd_ctlbot_layout.inc` — CB_L symmetric columns; MET w=0
- `src/ui/botcorps_cyd_ctlbot.inc` — L Font0 module/lab/orient; meters guard; D clamp
