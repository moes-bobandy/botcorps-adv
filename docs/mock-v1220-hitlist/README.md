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
| 1c TERMINALbot: side pins purple, mini terminal gold, bot body grey/silver (home page colours) | `LV_TERM_RECOLOR` | lv_core.h | gate_item1c_terminal.txt |
| 1b TERMINALbot: board rain removed, face rain one size smaller (3x5) | `LV_TERM_NORAIN`, `LV_TERM_RAIN35` | lv_core.h, lv_term_norain.h (generated, tools/livescan/gen_term_norain.py), lv_port.h | gate_item1b_terminal.txt |
| 6 HACKbot game tower self-destruct countdown | `LV_DL_SELFDESTRUCT` | lv_scanbot_download.h | gate_item6_selfdestruct.txt |

Notes:
- Item 3: git history has no lost per-tower GIBSON sway. The Dojo's locked `mo11_dx`/`mo11_dz` law + tables were cloned verbatim, with a hash phase per tower.
- Item 1: the gold face is the TERM chip's own 48x48 silver face through `lv_pg_gold`, and the face rain uses the `lv_pg_rain_row` law at real glyph size (8 columns, not 18).
- Item 4 TERMINALbot: GIBSON's strip rect (y247..319) covers the bottom card labels and the mini terminal box. This is a conflict for Mobandy to decide.
- Item 2: the tearing during the slide is SPI-bound and is not fixed.
- Item 6: opening a tower arms a 12000 game-ms fuse on the SPEED clock (12 s / 6 s / 4 s real at 1X / 2X / 3X). Readout = lv_dl_banner frame law on the open tower, flashing under 3 s with the HOT red edge. At 0: target file still in the tower (or mid-download) = loss (SELF DESTRUCT banner, then the TRACED card retitled BLOWN UP); no target inside = tower blows, you are thrown out, nothing lost. Backing out with under 3 s left shows CLOSE CALL.
- Item 1b: the board rain is baked into the locked TERM clip, so it is pinned per pixel to the bare board (RGB565 table, themes 0..2). Themes 3..5 and 6..11 draw ghost bots over a grid there; the per-pixel rule smeared them, so those themes keep their background. Side pins + terminal box: unchanged (0 px), they already carry the colour UI.
- Item 1c: side pins (signal pads only; 3V/5V stay red, GND stays grey) use the home TERMINALbot page pad purples; the mini terminal uses lv_pg_pan_hackbot gold=1 (0xD4AF37 / 0xE8C050 / 0x140E06); the black DIP body is remapped to the home IC greys (107,117,132 and bevel lips), black bezel + gold face + black rain unchanged. Screen-border gold is left alone.
