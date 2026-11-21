# Morpheus Dojo "glitchy" on the CYD, plus all-screens perf pass (2026-11-21)

SIM-ONLY. None of this has been tested on your CYD yet. No bin was built, uploaded or flashed.
Branch fix/dojo-glitch-2026-11-21, base cda0390 (v1.2.17).

## What makes the Dojo look glitchy (plain words)

1. **It draws too slowly, so it tears.** Each Dojo frame takes about 90 ms while idle and 160–170 ms with a pillar open. The
   screen is painted in strips from top to bottom (there is no RAM for a whole-screen buffer), and the pillars keep drifting.
   So the top of the screen shows one moment and the bottom shows a slightly later one. You see that as shear or jitter.
   Every close-up frame (open, scroll drop, scroll fold) is over 100 ms, before and after this fix.
2. **Opening a pillar froze the screen for about 0.6 s.** The big 40 px emblem was drawn from scratch the first time
   (6x6 supersampling with atan2 and sqrt per sub-pixel). The camera then jumped to catch up. Entering the Dojo had the same
   kind of freeze, about 0.38 s, while the small emblems were built.
3. **Device-only (the sim can't show this): emblems blank out and rebuild when the radio starts.** Every WiFi/BLE stack
   start frees all emblem caches between strips and holds new builds (mo_emb_request_free). Pillars lose their emblems for
   a moment, then up to 18 emblems rebuild in one frame, which is another long frame. The sim has no radio, so this is
   inferred from the code, not measured.
4. **The Dojo skipped the v1.2.17 changed-rows-only path.** itw_mo_frame used lv_blit_hw, which pushes all 296 rows
   (~149 KB) every frame even when only a few rows changed.

The white-flash glitch from earlier reports is not reproducible in the sim and is still open.

QEMU vs devrec: QEMU's steady numbers (idle ~95 ms, open ~149 ms) agree with devrec, so QEMU did not miss the steady cost.
It missed the one-off open and entry freezes and the radio emblem rebuilds.

## What changed (performance only; 0 px against cda0390)

- Emblems come from a flash bit-plane table (src/live/botpheus/mo_emb_table.h, 27,378 B, made by
  tools/livescan/gen_mo_emb_table.cpp) for sizes 4–18 and 40. That means no build on open or entry, and nothing to rebuild
  after the radio frees the cache. Other sizes still use the old cache.
- The Dojo frame now goes through lv_blit_dirty_hw (changed rows only).
- lv_mix has constant-divisor paths (/256, /8, /64). lv_lroundf replaces the libm lroundf with the same values. mo11_bl has
  a one-entry blend memo, and mo12_wash is a direct loop with a memo. The body-shade row loop is memoized.
- b_line, mo13_line, mo_aline and the kumiko lattice are clipped to the band's rows (same pixels, less looping).

## Gates (host sim)

- Pixel: docs/perf-all/gate_px.sh: TOTAL frames 2041 diff px 0 at band rows 24, and
  TOTAL frames 2041 diff px 0 at band rows 12. That is every frame of every pg_*, dj_gate and
  pa_gate script (Dojo idle, open, drop, fold, close, theme; Gibson; HACKbot; home pages; every screen in pa_gate).
- Deterministic CPU work in the Dojo scene (callgrind instruction count, docs/perf-all/ir_dojo.sh):
  ref 5,998,558,282 (100.0%)  PROGRAM TOTALS;new 4,864,383,825 (100.0%)  PROGRAM TOTALS;. That is 19 % less work.
- Device-timeline timing: docs/perf-all/dev_time.sh, 3 runs, SIM_DEV_K=120.7 (host CPU × K, so it is an estimate). See
  TABLE.md. The new dojo_enter max of 835.8 ms is a host-noise outlier in a single run. Five reruns gave a max of 89.8–98.6 ms,
  and the 380 ms entry freeze is gone.

## Results, honestly

- Fixed in the sim: the open freeze (max 604.5 → 198.6 ms) and the entry freeze (≈380 → ≈91 ms). With the flash table there
  are no emblem rebuilds after a radio start (device-only, unverified).
- Better but not fixed: Dojo frames are 7–14 % faster on average and push about 30 % fewer bytes. Close-up frames are still all
  over 100 ms, so the strip tearing (cause 1) remains. A faster frame rate is not the same as fixing the glitch: it needs
  your eyes on the CYD.
- Other screens, ranked worst-first in TABLE.md: no screen besides the Dojo has steady frames over 100 ms. The only
  over-100 ms frames are one-off first frames when entering TERMINALbot (~130–144 ms) and SCAN (~108 ms). Hotspots found
  but not changed in this pass:
  - Home pages (44–52 ms, ~150 KB/frame): board strips are re-inflated every frame, and the whole screen is pushed because the
    rain changes most rows. Fixing this needs a RAM cache, which the CYD's DRAM may not have.
  - HACKbot pause/play (36–52 ms): the bottom pad deck (octagons, discs, glows) is redrawn per pixel every frame, about half the
    HACKbot frame. An exact span rewrite is possible but riskier.
  - Gibson panel on home: per-pixel atan2f.
- "Home landscape" does not exist in this firmware (only portrait and the 180° ORIENT FLIP), so there is nothing to measure.
