# fw-port-1105: hunt meter A + V2 arc/ripple in firmware (cyd_livescan): sim verification

Ports LOCKED `ui_gibson_radar_hunt_meter` (A thermometer bands, lock 0320479, src c83a27e) and
`ui_gibson_radar_hunt_meter_interact` (V2 arc + band ripple, lock 20faf17, src 057279f) into `src/live/lv_radar.h`
(`LV_RD_HUNT_METER=1` in `env:cyd_livescan`, on top of the D4 charge lock 3b8b3de). Mock only until flashed on Mobandy's go.

Checked with `tools/port/hunt_meter_sims.sh` + `tools/port/verify_hunt_meter.py` (sim_live renders the real src/live headers):

| check | result |
|---|---|
| D4 pre-hunt moments sel / mid / full / lockfire / locked vs 3b8b3de | 0 / 0 / 0 / 0 / 0 px (byte) |
| 200 hunt frames (6 s) vs the locked V2 mock re-rendered by its own code on the 3b8b3de frames, RGB565 @1x | 0 px in all 200 frames |
| locked stills V2_idle / mid / pulse / full @1x vs firmware | 0 / 0 / 0 / 0 px (RGB565) |
| changed px outside the meter masks (tower glass + contact + cleared top-meter slot), 200 frames | 0 |
| `LV_RD_HUNT_METER=0` and `LV_RD_CHARGE=0` vs 3b8b3de (all dumps + 200-frame clip) | byte-identical |
| other locked screens (verify_port.py render set: home, scan rain, term rain, scan glow, themes) | 56 / 56 renders byte-identical |

Notes:
- The mocks are drawn at 3x. The firmware draws each 1x pixel the way the mock paints its centre sample (3x+1, 3y+1); the
  locked @1x stills are exactly those samples, so @1x they match pixel for pixel. Against the @3x stills the only differences
  are the mock's sub-pixel detail inside 3x3 blocks that a 1x panel cannot show (1287 / 1505 / 1498 / 1726 sub-px for
  idle / mid / pulse / full; yellow in the diff panels, red would be a real 1x difference: none).
- Occlusion: the locked radar draws the hunted tower lit in front of every tower, so the meter is too (as in the mock).
  `-DLV_RD_HM_OCC=1` (not shipped) would put it behind nearer towers: 8549 px differ from the lock over the clip.
- Brightness: V2 caps added light at 200 luma (Rec.601) exactly as the mock code does, including the contact port glow just
  outside the glass. Max changed px outside the glass = the mock's own value (same RGB565).
- Live signal: target bands = (level x 14 + 30) / 60 with level = dBm + 100 clamped 4..60 (the tower-height level):
  -86 dBm 3 bands (22 %), -66 dBm 8 (55 %), -40 dBm 14 (100 %); NO SIG drains one band per packet.
- The sim replays the mock's fill schedule with `hmsched 26640` (sim-only hook, `LV_RD_HM_SIM`, never in firmware).
