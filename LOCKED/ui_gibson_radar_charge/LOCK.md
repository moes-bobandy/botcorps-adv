# LOCKED: ui_gibson_radar_charge (Gibson radar charge-lock current, D4)

- **Locked:** 2026-11-04 3:47 PM ET. Mobandy picked "Lock D4" on "What should I do with D4?".
- **Source:** `4a8197a75d614b16bfdc23f79beb2e16e03b3261` on `backup/cyd-livescan-2026-10-30` (botcorps-adv), `docs/mocks/radar-charge/D4/`.
- **This is a mock only.** No firmware was built and nothing was flashed. Porting it into firmware needs Mobandy's word.
- Adds to the locked `ui_gibson_radar`; the radar itself is unchanged (0 px changed outside the stream mask, 8 px margin).

## What is locked
- Tap to cycle towers (wraps, never jumps on its own); design A's HOLD TO LOCK bar button charges to the lock.
- Pure electric grid-line current from the lower control panel to the hunted tower: 2 px ice-blue core in an 8 px blue glow band, slow 2.4 s breathe, steady pulses every 40 px, 2 small soft arcs re-rolled every 4 frames, soft panel port and contact glow. No GIBSON faces or bot glyphs, no jagged bolt.
- The current flows constantly while the tower is hunted; it passes behind nearer towers (edge lines stay on top) and shows through glass at 13/16.
- Brightness capped at 176, below the top HUNT banner's yellow peak of 203, so it never overpowers the top meter.
- Clip: charge_D4_3x.mp4, 344 frames, 11.47 s, 720x960 (3x), H.264 yuv420p faststart, sha256 5ce0b0fa914d7b5f4175019ef14f07ecba9f8a9862c54ae2bb5bf62a1dba956a.

## Rules
- Never change this screen; only add.
