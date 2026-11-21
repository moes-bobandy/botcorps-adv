# Dojo 10 pillars mock (sim-only, not yet tested on your CYD)
Branch mock/dojo-10-pillars-2026-11-21 (off dc98ba3). Current tatami/shoji background (LV_MO_BG not applied).
Pillars: AIR TAG, EVIL TWIN (in KEYFOB's slot 5), DEAUTH, META, HACK, SKIMMER, FLOCK, FLIPPER, DRONE, SAMSUNG.
Every [MO_N] table is row-selected verbatim from the 18 set (KEEP = 0,1,2,3,4,16,6,7,8,9); MO_EID maps each slot to its old id for emblem, seeds, mac, kind.
Placement: each device keeps its OWN existing MO11_SX/Z0/drift entries - no re-spread, no new numbers, no guesses. pillar_positions.txt shows X/Z/H identical to the 18 build for all 10.
EVIL TWIN therefore sits where it already was (left, X -2.22 Z 3.22), not at KEYFOB's old screen spot.
Gate: non-Dojo 0 px vs dc98ba3 (2959 frames, bands 24/12); only 28 Dojo clips differ (expected).
Timing: dev_time_3runs.txt (ref = 18 @ dc98ba3, new = 10). Stills @3x: *_18_vs_10_3x.png. GIF: device-timeline 1x, 60 ms step, EVIL TWIN walk.
No bin, upload, flash or merge.
