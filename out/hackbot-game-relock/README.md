# HACKbot game re-renders (relock out folder)

Restored / re-rendered 2026-11-15 after the 2026-11-14 9:51 PM ET VM fork wiped /workspace/out.

Source: /workspace/cyd-hackbot-game-wt/src/live/lv_scanbot_download.h via the real C++ sim (sim_live). Native 240x320 @1x, nearest-neighbor 720x960 @3x. Game code and look were not changed. The locked store (cyd-lock-store 3503679, LOCKED/ui_game_hackbot) was not modified.

## Stills 12 and 14 (pending after the first lock)

Cause of the earlier mismatch: GIBSON camera DRIFT (`lv_rd_camx = 0.5 * sin(t * 0.45)`, rain mode 0). In the HACKbot game the drift clock is the play clock: `lv_dl_play()` / `lv_dl_new_round()` sets `w.t0 = lv_now`, and `lv_hc_swap` copies that onto `lv_rd_t0`. Padding before START cannot change framing at a given play time; the camera only moves with the play timer.

Capture therefore has to land on the 30 ms tick whose drift phase matches the sheet:

- 12 Extra: back to towers — 5430 ms play (L2 5.4S, TRACE 6%). A dump at 5400 ms is visibly shifted.
- 14 Extra: Level 5, 2 of 5 found — the tick that shows L5 9.9S and TRACE 11%. An earlier 9.8S / 10% tick is shifted and was also misread: the sheet's 3x5 nines were mistaken for eights; the sheet time+percent glyphs are pixel-identical to this still (L5 9.9S, TRACE 11%).

Remaining residuals vs the sheet tiles are rain/particle phase only, not camera framing, HUD, files, or labels.

Compares: compare/12_extra_back_to_towers.png and compare/14_extra_level_5_2_of_5_found.png
