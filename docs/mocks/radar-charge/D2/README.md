# Radar charge — design D2 (mock only: no firmware port, no flash, no merge)

D2 is design D (c214e09) with the four weak spots fixed, after the owner's review on 2026-11-04 at 12:06 PM ET ("Fix the weak spots and show me again"). Everything else is unchanged from D:
- GIBSON-only white rain.
- A's HOLD TO LOCK bar (x166-234, y250-260).
- C's grid route: grid row, then up one floor line, then the tower base row.
- The stream flows constantly through LOCK, IGNORE/WATCH/HUNT and HUNT.
- No jagged bolt and no diagonal across the grid.

No tower, panel or locked element is moved or restyled. All new code is under `LV_RD_CHARGE == 4`.

## The four fixes
1. **The stream no longer draws over other towers. It passes behind them (occlusion, not a reroute).** Each frame builds the convex hull of every other tower's 8 corners. A tower whose front base is nearer than the stream element's ground row (`yb > gy`) hides it:
   - The tower's edge lines are never painted. Any pixel within 0.9 px of a front, top or back edge segment is skipped.
   - Behind the translucent face, only the wire and its glow show through, at 8/16 strength, the way the grid shows through the glass towers.
   - Glyph faces and crackle arcs are hidden entirely behind a nearer tower. Only a glyph's halo glows through.

   I tried a hard cut-off first, but the base row runs behind four near towers (f19, f14, f11, f1), so the target looked unfed. A reroute along grid lines was not possible: every floor line that reaches the target's base comes from off-screen left, and every base-row approach crosses the same cluster.
2. **Trail glyphs stay white.** The lead face is #FFFFFF. Trail faces are ice-white #F0FBFF, with no greying and no brightness jitter on the face. The run fades through a halo that shrinks and dims by rank: radius² 46/32/26/20/16, gain 20/13/10/8/6.
3. **Real crackle arcs.** `lv_rd_arc` draws stepped, axis-aligned, multi-segment arcs:
   - Shape: out 2-5 px, along 2-4, a 1-2 px kink, along 1-3, an optional tail, and a fork on 1 in 3 arcs.
   - Colour: a white core, then ice, with an electric-blue 4-neighbour glow.
   - Count: 4 + (lit glyphs) arcs per frame, re-rolled every frame.
   - They are drawn after the glyphs but never over a glyph face, so they jump out from behind the skulls. Every pixel stays within 7 px of the route, inside the mask.
4. **A panel contact port.** When locked, the stream meets the grown panel's top frame (y247). There it gets:
   - A white-hot pad on the frame, with ice and blue glow ±4 px along it and 1 px above and below.
   - A pulsing blue glow above it that flares each time a surge leaves.
   - 1-2 crackles spitting off it.

   Nothing else on the panel changes.

Also changed: **D2_full** is the 100 % frame just before the lock fires (t=23460: bar full, lock=0, stream reaching the tower). The lock-fire frame is kept separately as **D2_lockfire** (t=23490).

## Files
- `stills/D2_{idle,mid,full,lockfire,locked,hunting}@3x.png` (720x960) and `sheet_D2.png`
- `charge_D2_3x.mp4` (also at `video/`): the same timeline as D (A/B/C 0-8.47 s, then a HUNT tap and 3.0 s of hunting). H.264 yuv420p, 720x960, 30 fps, 344 frames, 11.466667 s, +faststart.
- `video/frames_D2_sheet.png`: every 29th video frame.
- `diff/`: diff_D2_*@3x.png, diff.json, video_diff.json
- `src/`: apply_charge_D2.py (run after ../src/apply_charge.py, LV_RD_CHARGE=4), run_D2.sh, diff_D2.py, scripts/charge_D2.txt, video/{video_D2.txt, encode_D2.sh, diff_video_D2.py}

## Verification
- Outside-mask diff against the identical run built with LV_RD_CG_NODRAW=1. The mask is the button rect +2, the locked tower box +2, and the logged route dilated 8 px.

  | Still | t (ms) | Changed px | Outside-mask px |
  |---|---|---|---|
  | idle | 22440 | 769 | 0 |
  | mid | 22950 | 1356 | 0 |
  | full | 23460 | 1640 | 0 |
  | lockfire | 23490 | 1524 | 0 |
  | locked | 24210 | 1424 | 0 |
  | hunting | 26610 | 1378 | 0 |

  Video: 0 outside-mask px in all 383 sim frames (386774 changed px in total).
- D2_idle@3x is pixel-identical to A_idle@3x.
- Flag 0: the radar_lock.txt run built from the patched header is byte-identical to the pristine 3240b03 build on 9 of 9 dumps.
- The HUNT tap works: the last radarstate shows lock=1 hunt=1.

## Remaining shortcomings
- Behind the near-tower cluster, the base row shows only as a dim wire with halos glowing through. The skull faces vanish for about 35 px there and reappear only at the target. This is correct for depth, but it is less bold than D.
- The arcs must stay within 7 px of the route (the mask), so at the native 240x320 they are crackles of at most about 5 px, not big arcs.
- The port is a glow pad on the frame line, not a drawn socket shape.
- While hunting, the camera pans and the route follows whichever floor line is current, so on some frames it rides a slanted floor line. It is still a grid line, the same behaviour as D.
