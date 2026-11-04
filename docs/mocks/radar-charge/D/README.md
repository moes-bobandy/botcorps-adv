# Radar charge — design D (mock only: no firmware port, no flash, no merge)

D uses A's HOLD TO LOCK 10-cell bar (x166-234, y250-260, unchanged) and C's route: from the grid row, up one floor line, then along the base row of the tower. The current is **GIBSON-only rain**: white GIBSON bot-face glyphs from the LOCKED ui_main_bot_rain (lead #FFFFFF, trail 92/90/88/86 %, black eyes), with no other device glyphs.

The electrical look has five parts:
- A flickering ice-white wire (#d2f4ff) inside an electric-blue glow (#3aa8ff), with white surge packets racing up it.
- A larger white-hot halo on the lead glyph, with an antenna spark.
- Brightness jitter on the trail glyphs.
- Small crackle arcs that run parallel to the path, 5-6 px to either side, and re-roll every frame.
- A white-hot charge tip with sparks while charging, and a contact glow at the tower once locked.

There is no jagged bolt and no diagonal across the grid. The stream grows with charge %. After the lock it never fades: it keeps flowing through LOCK, IGNORE/WATCH/HUNT and HUNT. Releasing early drains it the same way as A/B/C.

## Files
- `stills/D_{idle,mid,full,locked,hunting}@3x.png` (720x960) and `sheet_D.png`
- `charge_D_3x.mp4` is the same timeline as A/B/C (0-8.47 s), then a HUNT tap and 3.0 s of hunting. It is H.264 yuv420p, 720x960, 30 fps, 344 frames, 11.47 s, with +faststart.
- `video/frames_D_sheet.png` shows video frames at 5.0/6.5/7.3/8.6/10.0/11.4 s.
- `diff/`: diff_D_*@3x.png (yellow = changed inside the mask, red = outside), diff.json, video_diff.json
- `src/`: apply_charge_D.py (run after ../src/apply_charge.py, flag LV_RD_CHARGE=4), run_D.sh, diff_D.py, scripts/charge_D.txt, video/video_D.txt, encode_D.sh, diff_video_D.py

## Verification
- Outside-mask diff vs the identical run built with LV_RD_CG_NODRAW=1. The mask is the button rect +2, the locked tower box +2, and the logged route dilated 8 px.

  | Still | Changed px | Outside-mask px |
  |---|---|---|
  | idle | 769 | 0 |
  | mid | 1360 | 0 |
  | full | 1753 | 0 |
  | locked | 1605 | 0 |
  | hunting | 1551 | 0 |

  Video: 0 outside-mask px across all 383 sim frames.
- D_idle@3x is pixel-identical to A_idle@3x.
- Flag 0: the locked radar_lock.txt run built from the patched header is byte-identical to the pristine 3240b03 build on all 9 dumps.

## Known shortcomings
- The base-row segment passes in front of another teal tower.
- On top of the blue halo, the 86-92 % trail glyphs read slightly grey.
- The crackle arcs are small ticks, not big arcs.
- After the lock the panel grows to y247. The stream's bottom row is clipped, so the stream appears to come out of the panel top.
- D_full is the lock-fire frame: the bar is already gone and IGNORE/WATCH/HUNT are showing.
