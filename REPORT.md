# REPORT: fw-port-1105, hunt meter A + V2 arc/ripple in CYD firmware (NOT flashed)

**Source:** `b29f8abf17aa4c8ea0542dad9b574844e4e62807` on `backup/cyd-livescan-2026-10-30` (parent 057279f; firmware base 3b8b3de).
Ports LOCKED `ui_gibson_radar_hunt_meter` (A, lock 0320479 / src c83a27e) and `ui_gibson_radar_hunt_meter_interact`
(V2, lock 20faf17 / src 057279f) into `src/live/lv_radar.h`, `-DLV_RD_HUNT_METER=1` in `env:cyd_livescan`.
D4 charge lock (`ui_gibson_radar_charge`, bd10318) untouched.

## What changed
- While hunting: top banner's 11-bar meter removed (slot = banner bg + the mock's small tower glyph); HUNT/MAC, name,
  dBm, STEADY/READ text unchanged.
- 14 thermometer bands in the hunted tower's glass (draw_A): green base -> yellow -> red top; follows the tower as the camera drifts.
- V2: contact glow at the route end (breathes with D4); D4 packet law detects arrivals (head within 6 px of the contact,
  once per packet); each throws a stepped ice arc (120 ms) into one of the lowest 3 bands + a 25 ms/band ripple;
  150 ms later the packet lands: +1 band if below target, -1 if above, else top-band flash. Cap 200 luma.
- Target bands from real dBm: (level*14+30)/60, level = dBm+100 clamped 4..60 (-86 dBm 3, -66 dBm 8, -40 dBm 14); NO SIG -> 0.
- 1x rendering = the 3x mock's centre samples (exact for the locked @1x stills).
- Occlusion: `LV_RD_HM_OCC=0` (as locked: the hunted tower is drawn lit in front of all towers, so the meter is too).
- Sim-only hook `hmsched T0` (LV_RD_HM_SIM in sim_live.cpp) replays the mock's fill schedule; not in firmware.
- Formatting-only fixes for the 3 misleading-indentation warnings in lv_radar.h (from 3b8b3de).

## Sim verification (tools/port/verify_hunt_meter.py, verify.json)
| check | result |
|---|---|
| pre-hunt D4 sel/mid/full/lockfire/locked vs 3b8b3de | 0/0/0/0/0 px |
| 200 hunt frames vs locked V2 mock (re-rendered by its own code on 3b8b3de frames), RGB565 @1x | 0 px, 0 frames differ |
| locked V2_idle/mid/pulse/full @1x vs fw | 0/0/0/0 px |
| fw x3 vs locked @3x (sub-pixel detail inside 3x3 blocks a 1x panel cannot show) | 1287/1505/1498/1726 sub-px, all where the 1x centre matches |
| changed px outside meter masks (glass + contact + top slot), 200 frames | 0 |
| LV_RD_HUNT_METER=0 / LV_RD_CHARGE=0 vs 3b8b3de (all dumps + clip) | byte-identical |
| other locked screens (home, scan/term rain green+orange, scan glow, 12 themes) | 56/56 renders byte-identical |
| opt-in LV_RD_HM_OCC=1 (not shipped) vs lock | 8549 px over the clip (nearer tower over the left of the glass) |
| max luma of changed px outside the glass | 204.3 (Rec.601 on bit-replicated RGB565) = the locked mock's own pixel (same RGB565; 200.4 in its 888) |
| live dBm mode (no hook), TELLO -66 dBm | climbs 0 -> 8 bands, holds; STOP HUNT clears; HUNT restarts empty |

## Build (pio run -e cyd_livescan, clean, committed HEAD)
- Build ID `v1019z b29f8ab` (no -dirty). RAM 117,172 B (3b8b3de 117,060: +112 B .bss, .data +0). IRAM .iram0.text 119,463 = +0.
  Flash 2,209,375 B (+3,880). No new heap (state ~120 B static; ~0.6 KB stack in the draw). D4's one 9,264 B scratch unchanged.
- Warnings: 133 total vs 136 at 3b8b3de (the 3 lv_radar.h misleading-indentation ones fixed, 0 new); 4 pre-existing
  format-truncation warnings in lv_radar.h remain (log functions, unchanged).
- Gate (tools/gate.sh, read-only, host g++ = zig c++ + HOST_EXTRA=-fconstexpr-steps): 1042/1057 = 3b8b3de 1042/1057.
  Line diff: only G1's platformio.ini hash value (FAIL before and after; new flag line).
- esptool image-info launcher.bin: DIO, 40m, 4MB, checksum valid, SHA-256 valid. Merged 0x0 headers at 0x1000 / 0x10000: DIO 4MB 40m.

## Files
| file | bytes | sha256 |
|---|---|---|
| cyd_livescan_port1105_merged_0x0.bin | 2,275,312 | 09b2ca752fe8ddc8a386b7ee164e55f233ed74824a684a9268c983ab24bfc04c |
| launcher.bin | 2,209,776 | ece72d53a6b4f66d45511da080c0f14e55893c65d28fa3762fb45d046dc3b78a |
| fw_hunt_V2_3x.mp4 (6.0 s, 180 fr, 30 fps, 720x960 H.264 yuv420p faststart) | see SHA256SUMS | 9e5f57feb21e94f0ebde09bbbaf3f6fa86a5875471dd0ab0dd0608df110dfe25 |

Comparison PNGs: `cmp_V2_{idle,mid,pulse,full}@3x.png` (locked V2 | firmware | diff: yellow = 3x sub-pixel only, red = real
1x difference: none), `cmp_V2_zoom@3x.png`, `fw_V2_pulse@1x.png`; all fw stills in `verify/`.

Not flashed, not merged. Field risks: frame rate on the board (per-pixel float work on ~600 px per frame while hunting),
packet-arrival timing at the board's real frame interval (detection is once per packet at any rate).
