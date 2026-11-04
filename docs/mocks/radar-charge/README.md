# radar-charge — hold-to-charge lock, three UI designs (MOCK, stills + videos)

Owner feedback (2026-11-04, 6:58 and 6:59 AM ET): a CHARGE button you hold. A meter builds up and then the lock fires.
The current is not a jagged bolt: it is a circuit-style current running parallel to the grid lines.
One variant uses a stream of bot rain with electric properties.

The firmware is untouched. Every change lives in a scratch copy of the locked radar `src/live/lv_radar.h` @3240b03,
behind `LV_RD_CHARGE` (0 = off, 1 = A, 2 = B, 3 = C). See `src/apply_charge.py`, `src/lv_radar.charge.patch` and `src/run.sh`.
With the flag at 0, the locked `radar_lock.txt` run is byte-identical to the locked render (all 9 dumps, the full clip included).

Flow: select a tower (tap it, or tap empty space to cycle; unchanged), then HOLD the CHARGE button.
- The charge takes 1 s; the meter fills and the current builds from the button along the grid to the tower.
- At 100% the lock fires: the tower lights in its own colour (never white, no snap flash), and the LOCK brackets/label and IGNORE / WATCH / HUNT appear exactly as today.
- After the lock the current holds for 250 ms and fades out by 600 ms, giving way to the grown panel.
- If you let go early, the charge drains over 300 ms. The release tap never cycles, and tower hold-to-lock is off.

Route (all variants): from the button, along a grid row, then up one real floor line
(x = 120 + (g*0.5 - camx)(y - 138)/0.9, the same formula the grid uses, so the current sits on a drawn floor line),
then along a horizontal row at the tower's base to the tower.

| variant | charge button (where) | current |
|---|---|---|
| A | bar "HOLD TO LOCK" with 10 battery cells + nub, x166..234 y250..260 (the HUNT slot just above the info panel; the panel is not moved) | one PCB trace: dark dotted copper ahead, energized magenta behind, white/pink pulse packets travelling to the tower, 5x5 solder-pad vias at both turns (cyan at the tower) |
| B | round ring button (r 10) at x6..26 y241..261 (the IGNORE slot, left, above the panel); the ring fills clockwise, % in the centre | a bus of 3 parallel traces 5 px apart that fill in 6 px segments with lit joints, staggered corners, a via per lane |
| C (bot rain, replaces the original "node-stepping trace" C) | a tab on the info panel's top frame, x98..141 y251..262, labelled "CHARGE"; the frame line itself is the gauge, filling outward from the tab | a stream of the firmware's 8x8 device-rain glyphs (DEV_ART / DEV_NEON) flowing along the floor line in sparse runs (3–5 lit per 8-cell cycle), one cell per 60 ms. Electric: hot leading glyph with a magenta/violet glow box, trail 100/80/65/60/55 %, random crackle dropouts, white/pink/cyan sparks ahead of each glyph, brighter as the charge builds |

Stills (720x960, 3x nearest), in `stills/`:
- `X_idle` (selected, button idle)
- `X_mid` (t=22950, ~51%)
- `X_full` (t=23550, the lock fires: current at full, tower lit, buttons up)
- `X_locked` (t=24210, settled)

`sheet_ABC_mid.png` shows A|B|C at mid charge.

Diff (`diff/`, `src/diff_charge.py`): each still is compared with the same run built with `LV_RD_CG_NODRAW=1`, which has the same state and logic but none of the new drawing.
- Allowed mask: the button rect +2, the logged current path dilated by 3 px (C: 7 px, the glyph size), the tower box + label +2, and for C the panel top frame row.
- Changed pixels outside the mask = 0 in all 9 stills (diff.json).
- Yellow = changed, red = outside the mask.

## Videos (added 2026-11-04, 7:20 AM ET request)

`video/charge_{A,B,C}_3x.mp4`: 720x960 (3x nearest), 30 fps, H.264 yuv420p, +faststart, 254 frames / 8.47 s each.
Rendered from the same LV_RD_CHARGE scratch sim (`src/video/video_{A,B,C}.txt`, `src/video/encode.sh`). The sim runs on 30 ms ticks,
so frame j of the video shows the sim frame at j/30 s (no blending).

Timeline (same in all three):
- 0.0–1.35 s: base radar, nothing selected.
- 1.35 / 2.4 / 3.3 s: three tap-to-cycle taps on empty space. The third is part of the same prefix the stills use, which keeps TELLO in the spot where the stills tap it.
- 4.3 s: tap TELLO-5A21B3. The panel shows it and the CHARGE control appears.
- 4.9–5.3 s: early hold to about 40% (39%), then let go. The charge drains in roughly 0.12 s: the drain is 300 ms scaled by the level, so it is brief. The release tap does not cycle.
- 5.85 s: hold again from 0. Mid-charge is around 6.4 s.
- About 6.85–6.9 s: 100%. The lock fires, TELLO lights in its own cyan (no white flash), the LOCK brackets appear, and so do IGNORE / WATCH / HUNT. The current holds 250 ms and then fades.
- About 6.9–8.47 s: locked and settled (about 1.5 s).

A colour change (video request): A's unlit "copper ahead" dots were MAG at 3/8 and almost vanished on the violet floor line.
They are now the pale pink HOT at 4/8 (A only, every other pixel, still dim). Only `stills/A_mid@3x.png` and the A panel of
`sheet_ABC_mid.png` changed, because the other A stills have no unlit run. `diff/diff.json` is unchanged: 0 outside the mask, all 9.
B and C are byte-identical.
