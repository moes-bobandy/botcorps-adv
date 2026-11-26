# MOCK 2026-11-26 (CYD-17): terminal-prompt role panels on the home pager

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash.
Base: v1.2.22 source acd4c7b (Vapor Grid default). Branch mock/cyd17-panel-terminal-2026-11-26.

What changed: on each big-bot pager page (BOTpheus, HACKbot, GIBSONbot, TERMINALbot) ONLY the small role panel,
the locked 152x71 plate at (45,220)-(196,290), becomes a typed terminal prompt: `> Name`, `> role: ...`, 3 short lines.
CONTROLbot's live-controls panel is untouched in the main set (page 4 gate = 0 px). The CONTROLbot overpaint is opt-in
(`-DLV_C17_CTL=1`, off by default, hides the live controls, NOT rendered here).

Look: TPS **F2** (mock 0d64cf3, lv_term_ps.h): 5x7 mixed case, 6 px pitch, racing digits snap + lock cell by cell, CRT odd-scanline
dim, 1 px bloom. F2 vs G2 not picked yet; G2 not rendered. Only the line being typed races digits (later lines stay dark).
Font color = live theme, never hardcoded: per pixel `lv_theme_at(lv_theme, x, y)` -> `sec_legible` -> `lv_mix3` 6-role ramp
(same ramp as 0d64cf3), so single and multi-color themes both follow. The plate frame keeps the v1.2.22 `lv_pg_pan_plate` call.

## Roles (DERIVED: no role field exists in source). Every line is tied to source; no placeholders needed
| Bot | On-panel lines | Derived from |
|---|---|---|
| BOTpheus | role: dojo construct / 10 device pillars / tap pillar: 3 folders / search bar on top | lv_morpheus_v13.h (title DOJO CONSTRUCT; `MO_N 10`; tap = push-in, 3 DOWNLOAD-structured Dojo folders, search bar on top); old panel caption DOJO/CONSTRUCT in lv_pager_fx.h. (Red pill and 12 pillars were older versions: dropped.) |
| HACKbot | role: hack game / 1 file in 5 towers / tap file: download / trace timer: 90 s | lv_core.h `go hackbot` -> LV_SC_DL "HACKbot game"; lv_scanbot_download.h (find the ONE file, `LV_DL_NT 5`, tap file = download, `lv_dl_tms()` = 90000) |
| GIBSONbot | role: home radar / 3D tower scan view / tap tower: lock / tower height = RSSI | lv_core.h `bots` ("GIBSONbot home") and LV_CS {"home","GIBSONbot"}; lv_radar.h (3D tower scene, `lv_rd_level` = rssi height, `lv_rd_lock`); old caption SCAN/RSSI |
| TERMINALbot | role: term / sweep 10 device bots / scan lock zap purge / restart | lv_core.h `bots` ("TERMINALbot term"); lv_inside_towers.h Relay Ring (`itw_nt = 10` device bots, `sweep <dev>`); botcorps_ui.cpp:7615 core "scan lock zap purge restart" |
| CONTROLbot | (not changed) | `bots` says "CONTROLbot ctl"; only for the opt-in extra |

The word "role:" and the short phrasing are mine; the facts under each are the source above. Wording is easy to change.

## Gate (tools/gate/v1220_gate.py, host sim, last frame of each page, 240x320)
See gate_c17.txt: themes 22 Vapor Grid, 23 Amber CRT, 9 Prism; pages 0-3 changed only inside (45,220)-(196,290), 0 px outside;
page 4 CONTROLbot 0 px anywhere; each at two typing times (mid + full). Default build (LV_CYD17_PANEL_TERM=0) vs v1.2.22: 0 px.
gate_gif.txt: all 135 GIF frames 0 px outside the rect.

## Files
- cyd17_01..04: one page each in Vapor Grid (full 240x320, @3x + @1x)
- cyd17_05: GIBSONbot in Amber CRT (single-color-style theme)  - cyd17_06: TERMINALbot in Prism (multi-color per pixel)
- cyd17_07: BOTpheus mid-typing (racing digits)  - cyd17_08: GIF of typing, panel crop at 4x (crop, not full screen)

## Honest notes
- The mock uses a 21 KB static panel buffer for the glow pass. A firmware port would need a row-wise draw (dram0 is tight).
- Vapor Grid ink at the panel's height is the blue/lavender end of its gradient, so the text is not pink.
- Lowercase p/q/g/y have no descenders in the 5x7 GLCD font (same in the TPS mocks).
- Typing restarts each time you swipe to the page. Panel redraws every tick on these pages (as the v1.2.22 role panels already do).
