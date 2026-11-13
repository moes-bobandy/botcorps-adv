# MORPHEUSbot v13 "Dojo Construct" (MOCK ONLY)

This is a simulator mock only. Nothing was merged or flashed. Every frame comes from the `sim_live` real C++ renderer (240x320 RGB565, zig c++). The v12 outputs were moved to `_withdrawn/v12/`. Nothing was deleted.

## What changed (src/live: `lv_morpheus_v13.h` added, `lv_inside_towers.h` include v12 -> v13; `lv_radar.h`, v11 and v12 untouched)
- **Pillar skin, idle and close-up** (`mo11_tower_fx`): each pillar is an opaque lacquer monolith.
  - The lit timber top and the side face are drawn in perspective from the `lv_rd_place` back corners. The side face has receding ribs and a mid post.
  - Edges are crisp 1 px lines: parchment on the left and top, timber on the right, dark at the base. A Matrix-green seam runs up the side with a moving pulse.
  - A recessed icon plate holds the device icon at an integer nearest-neighbour size (8, 16, 32 or 48 px), with no blur.
  - Shoji window cassettes (lit paper, dark and Matrix-green panes with kumiko) cover the front.
  - The front uses flat planar tones, so there is no banding and no dither.
- **Motion is unchanged from v11:** tables, `mo11_drift`, the camera at 0.5 sin(0.45 t), the room, rain and chrome. The MOTW read-back is identical over 600 idle frames.
- **Tap:** there is no red pill, green flood or lock bracket. It goes straight to the 400 ms push-in close-up on the left.
- **Search bar** `SEARCH // <DEVICE>  n/3 SCROLLS` sits across the top of the tower.
- **3 folders** follow the DOWNLOAD structure:
  - A `SCROLL n` tab.
  - An icon and one big file name, for example `AIRTAG.TRK`.
  - A big size (EX.DATA) on the right.
  - A purpose line: WHAT IT IS / HOW SPOTTED / PROTECT.
  - A 70x7 bar with a % / LOADED line.
  - They drop at 300 ms, 110 ms apart, 380 ms each.
- **Skin:** parchment, timber, ink and green. No HACKbot neon.
- `< TOWERS` is 70x39.

## Gate 18/20
- FAIL: touch: every pillar keeps a tap cell in every idle sample (inherited v11 motion)
- FAIL: touch: >= 36 px idle tap cell for every pillar in every sample (inherited v11 motion; unchanged because positions must stay v11)
Both fails come from the v11 idle motion, which this version had to keep (Mobandy: keep the engine motion). When AIR TAG or META drifts directly behind another pillar, its tap cell briefly drops to 0 px. Medians are 29..61 px.

## Files
- Stills: `NN_*@1x.png` and `stills/*@3x.png`.
- `contact_sheet.png`, `compare_hackbot_vs_v13_open.png` and `compare_v12_vs_v13_idle.png`.
- `morpheus_playthrough@3x.mp4` (720x960 h264 yuv420p) and `@1x.mp4`.
- `gate.json`, `touch_targets.json` and `morpheus_v13.patch`. The patch is checked against the v12-state tree and reproduces both files.
- `src/`: the header, generator inputs and gate tooling.

## Known
The idle footer still reads "TAP A PILLAR = TAKE THE RED PILL". That is locked v11 chrome text, left unchanged.
