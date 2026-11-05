# radar-current-meter-interact — MOCK ONLY (three variants)

Owner (2026-11-05 ~12:51 PM ET): *"Yes make sure the electric current is connecting and interacting while the tower hunt meter is o[n]"* (message cut off; read as "on").

## Scope
- **Mock only.** No firmware change, no build for the device, no flash, no new LOCKED entry, lock store untouched.
- **LOCKED D4 current** (`ui_gibson_radar_charge`, lock bd10318 / src 4a8197a, fw port 3b8b3de) is **not repainted**. Every frame comes from the host sim (`tools/livescan/sim_live.cpp`) running this tree's `src/live/lv_radar.h` (LV_RD_CHARGE=1). The current animates exactly as ported: breathing wire, packets every 40 px at 0.2 px/ms, arcs, and the hunt pan.
- **LOCKED A thermometer bands** (`ui_gibson_radar_hunt_meter`, lock 0320479 / src c83a27e) use `draw_A()` and `clear_top_meter()` imported unchanged from `docs/mocks/radar-hunt-meter/src/render_hunt_meter.py` (copied as `src/locked_A_render_hunt_meter.py`). The only change is that A's tower anchors now follow the hunted tower as the hunt view pans.
- The interaction layer is drawn **on top** of A. Nothing outside the hunted tower and contact area changes (checked: **0 px outside the mask**, all 200 sim frames, all three variants).
- The hunt panel (IGNORE / WATCH / STOP HUNT), tap cycle, and HOLD TO LOCK are the sim's own, unchanged.

## Proof the base is the locked look
- The sim hunting dump (t=26610 ms) matches `radar-hunt-meter/src/base_hunting@3x.png` with **0 px different**.
- A alone drawn on that dump at 55% matches the locked `radar-hunt-meter/A/A_mid@3x.png` with **0 px different** (`report.json: A_alone_vs_locked_A_mid_px`).

## How the current reaches the meter
The D4 route ends at the hunted tower's base row (1x y=163), at the right edge of the glass. That is the right edge of A's band stack, about at band 2. A alone paints its bands over that contact, so with A alone the current looks like it stops short of the meter. Packet arrivals use the locked packet law: head index ≡ (now/5) mod 40, so a packet reaches the contact about every 180–200 ms. In the 6 s clip that is 29 hits.

The clip's fill follows A's three locked fills: idle hunt 22% (3 bands) for 0–1.5 s, then up to mid 55% (8 bands) by 3.5 s, then full 100% (14 bands) by 4.8 s. **A band only lights when a packet lands.** The meter therefore climbs in steps driven by the current's packets. Once the fill reaches the signal level, later packets "top up" the top band with a softer flash.

| | name | what you see |
|---|---|---|
| **V1** | **FEED**: current feeds the fill | A soft ice contact port glows at the join and breathes with D4. A faint ice feed seam runs up the stack's contact edge, so the meter reads as plugged in. Each arriving packet becomes a bead that runs up the seam (120 ms) to the top band. On landing, the next band lights ice-white and settles into its own green, yellow, or red within 300 ms. |
| **V2** | **ARC**: current arcs into the low green bands | On each hit, a stepped D4-style arc (1x grid, soft ice, no forks) jumps from the contact into one of the lowest three green bands for about 120 ms. Those bands flash, and a brightness ripple climbs the lit stack 25 ms per band. When the ripple reaches the top, the next band lights. The contact port glows as in V1. |
| **V3** | **WRAP**: current climbs the tower edge beside the bands | The wire turns at the contact and climbs the tower's right edge, outside A's orange edge line (which stays on top), up to the current fill top. It uses the D4 band: 2 px (1x) ice core plus a 3-step blue glow and 2.4 s breathing, and ends in a steady charge tip with one small spark re-rolled every 4 frames. Packets keep climbing the wrap at 0.2 px/ms. When one reaches the top, a band lights. A soft contact glow sits where the wire meets the top lit band, so the wire grows as the bands climb. |

Brightness: pixels the current adds outside the glass stay under the D4 ceiling (176 luma). Band hit flashes inside the glass are capped at 200 luma, below the top banner's peak of about 203, so the meter interaction never outshines the top banner (max measured in the stills: 200.4).

## Files
```
A_alone/  A_{idle,mid,full,pulse}@{3x,1x}.png  A_alone_3x.mp4       # locked A alone, same sim frames + same fill steps
V1/       V1_{idle,mid,full,pulse}@{3x,1x}.png V1_3x.mp4
V2/       V2_...                                V2_3x.mp4
V3/       V3_...                                V3_3x.mp4
compare/compare_{idle,mid,full,pulse}@3x.png   # LOCKED A alone | V1 | V2 | V3, full screen
compare/zoom_tower_all@3x.png                  # tower + contact crop x2, rows idle/mid/full/pulse, cols A | V1 | V2 | V3
report.json                                    # hit times, landings per variant, still frames, px checks
src/run.sh  src/render_interact.py  src/hunt_current_meter.txt  src/locked_A_render_hunt_meter.py  src/locked_A_mid@3x.png
```
Still moments (same sim frame for A and all variants): idle t=1.20 s (3 bands), mid t=3.60 s (8), full t=5.49 s (14), pulse hit t=3.57 s (8 bands, one tick after a packet reaches the contact). Mid is the tick right after pulse, so V2's ripple is still fading in its mid still.

MP4: H.264, yuv420p, +faststart (moov before mdat), 720×960, 30/1, 180 frames, 6.0 s, one per folder. Sim frames are 30 ms ticks; video frame j shows sim frame floor(j·1000/900), the same mapping as the D4 encoder.

## Shortcomings
- The banner still reads −66 dB while the clip's fill climbs to 100%. The fill schedule is a demo of A's three locked fills, the same as the locked A mock. In firmware the target would come from live dBm.
- V3's wire runs up the tower's side face, where the D4 ceiling gives a paler, greyer ice than on the dark floor.
- In V1 and V2 most of the effect sits inside the 1x ~8 px band column, so at true 240×320 size the motion reads more clearly than any single still.
- Mock only. A firmware port would need the same luma checks and would draw after `draw_A` in the hunt pass.

## Re-render
`bash src/run.sh` (needs `/tmp/rci/zig` ziglang 0.13.0 and `/tmp/rci/zinc` zlib 1.3.1 headers; the box has no g++).
