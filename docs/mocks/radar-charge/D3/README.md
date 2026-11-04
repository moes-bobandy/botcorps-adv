# Radar charge — design D3 (electrical grid current, no glyphs)

D3 is D2 (358fd6e) with every GIBSON skull/face glyph and its halo removed. The moving flow is now pure electricity riding the grid lines.

## Look
- **Wire:** an ice-white 1 px wire with a blue 4-neighbour glow, plus a 2 px soft blue bloom on the energized part. It follows the same grid-line route: along y255, up the floor line, then along the base row y163 to the target. Violet dots run ahead of the charge front.
- **Surge packets:** they replace the glyphs and travel up the wire every 34 route px. Each has a 6 px white-hot core (3 px white, about 9 px with blue glow) and a 12 px fading ice/blue tail, so it reads as an elongated energy packet with a glow tail.
- **Crackle:** stepped, square-cornered arcs that branch off the wire (2 + front/60 of them), all axis-aligned and within 7 px of the route. There is no jagged bolt and nothing runs diagonally across the grid.
- **Charge front:** a hot leading tip while charging.
- **Locked state:** a contact glow on the target tower and a contact port on the panel's y247 frame.
- **Behind the near-tower cluster:** the wire and glow show through the glass at 10/16. Packets and arcs are hidden there. Tower edge lines are never painted over.
- **Unchanged:** A's HOLD TO LOCK bar, the constant flow through lock and hunt, and everything else on the radar.

## Files
- stills/D3_{idle,mid,full,lockfire,locked,hunting}@3x.png (full = 100%, the frame before lock)
- charge_D3_3x.mp4 (also in video/), sheet_D3.png, video/frames_D3_sheet.png (every 29th frame)
- diff/diff.json (stills), diff/video_diff.json
- src/: apply_charge_D3.py (applied after apply_charge.py on 3240b03 lv_radar.h), run_D3.sh, diff_D3.py, scripts/, video/

## Verification
| check | result |
|---|---|
| px outside mask, stills | 0 (idle 769, mid 1273, full 1513, lockfire 1357, locked 1270, hunting 1244 changed) |
| px outside mask, video (383 sim frames, LOG build == encode build) | 0 |
| final RADAR line | lock=1 hunt=1 |
| flag 0 vs pristine 3240b03, radar_lock.txt | 9/9 dumps byte-identical, stdout identical |
| D3_idle == A_idle | identical |
| mp4 | h264 yuv420p 720x960 30/1, 344 frames, 11.466667 s, moov before mdat |

## Shortcomings
- Behind the near-tower cluster only the dimmed wire shows. Packets and arcs disappear there.
- For about 13 px the base-row route lies exactly on f19's top edge line. Edges are never repainted, so the core is the tower's own line there, and only the glow above and below bridges it.
- The arcs are small because of the 7 px mask. The port is a glow pad, not a drawn connector.
- During the hunt pan the route follows the slanted floor lines, so the wire is diagonal on screen even though it stays on grid lines.
- The current is thin at 1x. It reads best at 3x.
