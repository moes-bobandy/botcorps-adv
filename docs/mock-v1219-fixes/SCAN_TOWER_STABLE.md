# "Do whatever you did on scan tower to keep it stable" — investigation

HOST SIM ONLY, no bin. Nothing here was flashed or tested on the CYD.
Asked by Mobandy 1:17 PM ET (pt1569u). Translucent / device-tinted towers were
rejected as a fix and were not mocked.

## Mechanism on the GIBSON scan tower (exact)

There is no special present path that keeps moving scan towers from tearing.
What landed in **ea8637e4f76b226e522b98ff397800d8dea64ae4** (pullup-glitch,
docs/pullup-glitch/ROOT-CAUSE.md) was:

1. **Dirty-row push** (`lv_core.h` `lv_dr_push` at **:2393**, `lv_blit_dirty` at
   **:2410**). `lv_radar_frame` presents with
   `lv_render(lv_blit_dirty, LV_R_RADAR, lv_scene_radar, 0)` at
   `src/live/lv_radar.h:1632`. Only rows whose final 240-wide hash changed are
   sent, plus 1/32 insurance. Consecutive dirty rows go as one SPI window.
   RAM is the 1,280 B hash table (heap). No extra GRAM, no 142 KB field buffer,
   no panel readback.
2. **INFO-tower stroke clipping** (same commit): `lvi_line` / `lvi_micro` only
   visit the current band's rows, so a pull-up frame is no longer ~13× a landed
   frame (~800–1,350 ms → ordinary frame time). That is what made the *pull-up*
   "a little better" / "stable" on the model (45–171 of 180 mid-push pull frames
   over 100 ms, down to 0–11 outliers).
3. Already in place before that commit, shared with every overlay including the
   Dojo: v1.2.11 ping-pong bands (`lv_draw.h:18-24`, `lv_ui.cpp:111-123`,
   `lv_band_pp` set in `lv_ui.cpp:695`) and time-based motion
   (`lv_now` is set once at frame start; camera law
   `lv_radar.h:153-157` `0.5f * sinf(t * 0.45f)`).

Idle camera sway is the same law the Dojo already calls
(`lv_morpheus_v13.h` idle: "EXACTLY the Gibson radar's layout").

## The Dojo already has the transferable part

`itw_mo_frame` presents with `lv_render(lv_blit_dirty_hw, {0,24,239,319},
itw_mo_scene, 0)` at `src/live/lv_morpheus_v13.h:843`. That is the same
`lv_dr_push` helper, introduced for the Dojo in
**e732a7cb792dcf4b3e472f1a86c055bfcef329d1** (dojo-glitch / perf-all).
Cloning `lv_blit_dirty` / `lv_dr_push` "verbatim" into the Dojo would be a
no-op. The INFO-stroke clip does not apply (the Dojo has no INFO monolith fade).
Ping-pong and the timebase are already shared.

`LV_MO_COHERENT_PRESENT` (3c25c82) is a *sim panel latch*. It is not this
mechanism, and the device hook is still null. It was not reused here.

## Device-timeline evidence: the scan tower still tears

Same harness as the Dojo idle baseline (devrec 24 frames, camera every 20 ms,
`sim_term` so the Dojo fence is off — the real `lv_dr_push` path).

| clip | K | mixed cam frames | mean frame | mean CPU | ~fps |
|---|---|---|---|---|---|
| GIBSON idle | 1 | **15/36** (seam e.g. y=137) | 21.91 ms | 0.126 ms | 45.6 |
| Dojo idle | 1 | **21/36** (seam e.g. y=141) | 22.68 ms | 0.303 ms | 44.1 |
| GIBSON idle | 120.7 (board-timing) | **23/36** | 27.09 ms | 14.36 ms | 36.9 |
| Dojo idle | 120.7 | **33/46** | 38.16 ms | 33.74 ms | 26.2 |

At K=1, SPI of ~54–56 k dirty pixels (~22 ms at 40 MHz) dominates both screens,
so they tear the same way: a horizontal seam, old pillar/skyline geometry above
it, new below it. At K=120.7 (idle GIBSON CPU calibrated to the QEMU 16 ms
point) GIBSON is faster than the Dojo (~37 vs ~26 fps) and still mixed on
**23/36** cam frames. Dirty rows and a faster frame shrink the window; they do
not make a mid-push sample a coherent image.

Proofs (native C++ sim, @3x nearest-neighbor):
`/workspace/out/fw-bigbot-1117/v1219-fixes/dojo-scan-mech/proof/`
- `scan_tower_also_tears@3x.png`
- `scan_vs_dojo_seam_zoom@4x.png`
- `gibson_idle_cam_split@3x.png` / `dojo_idle_cam_split@3x.png`

No firmware pixels were changed in this layer, so completed Dojo frames stay
0 px vs c70ef04, every non-Dojo screen stays 0 px, and the locked drop law is
untouched. There is no fps cost vs baseline because nothing was cloned.

## Not implemented (waiting on Mobandy's word)

**One-block-per-tower redraw.** After the frame is composed (same scene, same
`lv_now`), present each tower as one rectangular window (its `lv_rd_box` /
Dojo front-face box) instead of full-width dirty *rows*. A mid-SPI camera
sample could then show some towers from the old pose and some from the new, but
it would not cut a single pillar in half on a scanline. That still is not an
atomic flip (and it needs a small per-tower staging rect, not a 142 KB field
buffer). It is a different visual (towers popping complete) and is only a
proposal here.

See-through / device-tinted towers were not mocked: they do not fix the
timing and they add blend cost.
