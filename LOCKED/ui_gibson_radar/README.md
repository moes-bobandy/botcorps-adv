LOCKED ui_gibson_radar. Requested in Mobandy's 2026-10-31 CYD hit-list steer (item 8: Gibson radar ideas 1+2).

What this is: a FIRMWARE render, not a mock. gibson_radar_fw@3x.mp4 comes from the livescan sim (tools/livescan/sim_live.cpp), which compiles the real src/live/*.h code, at
botcorps-adv commit 3240b03351cb5079e1fb13708802299223c7cd5d (branch backup/cyd-livescan-2026-10-30). Script: radar_lock.txt (copy of tools/livescan/scripts/radar_lock.txt).
Re-rendered at 3240b03: the raw RGB is byte-identical to the frames this mp4 was encoded from.
Clip: H.264, yuv420p, 720x960 (3x of 240x320), 100/3 fps (one 30 ms sim tick per frame), 684 frames, faststart.
Covers:
- the base radar
- tap-cycle (tap empty space, glow moves to the next tower)
- hold-lock (1.2 s hold, LOCK bracket and label)
- the IGNORE / WATCH / HUNT buttons
- the hunt ticker (HUNT -> reads -> STOP HUNT)
- IGNORE

Mocks it implements (copied into refs/ and compared before locking; see refs/mock_vs_fw@3x.png):
- refs/mock1_tap_cycle@3x.mp4  sha256 b32a61bab0efa6c8b9520f330b56a56f495d9fdef3b92c4b172f2be23b697a5e (idea 1, tap to cycle)
- refs/mock2_hold_lock@3x.mp4  sha256 212e7e38e2b7314921c5a0f6ad4880a477e67f390028151b931f677f54750282 (idea 2, hold to lock)
- refs/mock1_key@3x.png, refs/mock2_key@3x.png, refs/mock2_lockon_moment@3x.png are key stills of the mocks.

Key frames are in keys/:
- k1_base
- k2_cycle_glow
- k3_cycle_3
- k4_locked
- k5_watch
- k6_hunt
- k7_hunt_ticker
- k8_ignore

Known differences from the mocks:
- The cycle glow is a white rect blend ported from mock 1. It does not brighten the tower artwork, unlike the item 5 press brighten.
- The IGNORE/WATCH/HUNT tap rows are 22 px tall: drawn 11 px, padded.

SHA256SUMS covers every file in this folder.
