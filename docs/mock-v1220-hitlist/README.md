# mock/v1220-hitlist-2026-11-24 — MOCK ONLY (host sim, not in any bin)

Base: v1.2.19 `42587a5`. No bin was built, uploaded or flashed. Renders: `/workspace/out/fw-bigbot-1117/v1220-hitlist/` (one screen per file).
Every layer has its own flag; set it to 0 for byte-identical v1.2.19 behaviour on that item.

| item | flag(s) | files | gate |
|---|---|---|---|
| 1 TERMINALbot sub-screen gold + face rain, no icon rain | `LV_TERM_GOLD` | lv_port.h, lv_core.h (lv_scene_tint hook) | gate_item1_terminal.txt |
| 2 banner flicker on pager slide | `LV_PG_SLIDE_SPLIT` | pager/lv_pager.h | gate_item2_banner.txt |
| 3 GIBSON independent towers + banner name/colour | `LV_RD_TW_DRIFT`, `LV_RD_SEL_BANNER(_HOOK)` | lv_radar.h, lv_core.h | gate_item3_gibson.txt |
| 4 control strip: Dojo (white), TERMINALbot | `LV_MO_STRIP`, `LV_TERM_STRIP` | lv_morpheus_v13.h, lv_core.h, lv_port.h | gate_item4_dojo.txt, gate_item4_term.txt |
| 5 HACKbot game SPEED + independent towers | `LV_DL_SPEED`, `LV_DL_TW_DRIFT` | lv_radar.h, lv_radar_pad.h, lv_scanbot_download.h | gate_item5_hackbot.txt |

Notes:
- Item 3: git history has no lost per-tower GIBSON sway. The Dojo's locked `mo11_dx`/`mo11_dz` law + tables were cloned verbatim, with a hash phase per tower.
- Item 1: the gold face is the TERM chip's own 48x48 silver face through `lv_pg_gold`, and the face rain uses the `lv_pg_rain_row` law at real glyph size (8 columns, not 18).
- Item 4 TERMINALbot: GIBSON's strip rect (y247..319) covers the bottom card labels and the mini terminal box. This is a conflict for Mobandy to decide.
- Item 2: the tearing during the slide is SPI-bound and is not fixed.
