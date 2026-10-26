# hub6 REPORT: shared main-bot chip, landscape cleanup, CONTROLbot power strip

Work completed Mon Oct 26, 2026, about 4:55 PM ET. Branch `cyd-portrait-v3-fixes`, parent 5157377 (hub5).

## Shared main-bot chip (Mobandy 4:30–4:32 PM ET, confirmed 4:49 PM ET)
- `drawMainBotChip(g, x, y, orient, botId, info)` lives in `botcorps_cyd_ctlbot.inc`. It is factored from the CONTROLbot center module: frame, pins, stretched power-bar header, face window, and name/info label.
- Size: P 84×120 and L 74×96 on every board.
- Only the face and the name/info text change per bot:
  - GIBSON: ivory face with dark glasses
  - TERM: silver
  - SCAN: metallic gold
  - CONTROL: theme color, tap cycles the theme
- Measured main-chip rects (x, y, w, h):

  | Board | L | P |
  |---|---|---|
  | home A/B/C | 123,34,74,96 | 78,78,84,120 |
  | TERMINALbot | 123,30,74,96 | 78,78,84,120 |
  | SCANbot | 123,30,74,96 | 78,30,84,120 |
  | CONTROLbot | 123,38,74,96 | 78,86,84,120 |

- Identity proof (`mainchip-identity.txt`): each chip was cropped and compared with CONTROLbot's pixel by pixel, outside the face window and label rect. Every board differs by **0 px** (L: 0/2896, P: 0/4346).
  - Face window: L (10,19,54,54), P (10,19,64,64).
  - Label rect: L (3,73,68,19), P (3,85,78,21).
- Side-by-side images: `mainchip_{L,P}_{native,3x}.png`.

## Automated overlap check (`ovlcheck.py` over the sim-recorded `ovl/*.ovl.txt`)
- Rules:
  - chip∩chip;
  - chip or text vs plate rect inflated 4px (pin-header labels exempt);
  - text∩text;
  - text partly over a chip it doesn't belong to;
  - text in the top bar.
- Result: **0 hits on all 12 screens** (L and P × homeA, homeB, homeC, term, scan, ctrl). See `overlap-report.txt`.

## Landscape fixes
- Home A/B/C L and TERMINALbot L: the main chip is shared. All chips and labels end at y ≤ 177; L castellations are at 182. Corner silk (3V3/GND/NET-H) is printed only where no chip sits within its +4px keep-out.
- SCANbot L:
  - SCANbot is centered with WIFI/BLUETOOTH mirrored above.
  - Tabs are a 2×2 block of 39×28 on the left. They mirror the bay on the right, which holds 6 readable 28px tiles (2 cols × 3 rows). MORE sits under the module. GIBSON is under the tabs.
  - The garbled y≈26 row was background-motion pixels leaking around the field fill. The fill now covers y26 and the header gaps.
- CONTROLbot L:
  - Mirrored columns: SHAPE|MIX on the left, BG|MULTI on the right. RAIN MODE mirrors the CP chip (RAIN COL, now at x230, off the edge).
  - ORIENT and BG MOTION are mirrored 72×16 wide toggles, with the name above and the value inside. The stray red square is gone.
  - Two-line labels are spaced 9px apart in L.
- CONTROLbot power strip (Mobandy 4:27 PM ET): a full-width meander strip just under the top bar, from x 36–284 in L and 30–210 in P. The CONTROLBOT header is centered on it. It replaces the old top silk in L and the bottom silk in P (the P silk was inside the plate keep-out).
- CONTROLbot P layout:
  - Meters moved to y45. The center module moved to y86 so the meters no longer overlap the chip.
  - ORIENT and BG MOTION moved up 4px so their labels clear the plate.
  - MIX widened to 36 so its MASHUP label fits.
- Touch: sim taps confirmed. The CONTROLbot main chip cycles the theme in L and P, and the L ORIENT toggle selects. Hit rects come from the same tables.

## Build, gate, IRAM
- `pio run` succeeded for cyd, cardputer-adv, cardputer-adv-dual, -dual-launcher and -dual-livescan.
- Adv `.iram0.text` = **94423 (Δ0)**.
- `tools/gate.sh`: **1042/1057 PASS, 15 FAIL, 4 N/A**, the same as baseline. gate.py was not edited.
- RAM: all 4 caps pass.
- Every edit is CYD-only. The sim overlap recorder in `botcorps_ui.cpp` is under `BOTCORPS_CYD && BOTCORPS_SIM` and compiles to nothing on device.

## Bins (flat): `/workspace/out/cyd-full-board-build/checkpoints/bins-hub6/`
- cyd-firmware.bin: sha256 a8ec9f57d992e62aca4744ca7e060eda7da663456895d2faafd201d25ef14e86 (1488704 B)
- cyd-firmware-merged-0x0.bin: sha256 6bfd6ff3a99618964b857e567eb4dc699805b4eb336f1564d54af8f46acd84b8 (1554240 B)
- MERGED-PROOF: the slice @0x10000 of app length has the same sha256 as the app, so **PROOF_slice_eq_app=True**.

## Renders
- Every L screen at native size and @3x, plus `landscape-L-contact@3x.png`.
- Every P screen, plus `portrait-ABC-contact@2x.png` and `portrait-all-contact@2x.png`.
- `control-L@3x.png` and `control-P@3x.png`.
- Main-chip proofs.

## Gaps / interpretations
- The SCANbot L bay is 2 cols × 3 rows (6 tiles) beside the module, not 3×2 under it. A 74×96 shared chip leaves no room for a 3×2 bay below it in 240px.
- The "stretched power bars" are a new board-level strip under the top bar. The chip's own header bars are also stretched across the chip width, so the shared chip stays identical on all boards.
- The main chip's y position differs between boards; its size and frame are identical. P CONTROLbot moved from y78 to y86 to fit the strip and meters.
- No push, merge, flash or Downloads.
