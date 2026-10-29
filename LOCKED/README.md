# LOCKED — CYD lock store (rebuild started 2026-10-29)

## What was lost
At about 1:05 PM ET on 2026-10-29 the box restarted, and these paths were lost:
- /workspace/out/cyd-full-board-build/ (the old LOCKED store: locks.tsv, reference PNGs, harness/poke, harness/gp,
  scripts/, pxdiff.py, backups; the hub15-mock sim harnesses zapgib8 and zapair; the rain/v4/send clips; repro folders;
  shock-controls/ASSIGN.txt)
- /tmp/p4/C (the hub15 chain source: botcorps_ui.cpp with `{ GL_AP, -1 }, /* 2 deauth */` at about line 4682)
- /tmp/cydpaint-venv and /dev/shm

No copy of any of these was found on the box, in any git branch, stash, reflog or worktree, or on origin
(`git ls-remote origin` checked 2026-10-29).

## Lock state before the loss (from chat records, not files)
- GIBSON v8: LOCKED 12:51 ET, lockcheck 1788/1788. Sums: GIBSON f4a4dd14…, overall 58d23bf1…, backup c15c9b67…
- SCANbot pin ring re-lock 12:24 ET, saved as pins_ring_03 (5b7ecebd…), 63/63
- AIR TAG v3 and KEYFOB v2: lock started 13:03 ET, NEVER completed
- ASSIGN.txt (233f8ab7…): A shock button = HACK, FLOCK, DRONE, SAMSUNG; B SHOCK trigger + gauge = GIBSON (later F), KEYFOB, META,
  SKIMMER; C dial + FIRE = AIR TAG, DEAUTH, FLIPPER; F capacitor bank (OVERLOAD, 1-2-3 lamps, SHOCK 24kV, no silk) = GIBSON

## Surviving visual references (Grok Bot chat attachment cache, sha256, 720x960)
| clip | sha256 |
|---|---|
| game_gibson_v8@3x.mp4 (LOCKED ref) | 7ec392a3c297ddc9c8fd86b36c6eac0ae68fdfb9afebd2032e2e2f8dec1ddea9 |
| game_airtag_v3@3x.mp4 | 792bb75790674d4a719f8d4e3ff5f50bfd410ba8ec26138c0f5eff1ede863f3d |
| game_keyfob_v2@3x.mp4 | 8998efeae310e2a84f33c9cc07ed2f61d40c40af37ada75cc7296848d605ed0b |
| airtag_v3_cmp_v2.png | 4ea36ac55a9a5661612385dbb52156cc239accee9eaba8d20e7d2d622dd1eac8 |
| keyfob_v2_cmp_v1.png | 48d3d87b010b6b59d503f410a55f53c46eeca25d0bdad216e2e9e3256c9c4367 |
| F_shock@3x.mp4 | 48a24ffb10dda326a898dbb6b6fa321a54079f3df89d68fc271683036de82a11 |
| game_airtag_v2@3x.mp4 (old AIR TAG lock) | 176b98e23375a902c169a505960d247e5205a70e44dfc5a4863354475eba42f2 |
| D_shock (LOCK/PURGE, identified by frame) | 43c5faf6… |
| E_shock (LOCK/OVERLOAD tesla, identified by frame) | 8b60306d… |

## Rule from now on
Mobandy, 1:59 PM ET 2026-10-29: "back up the lock store to GitHub after every lock."
