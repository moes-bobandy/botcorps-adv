# Firmware port of three LOCKED mocks into cyd_livescan (2026-11-03)

Nothing was flashed and nothing was merged.

## What was ported (src/live/lv_port.h, generated src/live/lv_port_data.h, hooks in src/live/lv_core.h)
- A port layer sits over the exact firmware clip. It owns only the pixels the locks change:
  - HOME: 17616 px
  - SCAN: 7149 px
  - TERM: 4120 px
- Each owned pixel is one of three kinds:
  - static palette colour
  - rain slot (10 steps)
  - glow slot (lift, ring, or face tint)
- The clip draw and the tint repaint call lv_port_apply. lv_port_tick repaints:
  - only the rain runs, when the step changes (140 ms)
  - only the glow runs, when the phase changes (30 fps, 42-frame 1.4 s breath)
  - every repaint goes through the same tint transform and press flash as the clip
- Generators, all deterministic and reading only LOCKED inputs and the firmware clip:
  - tools/port/ext_home.py, ext_rain.py, ext_scan.py
  - gen_port.py
  - verify_port.py
- Other themes:
  - Themes 0-5 use a colour map derived from the clip. It was checked against the locked ORANGE boards: 100% within 16, p95 = 0.
  - Themes 6-11 use theme 3 plus the existing multi-ink.

## Pixel diffs (RGB565, sim_live built from the same lv_core.h), verify.json
| Screen / lock | Pixels compared | Result |
|---|---|---|
| HOME vs ui_home_tap_glow O, frames 7..147 (theme 2) | lock-drawn 10604 | 10604 exact |
| | glow over live clip 7012 | max diff 3 levels, mean 0.23-0.44 (lock base codec noise) |
| SCAN rain vs ui_main_bot_rain gaps3, 14 steps green + orange | 4120 chip px | 0 differ |
| TERM rain vs gaps3, 14 steps green + orange | 4120 chip px | 0 differ |
| SCAN vs ui_scan_glow peak (theme 0) | faces 3x529, halos 3x657 | max diff 2 levels |
| SCAN vs ui_scan_glow trough | same | max diff 2 levels |

Everything outside the port regions is the unchanged firmware clip. The lock frames came from older captures, so those areas differ (purple in the heat maps).

PNG files (lock | firmware sim | diff):
- home_n021.png (peak), home_n042.png (trough), home_n105.png
- scan_glow_peak.png, scan_glow_trough.png
- scan_rain_{green,orange}_n0{21,42}.png, term_rain_{green,orange}_n0{21,42}.png
- theme_{home,scan,term}_t{00,02,05,08}_peak.png
- home_morpheus_tap_flash.png (before | flash | after: the press flash on MORPHEUS still works)
- *_fw.png: the raw 1x firmware renders

## Build, sizes, gate
- pio run -e cyd_livescan: SUCCESS. The warning set is identical to the HEAD build, so there are 0 new warnings.
- firmware.bin is 2199152 B, versus 1800944 B at HEAD: +398208 B.
  - Flash use is 2198751 of 3145728 B (69.9%).
  - .flash.rodata grew by 394592 B. That is the port data; the largest table is the HOME glow table at 115 KB.
- IRAM (.iram0.text): 119463 B, versus 119463 B at HEAD, so +0 B.
- Static RAM: 117020 B, versus 116252 B, so +768 B.
  - The 19 KB glow-under buffer is on the heap, allocated once, like lv_shadow. A static buffer overflowed dram0 by 11632 B.
  - If the allocation fails, the glows stay still.
- Gate: tools/gate.py does not exist in this branch. The gate is tools/gate.sh, which runs tools/gate/gate.py, and I ran it read-only with OUT in scratch.
  - Port: 1042/1057 PASS, 15 FAIL. HEAD bfbef79 baseline from the same run: 1042/1057, 15 FAIL.
  - Every PASS/FAIL line is identical between the two.
  - The 15 failures are older Adv/hub items (L1/G1/H2/G19/G21). The RAM rows fail because the gate needs Cardputer elfs, which were not built.
  - This is not the 1044/1057 figure in the brief.

## Deviations, stated plainly
1. Two locks overlap on SCAN: the main-bot rain chip and the scan glow. They are applied on disjoint pixels, and gen asserts this.
2. HOME's BB_HOME tint changed from BGREY to FLASH, so MORPHEUS is not greyed.
   - HOME_INTRO still shows the grey BOOTbot.
   - Tapping MORPHEUS still opens the BOOTbot self-test (routing unchanged).
3. WIFI/BLE idle uses the face-only tint from the lock. The pre-existing whole-chip busy pulse while that radio scans is kept.
4. Glow pixels drawn over the live clip use the exact firmware clip as their base, not the lock's codec-noisy base. That is the source of the ≤3 level diffs.
5. The glow uses float math (powf, rintf). On the ESP32, newlib powf and FMA contraction could put some pixels ±1 level off the sim.
6. With BG MOTION OFF, the glow and rain freeze, like the clip.
7. HOLD: SCAN lock-on tower cycling was not started. The scan/radar lock code is untouched.
