# Dojo folders check + 10 pillars brown grid (sim-only, not yet tested on your CYD)
No bin, upload, flash or merge.

## A) Folders
Same walks at every commit (tools/livescan/scripts/dj_dev.txt with rec; FLOCK = tap 120,160 and EVIL TWIN = tap 47,140 at t=17000; fold = tap scroll 1 at 33000).
Locked = c08935a "v1.2.11 LOCKED (Mobandy 10:33 AM ET)" (lv_select.h folder drop law).
- fullframe_diff_vs_locked_c08935a.txt: every frame of idle/open/drop/fold/close. 0a2038c, fd8efa8, cd5e118, cda0390, 4550a69, dc98ba3, abd538f (flag 0) = 0 px full frame.
  514df65 / 10-brown differ only where the other pillars / floor show (pillar count, background); "PILLAR 6/10" header.
- folder_body_diff_vs_locked.txt: card bodies+tabs (MO12 geometry, landed) and the opened pillar's folder list, bg-invariant pixels: 0 px for every commit from 0a2038c on, incl 514df65 and 10-brown.
- Only change found: v1.2.10 e11c7f2 -> 0a2038c (pre-lock, 2026-11-20 08:06 ET): tapped scroll = press flash (4 frames) + 2 px green open frame + footer facts. Part of the locked c08935a. Nothing changed after the lock.
- folder_region_diff_bginvariant_mask.txt: wider mask incl the gaps beside tabs/between cards; the 10-pillar residue there is other pillars seen through the gaps (verified by zoom), not folder pixels.
- devrec_stale_row_check.txt: every devrec panel row is the current or previous frame's row (0 stale rows); panel rows mix two frames mid-push (tearing).
- device_timeline_drop_*.png: devrec panel samples through the drop. cda0390 devrec tap lands at t=18285 on ALPR (predates aligned devrec taps).

## B) 10 pillars + brown grid (LV_MO_BG=1, cherry-picked abd538f, default set to 1 on this branch)
dev_time_10tatami_vs_10brown_3runs.txt (ref = 10 tatami, new = 10 brown). gate_px_vs_dc98ba3.txt: non-Dojo 0 px.
Stills: *_10tatami_vs_10brown_3x.png. "Brown grid" = A from abd538f: white floor/wall with dark Dojo-brown (MO11_SEAM 0x5c4a32) grid lines.
