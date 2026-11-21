# perf-all TABLE (round 2, 2026-11-21) - HOST SIM ONLY, sim-only, not yet tested on your CYD

Device-timeline model (tools/livescan/dev_model.h): device ms = host thread CPU x 120.7, SPI 40 MHz + per-window cost, DMA overlap.
cda0390 and round 2 columns: docs/perf-all/dev_time_r2_vs_cda0390_3runs.txt (3 runs each, same session).
Round 1 column: docs/perf-all/dev_time_r2_vs_r1-4550a69_2runs.txt (2 runs, a later session; host noise is a few ms).
Pixel gate: docs/perf-all/gate_px_r2.txt (2041 frames at band rows 24 and 12, plus 918 theme-sweep frames: 0 px vs cda0390).

## Targets, plainly

| goal | target | result | hit? |
|---|---|---|---|
| 1 Dojo open / drop / fold | ~50 ms | 121.9 / 129.0 / 130.6 ms avg (cda0390 171.7 / 172.3 / 177.5) | **NO** (-25 to -29 %) |
| 1 Dojo idle | ~35 ms | 62.6 ms avg, 0 frames >100 ms (cda0390 91.5, 25/180) | **NO** (-32 %) |
| 1 Dojo CPU work (deterministic Ir) | - | 3.13 G instr vs 6.00 G (-47.8 %) | - |
| 2 HACKbot control pad | exact rewrite | span version, 60000-case unit test 0 differ; dl_play 36.2 -> 25.0, dl_open 43.3 -> 29.6, dl_pause 57.1 -> 44.3, dl_intro 30.7 -> 23.3 | **YES** (cache not done: ~38 KB RAM) |
| 3 Gibson panel on home | exact LUT / spans | atan2f calls cut ~10x by an exact bound test (0 misses, exhaustive); window cut to the 57x57 disc | **YES** |
| 4 Home pages | cheaper unpack, rain rows | still page now pushes changed rows only: KB/frame 146-150 -> 71-79, avg 51.6 -> 46.0 (p0) | **PARTIAL** (strip inflate still per frame; no RAM for a strip cache while idle) |
| 5 TERM / SCAN first frame | remove spike | max 141 -> 115-136 / 118 -> 96-111 ms across runs; still one ~100+ ms frame on entry | **NO** |

## All screens

| # | screen / clip | avg ms: cda0390 → r1 4550a69 → r2 | p95 ms: cda0390 → r1 → r2 | max ms: cda0390 → r1 → r2 | KB pushed/frame: cda0390 → r2 | frames >100 ms: cda0390 → r1 → r2 |
|---|---|---|---|---|---|---|
| 1 | dj_dev/dojo_fold | 177.5 → 155.9 → **130.6** | 239.4 → 173.6 → 154.0 | 245.9 → 224.4 → 174.2 | 146 → 95 | 88/90 → 58/60 → 87/90 |
| 2 | dj_dev/dojo_drop | 172.3 → 159.1 → **129.0** | 196.9 → 177.7 → 141.8 | 210.5 → 210.6 → 153.1 | 149 → 100 | 135/135 → 90/90 → 135/135 |
| 3 | dj_dev/dojo_open | 171.7 → 146.8 → **121.9** | 199.1 → 179.1 → 141.9 | 676.7 → 234.7 → 158.7 | 146 → 100 | 132/135 → 88/90 → 132/135 |
| 4 | pa_dev/dojo_enter | 110.8 → 79.9 → **62.7** | 157.4 → 87.7 → 69.6 | 429.3 → 89.1 → 74.0 | 149 → 112 | 40/120 → 0/80 → 0/120 |
| 5 | dj_dev/dojo_close | 101.5 → 89.3 → **73.0** | 145.0 → 128.2 → 110.9 | 176.6 → 161.3 → 125.4 | 147 → 110 | 44/135 → 23/90 → 23/135 |
| 6 | dj_dev/dojo_idle | 91.5 → 81.4 → **62.6** | 106.0 → 115.2 → 69.0 | 121.7 → 134.4 → 90.9 | 147 → 110 | 25/180 → 8/120 → 0/180 |
| 7 | pa_dev/dl_pause | 57.1 → 51.9 → **44.3** | 79.9 → 60.4 → 51.5 | 105.0 → 65.1 → 108.5 | 27 → 26 | 1/90 → 0/60 → 1/90 |
| 8 | pa_dev/home_p0 | 51.6 → 50.4 → **46.0** | 61.4 → 60.6 → 54.4 | 79.2 → 71.6 → 68.6 | 146 → 71 | 0/120 → 0/80 → 0/120 |
| 9 | pa_dev/home_p2 | 49.4 → 48.6 → **44.1** | 59.1 → 52.8 → 48.4 | 66.3 → 63.1 → 66.9 | 150 → 74 | 0/120 → 0/80 → 0/120 |
| 10 | pa_dev/home_p1 | 44.5 → 45.2 → **43.7** | 54.0 → 52.3 → 50.6 | 83.3 → 62.0 → 63.0 | 150 → 79 | 0/120 → 0/80 → 0/120 |
| 11 | pa_dev/dl_open | 43.3 → 42.5 → **29.6** | 63.0 → 60.0 → 53.7 | 99.0 → 73.4 → 64.4 | 58 → 51 | 0/120 → 0/80 → 0/120 |
| 12 | pg_dev/dl_pull | 40.1 → 38.4 → **28.5** | 58.2 → 56.5 → 55.0 | 78.7 → 68.9 → 65.5 | 50 → 44 | 0/180 → 0/120 → 0/180 |
| 13 | pg_dev/gib_pull | 39.2 → 34.4 → **31.2** | 62.2 → 52.7 → 51.8 | 98.8 → 59.7 → 71.1 | 60 → 56 | 0/180 → 0/120 → 0/180 |
| 14 | pa_dev/dl_play | 36.2 → 35.2 → **25.0** | 42.6 → 42.1 → 30.8 | 47.7 → 54.4 → 32.3 | 89 → 80 | 0/120 → 0/80 → 0/120 |
| 15 | pa_dev/dl_intro | 30.7 → 31.0 → **23.3** | 37.2 → 38.0 → 29.4 | 45.6 → 48.1 → 33.4 | 25 → 23 | 0/120 → 0/80 → 0/120 |
| 16 | pg_dev/gib_idle | 28.1 → 27.6 → **25.5** | 36.6 → 31.8 → 28.6 | 40.9 → 33.1 → 31.9 | 92 → 98 | 0/180 → 0/120 → 0/180 |
| 17 | pa_dev/gib_enter | 26.6 → 27.4 → **23.0** | 34.7 → 33.2 → 25.9 | 39.4 → 35.2 → 32.8 | 87 → 87 | 0/120 → 0/80 → 0/120 |
| 18 | pa_dev/hch | 25.6 → 27.8 → **25.3** | 34.5 → 41.0 → 31.0 | 41.2 → 48.5 → 40.6 | 41 → 45 | 0/120 → 0/80 → 0/120 |
| 19 | pg_dev/gib_sel | 19.5 → 17.6 → **15.3** | 38.6 → 32.1 → 27.9 | 39.6 → 32.1 → 27.9 | 55 → 58 | 0/6 → 0/4 → 0/6 |
| 20 | pa_dev/scan | 18.8 → 18.6 → **19.4** | 28.2 → 27.0 → 29.9 | 117.6 → 109.9 → 110.8 | 19 → 19 | 3/120 → 2/80 → 1/120 |
| 21 | pa_dev/term | 9.6 → 9.2 → **9.5** | 16.9 → 16.0 → 16.9 | 141.1 → 123.0 → 136.3 | 17 → 17 | 3/120 → 2/80 → 3/120 |
| 22 | pa_dev/gloss | 8.3 → 8.2 → **8.8** | 23.1 → 21.1 → 25.4 | 52.9 → 53.7 → 54.6 | 16 → 16 | 0/90 → 0/60 → 0/90 |
| 23 | pa_dev/tty | 5.7 → 5.4 → **5.7** | 18.1 → 18.2 → 18.2 | 35.2 → 35.7 → 36.2 | 22 → 21 | 0/90 → 0/60 → 0/90 |
| 24 | pa_dev/home_back | 4.2 → 4.0 → **4.0** | 3.3 → 3.1 → 3.2 | 65.5 → 54.5 → 54.0 | 14 → 14 | 0/120 → 0/80 → 0/120 |
| 25 | pa_dev/home_p3 | 4.1 → 4.1 → **4.1** | 3.2 → 3.2 → 3.3 | 51.5 → 50.2 → 48.9 | 14 → 13 | 0/120 → 0/80 → 0/120 |
| 26 | pa_dev/ct_inside | 4.0 → 4.0 → **4.1** | 3.2 → 3.2 → 3.6 | 49.3 → 49.0 → 51.9 | 14 → 14 | 0/120 → 0/80 → 0/120 |
| 27 | pa_dev/tr_inside | 2.8 → 2.7 → **2.7** | 2.9 → 2.8 → 2.9 | 3.2 → 3.0 → 3.0 | 11 → 11 | 0/120 → 0/80 → 0/120 |
| 28 | pa_dev/controlbot | 2.8 → 2.8 → **2.8** | 2.9 → 2.8 → 3.2 | 3.3 → 3.1 → 3.4 | 11 → 11 | 0/120 → 0/80 → 0/120 |
