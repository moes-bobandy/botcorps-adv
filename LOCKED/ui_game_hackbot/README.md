# LOCKED — HACKbot extraction game (ui_game_hackbot)

LOCKED reference, Mobandy 2026-11-14 9:43 PM ET: "Lock the sheet and re-render the stills and video from source, matched to the sheet."

## Reference sheet (delivered in CYDbot chat)

File: `reference_levels_sheet_2026-11-12.png`  
sha256 `6b67f6a76902c54d3ddd514b1f12c431af6cd568a3dd9d6fb328aba9e5cab0a4`  
Size: 1872×2752, 237658 bytes.

Source (copy, never moved):  
`/home/box/agent-data/agents/c18300f9-2b0f-42e0-a342-d3bd5c97abe7/attachments/6b67f6a76902c54d3ddd514b1f12c431af6cd568a3dd9d6fb328aba9e5cab0a4.png`

The original mock folder `/workspace/out/hackbot-game-1112/` was not recovered. This contact sheet is the latest delivered artifact of that mock (title dated 2026-11-12). Individual stills and the playthrough video were re-rendered from source (`cyd-hackbot-game-wt` `lv_scanbot_download.h`, real C++ sim) and compared to this sheet. Only panels that visually match the sheet are copied into `renders/`. Others are listed as pending.

## Rules printed on the sheet (also locked 2026-11-12 4:46 PM ET, "please lock all that in")

LOCKED: 3 a tower, +1 every 2 levels, max 6. Find L files (max 5). 90 s. Wrong files add nothing.

- Trace is a 90 s timer only. Wrong files show ACCESS DENIED with the reason and never move the trace.
- Each level adds a target file, up to 5, each in a different tower.
- Towers add a file every couple of levels (L1-2: 3, L3-4: 4, L5-6: 5, L7+: 6 cap). 7 a tower is feasibility only, not in the progression.
- Folders shrink to fit the same area, with no scrolling.
- Clearing a level shows LEVEL n CLEAR with NEXT LEVEL. Getting TRACED offers a RETRY of the same level.

Required tiles 1-10 first, extras after.

## Re-renders locked in `renders/` (2026-11-14)

Source: `/workspace/cyd-hackbot-game-wt/src/live/lv_scanbot_download.h` via the real C++ sim (`sim_live`). Native 240×320 `@1x`, nearest-neighbor 720×960 `@3x`. Playthrough `hackbot_playthrough@3x.mp4` is h264 yuv420p at 100/3 fps, also `@3x`.

Full re-render output (all 22 panels, including pending) and side-by-side compares: `/workspace/out/hackbot-game-relock/` (compare contact: `compare_contact.png`).

Game look was not changed to chase a pixel match. Capture timing and play path were aligned to the sheet (timer, overlays, typist, which tower/file). Remaining residuals are noted below.

### Visually match (copied into renders/)

Pixel-identical to the sheet tile (scaled 360×480 nearest): 01, 02, 03, 10, 13, 15.

Visual match of scene, files, HUD, and times, with only background rain/particle phase (and in one case a typist cursor or ticker phase) differing — not a different game look:

- 01 Level 1 intro, 1 file — pixel identical
- 02 Level 2 intro, 2 files — pixel identical
- 03 Level 3 intro, 3 files — pixel identical
- 04 Progress 1 of 2 found — rain/particles only
- 05 Wrong pick ACCESS DENIED (overlay already down) — rain/particles only
- 06 Level 1 CLEAR — card text/score/time identical (860, 12.9s, 14%, towers 3/5, wrong 1); rain behind the card
- 07 After NEXT LEVEL — typist at "1 DAVINCI"; re-render has the mid-type cursor block on the next line (108 px)
- 08 Level 2 CLEAR — card text/score/time identical (870, 12.2s, 13%, found in towers 2 5); rain behind the card
- 09 TRACED — lose — card text identical (got 1 of 2, missed T5 F3, 90.0s); rain behind the card
- 10 RETRY same level — pixel identical
- 11 Extra: file found, not last — rain/particles only
- 13 Extra: Level 5 intro, 5 files — pixel identical
- 15 Extra: Level 6 still 5 files — pixel identical
- 16 Extra: Level 1 wrong pick — rain + terminal ticker scroll phase
- 17 SIZE: 3 files a tower (L1-2) — rain/particles only
- 18 SIZE: 4 files a tower (L3-4) — rain/particles only
- 19 SIZE: 5 files a tower (L5-6) — rain/particles only
- 20 SIZE: 6 files a tower (L7 cap) — rain/particles only
- 21 SIZE: 6 files, wrong pick (L7) — rain/particles only
- 22 Feasibility only: 7 a tower — rain/particles only

### Pending (not copied into renders/)

- 12 Extra: back to towers — HUD and game state match the sheet (L2 5.4s, TRACE 6%, T2 checked, ELLINGSON found). The 3D camera framing of the tower field is visibly shifted (towers sit further right than on the sheet).
- 14 Extra: Level 5, 2 of 5 found — same class of camera-framing shift. Also TRACE reads 11% on the sheet vs 10% from current source at L5 9.8S (source formula: 9810 ms → trace 109 → 10%).

Those were not locked. Stills for them remain only under `/workspace/out/hackbot-game-relock/stills/`.
