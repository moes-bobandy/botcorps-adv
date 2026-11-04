# Radar charge — design D4 (thicker, calmer, below the top banner)

D4 builds on D3 (c17bc28) after three review notes: "Make the current thicker and brighter", "Make it less aggressive though", and "It can't over power the top meter". It has the same grid-line route as D3 and no glyphs.

## What changed vs D3
- **Thicker.** The wire is now a 2 px ice core (the route pixel plus the next pixel across) inside a 3-step blue glow band on each side, about 8 px wide in total. D3 had a 1 px core and a 2 px bloom.
- **f19 dip fixed.** Where the route pixel lies on a nearer tower's edge line, the core moves 1 px to the other side, so it stays 2 px wide and the tower's edge line stays on top.
- **Steadier glow.** D3 changed the wire brightness randomly every frame. D4 has none of that, only a slow, smooth 2.4 s breathing of ±2/32.
- **Calmer packets.** One every 40 px, moving at a steady 0.2 px/ms (D3: every 34 px at 0.3 px/ms). Each is a 6 px core, 2 px wide, in a 4 px glow on each side, with a 10 px fading tail. Brightness is fixed: no per-frame boost and no random leading spark.
- **Fewer crackles.** About half of D3's count (1 + front/120, so 2 at full; D3 had 4). They are smaller: out 2–3, along 2–3, a 1 px kink, along 1–2. No forks, no tails, a soft ice colour (never white), and they re-roll every 4 sim frames instead of every frame.
- **Softer tip and contacts.** The charge tip is a steady 2x2 with one small spark, re-rolled every 4 frames. D3's arc ahead of the tip is gone. The contact glow and panel port are soft, breathe with the wire, and have no crackles.
- **Brighter behind the near towers.** The wire shows through the glass at 13/16 (D3: 10/16). Packets and arcs stay hidden there.
- **Stays below the top banner.** D4 uses no pure white, and a brightness ceiling (LV_RD_CAP = 176 luma) stops stacked blends from going brighter. The top banner's yellow text peaks at about 203.

## Measured: current vs top banner (1x sim frames)
"Current" counts pixels that the current brightens by at least 8 luma, excluding A's button and the locked tower box.

| still | banner lit mean / peak | D3 current px, mean, peak | D4 current px, mean, peak |
|---|---|---|---|
| full | 190 / 203 | 644, 86, 255 | 990, 83, 179 |
| locked | 192 / 203 | 554, 96, 255 | 746, 78, 178 |
| hunting | 194 / 203 | 519, 95, 255 | 758, 79, 178 |

Frame-to-frame churn caused by the current in the video, averaged per frame:

| video frames | D3 | D4 |
|---|---|---|
| 200–280 | 537 px | 326 px |
| 300–380 | 468 px | 372 px |

## Files
- stills/D4_{idle,mid,full,lockfire,locked,hunting}@3x.png
- stills/D4_locked@1x.png and D4_hunting@1x.png: the full screen at true 240x320 size
- stills/D4_full_crop@1x.png: a crop of the current at true size
- charge_D4_3x.mp4 (also in video/)
- sheet_D4.png, compare_D3_vs_D4.png, video/frames_D4_sheet.png
- diff/diff.json, diff/video_diff.json
- src/

## Verification
| check | result |
|---|---|
| mask margin | unchanged at 8 px route dilation (no widening needed): wire band and packets within 4 px of the route, port glow within 5 px, arcs within 7 px |
| px outside mask, stills | 0 (idle 769, mid 1363, full 1835, lockfire 1563, locked 1440, hunting 1455 changed) |
| px outside mask, video | 0 over 383 sim frames (LOG build output identical to encode build) |
| final radar state | lock=1, hunt=1 |
| flag 0 vs pristine 3240b03 (radar_lock.txt) | 9/9 dumps byte-identical, stdout identical |
| D4_idle vs A_idle | identical |
| mp4 | h264, yuv420p, 720x960, 30/1, 344 frames, 11.466667 s, moov before mdat |

## Shortcomings
- Under the brightness ceiling, packets are only moderately brighter than the wire. They read as gentle bright segments sliding up the line, not punchy flashes. That trade is deliberate.
- The mean luminance of the current's lit pixels is a bit lower than D3's because the wider band adds many dim glow pixels. Its total footprint is 40–55% larger. In short: "brighter" here means more lit area and a stronger see-through, not a higher peak.
- Behind the near-tower cluster only the wire shows. Packets and arcs hide there.
- During the hunt pan the floor lines slant, so the wire looks diagonal on screen, although it stays on grid lines.
- The ceiling is applied per blend in the mock. A firmware port would need the same luma check.
