# DEVICEdojo (BOTpheus Dojo, LV_SC_MO=13) diagnosis and v1.2.10 fix (2026-11-19 ET)

Base: v1.2.9 = a51f4db. Fix branch: `fix/dojo-2026-11-19` (24f28c5, then e11c7f2). Nothing was flashed or copied to a PC.
Design followed: the newest Dojo lock, `/workspace/out/morpheus-mix/towers3c/EB3/LOCK.md` ("LOCKED (FINAL)", Mobandy, 2026-11-14 8:55 PM ET). The firmware Dojo has implemented that lock since 7105135 ("BOTpheus EB3 (006aa03 source)"). The look was kept: the Dojo field is pixel-identical to v1.2.9 when the same data is shown (see Gate).

## 1. Why the Dojo "totally doesn't operate" on hardware
| # | Root cause | Where (v1.2.9, a51f4db) | Effect |
|---|---|---|---|
| R1 | Touch was read **only between frames**, and one Dojo frame took about **1.07 s** on the ESP32 (QEMU icount; GIBSON radar takes 53 ms). A normal tap (about 100–200 ms down) starts and ends inside one frame, so the release-edge filter never sees it. | `src/live/lv_ui.cpp:108-114` `lv_read_tap` (the only `getTouch` call), called once per loop at `lv_ui.cpp:598`. The frame is `lv_draw.h` `lv_render` (13 bands of 24 rows) → `lv_morpheus_v13.h` `itw_mo_frame`. The cost is per-pixel float mixing in `botpheus/mo_mix.inc:316-320` (an unused `expf` per pixel, plus 6 `mo_mixc` per pixel). | Taps "do nothing". GIBSON is fast, so its taps work. This matches field video v2.mov. |
| R2 | The radio core freed the Dojo emblem cache (`mo_emb_free_all`) **while the UI core could be drawing those emblems**. | `lv_ui.cpp:277-280` `radio_heap_prep` | A cross-core use-after-free risk during the scan prep. |
| R3 | The Dojo showed **only built-in example data**. Nothing read the live scan table (`lv_f`). | `lv_morpheus_v13.h:73` `MO11_RSSI[] /* EX.DATA */`; the labels are at `:330`, `:347`, `:357-358` | This is why v1.mov shows "EX.DATA" everywhere. EX.DATA is the honest label on example data, which was all the Dojo ever had. |
| R4 | Tapping the TERM banner after opening TERM from the Dojo footer went to the pager, not back to the Dojo. | `lv_core.h` lv_tap banner block (the v1.2.5 `banner-home` branch at `:2576` takes the tap, so the `backbot-term-dojo` path at `:2484` is never reached) | v1.2.9 sim log: `tap 149,8 -> banner-home (scr=3)` |
| R5 | The Dojo ticker text leaked into the GIBSON ticker after leaving the Dojo (found in the fix build). | `lv_core.h` `lv_go` | Wrong banner text on GIBSON |

The video frame (`field_v1mov_5s.png`) matches the v1.2.9 sim idle render (`dojo_idle_v129@3x.png`). The pillars look white or washed-out because the camera is often moving: the device draws about one frame per second, so the view jumps between poses.

## 2. What changed v1.2.3 → v1.2.9 and what never shipped
- The Dojo commits since v1.2.3 were stability and wiring only:
  - v1.2.4: emblem LRU
  - v1.2.5: dirty-rect field, banner no-clear, 30 ms pace
  - v1.2.6: footer → TERM
  - v1.2.9: wider footer
- These are all compiled in. `LV_MO_INSIDE=1`, `MO_EB2=1` and `MO_BODY=5` are set in `lv_build_3locks.h`. No disabled flag, dead function or old screen path was found.
- **What never shipped:**
  - a tap that registers on real hardware (R1)
  - any live data (R3)
  
  Both worked in the sim only, because the sim has no frame cost.

## 3. Controls: every one tapped through the real dispatch (`lv_tap` via `rtap`, with CAL)
Fix log: `logs/logic_fix.log`. v1.2.9 log: `logs/logic_v129.log`. Stills: `fix_s00..s07@3x.png`.
| Control | Tap | v1.2.9 sim (0 ms frame) | v1.2.9 on device (frame cost) | v1.2.10 |
|---|---|---|---|---|
| BOTpheus face on pager → Dojo | 120,145 | works | works (pager is fast) | works |
| Pillar → open (search bar, folders, 3 scrolls) | 120,160 | works | **lost**: 4/10 at 300 ms, 1/10 at 800 ms | works, 10/10 at every cost |
| `< TOWERS` {170,247,239,285} → close | 200,265 | works | **lost** the same way | works, 10/10 |
| Folders / pillar / search while open | | `mo-open` (no action, by design in the EB3 lock) | — | same |
| Tap during the close animation | | `mo-camera-moving` (ignored by design) | — | same |
| Banner y≤23 → pager | 150,10 | works | lost | works |
| Footer y≥287 → TERMINAL | 120,300 | works | lost | works |
| TERM banner after the Dojo → back to the Dojo | 150,10 | goes to the pager (R4) | — | **back to the Dojo** |

Frame-cost sweep: 10 pillar opens and 10 TOWERS taps each, using the real tap filter and finger timing, with the tap landing at a random point inside a frame. Logs: `logs/m_F_B.log`.
| Frame cost | v1.2.9 | v1.2.10 |
|---|---|---|
| 0 ms | 10/10 | 10/10 |
| 150 ms | 8/10 | 10/10 |
| 200 ms | 10/10 | 10/10 |
| 300 ms | 4/10 | 10/10 |
| 500 ms | 2/10 | 10/10 |
| 800 ms | 1/10 | 10/10 |

## 4. Fixes in v1.2.10 (function and wiring only; the look is unchanged)
1. **Touch inside the frame.**
   - New `src/live/lv_tapfilt.h`: the same release-edge filter as before, shared by the firmware and the sim.
   - `lv_draw.h`: `lv_band_hook` runs after each band's blit.
   - `lv_ui.cpp`: `ui_band_hook()` samples touch after every band, but only while the Dojo is on screen.
2. **Dojo render about 5.5x faster, pixel-identical.**
   - In `mo_mix.inc`, the pillar side and front colours are computed once per column instead of per pixel.
   - Columns outside the current band are skipped.
   - The unused per-pixel `expf` sheen was dropped.
   - QEMU: **1070 ms → 194 ms per Dojo frame** (`logs/qemu_dojo_before_speedup.log`, `logs/qemu_dojo_after_speedup.log`).
   - Sim: 0 differing bytes across 300 frames (idle, open, close) compared with before the speedup.
3. **Safe emblem free.** `mo_emb_request_free()` asks the UI core to free the emblems and waits up to 2 s. `mo_emb_hold` stops new emblem builds until the radio has finished initialising.
4. **Live data, wired to the lock's pillars.**
   - `MO_HUNT[]` maps pillars to hunt matchers. `mo_live_update()` runs `lv_hunt_match` over `lv_f` (and `lv_deauth_n` for DEAUTH).
   - The live values feed the banner ("LIVE SCAN: N FINDS IN M PILLARS - TAP A PILLAR"), the header ("N LIVE 18 PILLARS"), the panel ("-61 DBM LIVE 1 SEEN"), the DBM 7-seg, the signal bars and the LED meter.
   - Entering the Dojo starts a scan if none is running and the table is empty, the same as GIBSON does.
   - Pillars with no live match keep their example values. The pillar heights and positions stay the locked, touch-optimised layout, so the footer still reads "HEIGHT = EXAMPLE SIGNAL".
5. **TERM banner from the Dojo returns to the Dojo**, and the ticker is reset when leaving the Dojo.
6. **Serial log** every 5 s: `[live] dojo N ticks avg X ms max Y ms`. This gives a real on-device measurement.

GIFs: `dojo_flow_v129_vs_fix@2x.gif` (idle → open FLOCK → TOWERS, with v1.2.9 EX.DATA beside v1.2.10 LIVE), plus `dojo_flow_fix@3x.gif` and `dojo_flow_v129@3x.gif`.

## 5. GIBSON "rain and movement gone" (reported separately)
- **Cause found:** after a HACKbot DL tower view, `lv_rd_cam_hold` stays true. It is set in `lv_scanbot_download.h:198`, but `lv_dl_leave` (`:144`) never cleared it. The GIBSON radar then skips its sway (`lv_radar.h:124-126`), so the towers stand still.
  - The bug is present in v1.2.7 (cfc8a27) and v1.2.9.
  - Fix: `lv_dl_leave` and `lv_radar_enter` now clear it.
- **Proof**, skyline rows 40–200, mean changed pixels per frame over 60 frames:

  | Path | v1.2.7 | v1.2.9 | v1.2.10 |
  |---|---|---|---|
  | Normal | 1322 | 1368 | 1368 |
  | After DL tower | 688 | 777 | **1377** |

  - On the normal path, v1.2.10 and v1.2.9 differ by 0 px over 60 frames.
  - GIFs:
    - `gibson_after_hackbot_dl_tower_v127_v129_fix@2x.gif`
    - `gibson_normal_v127_v129_fix@2x.gif`
    - `gibson_after_dojo_v127_v129_fix@2x.gif`
    - single @3x versions of each
  - After the Dojo, v1.2.10's GIBSON already shows towers, because the Dojo started a scan (`gibson_after_dojo_v129_vs_fix@2x.png`).
- **Rain:** no code path that removes the rain was found. The radar rain turns off only through CONTROLbot SCAN=OFF (`controlbot/lv_controlbot_panel.h:153`: `lv_rain = s.scan != 2`). SCAN=S.RAIN keeps the rain but stops the sway (rainmode 1). **Please check that CONTROLbot SCAN reads B.RAIN on the device.** The rain itself was not reproduced as missing in any sim build.

## 6. Gate (`logs/gate_v129_vs_fix.txt`)
- HACKbot, BOTpheus, GIBSON and CONTROL pager pages, the radar, DL and TERM: **0 px** different from v1.2.9.
- Dojo with no live data: field identical. Only the banner text differs.
- Dojo with live finds: only the live text and meters differ.

## 7. Build v1.2.10 (e11c7f2, BUILD_ID "v1019z e11c7f2", clean tree)
Files are in `/workspace/out/fw-bigbot-1117/v1.2.10/`:
- `BOTcorps-CYD-v1.2.10.bin`: app only, 2,652,416 B, sha256 e080c6a456f7829af0e1637d934ac4bfd1197f7ece6615b5cb4d8a554996ddec. This leaves 34,560 B of headroom in the 2,686,976 B ota_0 partition.
- `BOTcorps-CYD-v1.2.10-full.bin`: merged image to flash at **0x0**, 2,717,952 B, sha256 407bd84659d1cb04c65b5fcfc2277ab94121e07f1c5265cdf9f121b028c837b7. It equals the app plus 64 KiB, with the 0xE9 magic at 0x1000.
- RAM: 37.7% (123,548 B).
- Symbols present: `ui_band_hook`, `lv_touch_sample`, `lv_band_hook`, `s_tf`, `mo_live_update`, `mo_live_n`, `mo_emb_request_free`.
- Strings present: "LIVE SCAN: %d FINDS IN %d PILLARS - TAP A PILLAR", "[live] dojo %lu ticks…", "backbot-term-dojo".
- `lv_dl_leave` and `mo_emb_service` are inlined, so they have no symbols of their own; they are in source at e11c7f2.

## 8. Not tested or limits
- Not run on real hardware. The 1.07 s and 194 ms frame times are QEMU icount estimates; the new serial log will give the real numbers.
- QEMU was built with Wi-Fi scans disabled (QEMU's PHY asserts), so live data on the device is proven in the sim only.
- At about 5 fps the Dojo animations are still slower than GIBSON (about 19 fps). Taps no longer depend on frame rate.
- Emblems may blank for under 2 s while the radio starts up (`mo_emb_hold`).
