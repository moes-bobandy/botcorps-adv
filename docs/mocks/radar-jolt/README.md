# radar-jolt — MOCK (no firmware change, nothing flashed)

Owner, 2026-11-04 6:41 AM ET: "I want to tap to cycle and instead of a holding lock, how about a jolt of energy from the
lower control panel to the tower that lights it up and gives the hunt option".

## What changed vs the locked radar (LOCKED/ui_gibson_radar, rendered at 3240b03)
Only the lock gesture/effect. Everything else is the same firmware drawing code.
- Tapping empty space cycles the towers as before, with the same white cycle glow.
- **The hold is gone.** Tapping a tower (the highlighted one or any other tower) selects it and fires the jolt. Taps during the 360 ms strike are ignored.
- **The bolt** comes from the top edge of the mini terminal (the lower control panel, (110,287)) and hits the base of the tower.
  - Shape: a 10-segment jagged polyline that tapers at both ends (sin taper, max ±11 px), plus 2 forks.
  - It gets new jags every 30 ms frame, which gives the crackle.
  - On impact it throws 6 spark rays and a flash at the tower base.
  - It is drawn as a distance field. Hot core 0xffd8f6 (≤1 px), magenta 0xff2ad4 (≤2 px solid, ≤3.4 px at 50%), violet 0xb040ff halo (≤5.4 px at 25%).
- **Timing** (30 ms frames, 12 frames total):

  | Time (ms) | What happens |
  |---|---|
  | 0–150 | The terminal frame flares hot pink and fades. |
  | 60–150 | The bolt tip races up to the tower. |
  | 150–360 | Crackle. Brightness per frame is 1, 1, 1, .65, 1, .8, .95, .5, .35. |
  | 150–360 | The tower fills with light from its base up. |
  | 360 | Lock. |

- **Lit tower:** the tower is redrawn at full brightness in its *own* colour: the face gets an extra fill and the edges are at full brightness. It also gets a 2-ring halo in its own colour, with a short over-bright pop for 150 ms after the strike. It is never white and never hue-shifted. A watched tower lights lime, which is its existing watched colour. The tower stays lit while it is locked.
- After the lock, the IGNORE / WATCH / HUNT panel, the LOCK log line and the HUNT ticker are unchanged.
- **Kept** the magenta LOCK/HUNT corner brackets and label. They sit outside the halo and don't clash with the cyan.
- **Dropped** the white "lock snap" flash, because the strike replaces it.
- The tower that is jolted or locked no longer gets the white selection edges. It gets its own-colour light instead.
- The idle hint text changes to `TAP=CYCLE  TAP TOWER=LOCK`. It only shows when there are already nodes on entry, so it is not visible in this demo.

## How it was made
- `src/apply_jolt.py` patches a **scratch copy** of `src/live/lv_radar.h` taken at 3240b03 (the locked look).
  - All new code is behind `LV_RD_JOLT`, which defaults to 0. Built with `-DLV_RD_JOLT=0`, the sim's raw output is byte-identical to the locked render.
  - `src/lv_radar_jolt_on_3240b03.patch` is the resulting diff.
  - The real firmware in this branch is untouched.
- The sim is `tools/livescan/sim_live.cpp` with `-DLV_RD_JOLT=1`, running script `src/radar_jolt.txt`.
  - The script is `radar_lock.txt` with `rhold 97 140 1200` replaced by `tap 97 140` at the same moment (t=21840 ms). All later taps happen at the same times as in the old script.
- `radar_jolt_3x.mp4`: 720x960 (nearest-neighbour 3x), 30 fps, H.264 yuv420p, +faststart, 615 frames, 20.5 s.
  - The sim ticks every 30 ms (33.3 fps), so it was resampled to 30 fps at real-time speed. Every 10th sim frame is dropped; in the strike, that is the one at e=270 ms.
- `keys/`: 3x PNG stills straight from the sim (no compression).
  - `06_j06_jolt_mid_strike` is the mid-strike frame.
  - `08_k4_lit_tower_buttons` is the lit tower with the buttons.
  - `variant_HEAD_*` use the same patch on HEAD 06d4352 source, which has orange WiFi / blue BLE towers. It shows the lit tower follows the current colours.

## Pixel diff vs the locked radar (proof/pixel_diff.json), raw 240x320 frames, same script timeline
- Frames 0–140 (base, 3 cycle taps, up to the tap): **0 differing px**.
- Frames 141–196 (strike, then the terminal finishing typing the LOCK line about 660 ms earlier than the hold did): differences in the bolt path, the tower, the panel and the terminal. This is expected.
- Frames 197–635 (t 23550–36690 ms: locked, WATCH, HUNT + ticker, STOP HUNT):
  - 585–756 px/frame differ (mean 656.4).
  - All of them are inside an ≤18x56 px box on the locked tower (tower + halo + brackets/LOCK label, rows 117–172).
  - **0 px** differ in the panel, buttons or terminal (rows 247–319).
- Frames 636–683 (after IGNORE): **0 differing px**.
- k4 at t=23160 vs the old k4_locked:
  - 704 px in the tower box (77..93, 117..172).
  - 171 px in terminal line y304–308, because the LOCK line is fully typed in the new version and still typing in the old one.
  - Nothing else differs. See `proof/k4_pixel_diff_old_new_mask.png`.
