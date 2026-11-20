# LOCKED: sub-screen selection layout (pull-up + folder drop)

Locked by Mobandy 2026-11-20 10:33 AM ET: "that needs to be the locked layout for subscreen selections".

One module, `src/live/lv_select.h`, is THE selection for every sub-screen (Dojo / MORPHEUSbot Construct,
GIBSON radar, HACKbot DOWNLOAD, and any future sub-screen). No screen keeps its own copy of the law.

1. Pull-up: the camera eases onto the selected tower (`lv_sel_cam`, cubic ease-out `lv_sel_ease`) over the push time
   (Dojo 400 ms push + its close-up zoom, GIBSON / HACKbot 400 ms pan to x 50).
2. Folder drop: folders fall from the top of the scene (`lv_sel_drop`: start y = LV_RD_Y0 - 50, start + j * gap,
   eased over the drop time; Dojo / GIBSON / HACKbot all 110 ms gap, 380 ms drop).
3. Landed = 65 % of its drop (`LV_SEL_LAND`, `lv_sel_landed`): drawn solid and tappable; a falling folder is not.
4. Tap: a landed folder is a button (`lv_sel_hit` over the screen's >= 40 px finger boxes) with lv_press feedback.
5. Clock: `LvSelClk`; while the pull-up / drop plays one frame advances it by at most 50 / 100 ms, so a slow frame
   never skips the pull-up. At the 30 ms cadence the cap never binds (every frame = the locked frame).

Per screen (not shared): folder geometry and art, colours, folder actions, camera target, drop start time
(Dojo: after the search bar types the name; GIBSON / HACKbot: 300 ms).

Proof: sim gate vs v1.2.10 = 0 px on Dojo, HACKbot, pager, CONTROLbot after the refactor; GIBSON and Dojo slow-frame
flows byte-identical to the pre-module commit.
