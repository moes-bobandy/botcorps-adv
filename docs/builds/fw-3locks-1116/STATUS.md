# fw-3locks-1116 STATUS
Started: 2026-11-16 13:17 EST
Request: Mobandy 2026-11-16 1:17 PM ET — build CONTROLbot Color UI v6 + BOTpheus EB3 + HACKbot game into one firmware. BUILD ONLY (no flash/merge/publish/delete).
## Plan
1. Locate base worktree /workspace/cyd-portrait-wt @049ff59; baseline pio build env cyd (sizes).
2. Fetch locked sources: a87fe22/585810c (v6), 006aa03 (EB3), 3503679 (HACKbot).
3. Port sim code verbatim into firmware; wire nav; NVS theme persistence.
4. Build, merge_bin 0x0, FLASH.md, gate, framebuffer diffs vs locked renders.
5. Push branch backup/cyd-fw-3locks-2026-11-16.
## Log
- 13:19 worktree /workspace/cyd-fw-3locks-wt on backup/cyd-fw-3locks-2026-11-16 @049ff59; baseline builds running in /tmp/base049/wt
- 13:22 baseline 049ff59 cyd_livescan: RAM 110740 B, Flash 2348771 B / 3145728; env cyd: RAM 74172, Flash 1496047
- 13:25 EB3 source reconstructed (/tmp/t3c3b/tree = cyd-morpheus-v6-wt + 006aa03 tree_patch + eb3_tree_patched; src = 006aa03 morpheus-mix-spread/out/src + eb3 body patch). Sim reproduces locked idle_3x and open_deauth_3x at 0 px diff.
- 13:29 combined tree assembled (EB3 tree live headers + HACKbot lv_scanbot_download.h + botpheus/ mo_emblem.h, mo_mix.inc + lv_build_3locks.h). Sim: BOTpheus idle/open_deauth 0 px vs lock with LV_BOTPHEUS_NAME=0; 17 px (label MORPHEUS->BOTPHEUS, bbox x4-13 y27-31) with name=1.
- 13:32 HACKbot: combined sim == source sim (cyd-hackbot-game-wt) over a 750-frame playthrough (byte-identical); locked renders 01/02/03/13/15 = 0 px. First fw build: DRAM overflow 11320 B -> mo_build scratch moved to heap. cyd_livescan build OK: RAM 120404 (+9664), Flash 2430399 (+81628).
- 13:40 Color UI v6 screen added (LV_SC_CUI=14): 24 locked stills as RGB565 deflate strips (src/live/cui/lv_cui_data.h, 188,893 B, made by tools/cui/make_cui_data.py), 26-theme tables (themes_spec_v2.h / new_themes_v3.h verbatim), lv_set_theme_any 0..25 -> same NVS key lktheme, boot validation 0..25.
- 13:44 mo_hit 32-bit overflow (1L<<40 on ESP32 = no pillar ever hit) fixed with long long. Host renders unchanged.
- 13:45 regression vs 049ff59, all 14 tools/livescan scripts (277 dumps/recordings, 2,141 frames): 0 px diff (ticker tint off). With LV_CUI_TICKER_ALL=1: diffs only in the top banner y0-23 (0 px at y>=24).
- 13:46 verify_live.py gate: output byte-identical to 049ff59 (33 screens). tools/gate.sh: 1042/1057 PASS, 15 FAIL, 4 N/A on BOTH 049ff59 and this tree (identical output and renders; src/ui untouched; RAM rows fail for missing cardputer elfs, same as base).
- 13:48 theme persistence: CUI face tap -> NVS lktheme=24 then 2 (sim log). Boot with lktheme 22/24 -> home/term/scan/ctl/radar/hackbot bodies re-themed; BOTpheus art has no theme hooks.
- 13:53 decision: LV_CUI_TICKER_ALL default 0 (tinted banner only on Color UI) so other locked screens and BOTpheus amber banner stay 0 px at default theme. Commit 10e5b12.
- 13:56 builds @10e5b12: cyd_livescan SUCCESS RAM 120,444 (+9,704 vs 049ff59) Flash 2,625,279 / 3,145,728 (+276,508, 83.5%); env cyd SUCCESS RAM 74,172 Flash 1,496,047 (both unchanged vs 049ff59).
- 13:56 merged 0x0 bin: cyd_fw3locks_10e5b12_merged_0x0.bin 2,691,216 B sha256 b755c209100386cbb1459e17b9e2e81311bca5b660ccc412e76fcd069d46b68e (== pio firmware.factory.bin). FLASH.md written. NOT flashed.
- Per-screen diffs: compare/*.png (ref | render | red diff @3x), compare/diffs.json. BOTpheus 17 px (rename label only; 0 px with locked name). HACKbot 01/02/03/13/15 = 0 px, 07 = 48, 10 = 43 (timing phase); play-state refs 04-06,08,09,11,16-22 not reproducible (scripts lost) but the 750-frame playthrough is byte-identical to the locked source sim. Color UI 24 stills + alert = 0 px after RGB565 quantization.
## Open
- Nav: new screens reachable from terminal `go` commands only; home CONTROLbot still opens the locked CTL.
- Banner tint on non-CUI screens is off by default (lock conflict); -DLV_CUI_TICKER_ALL=1 turns it on.
- CTL theme bar still cycles the original 12 themes (a CUI theme 12..25 wraps into 0..11 on the next CTL tap).
- Boot / glossary MORPHEUS pixel art not renamed (locked art). No hardware test, no runtime heap number.
