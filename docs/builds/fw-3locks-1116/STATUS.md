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
