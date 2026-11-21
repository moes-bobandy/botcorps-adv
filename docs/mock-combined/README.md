# Dojo combined mock (sim-only, not yet tested on your CYD). No bin, upload, flash or merge.
Branch mock/dojo-combined-2026-11-21 = 0513879 (rounds 1+2 via dc98ba3, 10 pillars, LV_MO_BG=1 brown grid) + c70ef04 (smoother drop).

Smoother drop = DROP-ONLY FRAMES (src/live/lv_morpheus_v13.h, MO_DROP_FAST / MO_DROP_X0, itw_mo_frame + 2 lines in mo11_overlay):
while the 3 folders drop (no folder open, no press flash, no close) the scene behind holds at the last full frame's clock and each
frame re-renders only the rows (x >= 84) a folder moved through. Frames drop to ~10-60 ms on the device model, so the locked law
(lv_select.h DROP0/GAP 110/DROP 380, ease, 65% landing, MO_DT_DROP 100 unchanged) is sampled ~10 times per folder instead of 5,
at real-time speed. First frame after the drop = normal full frame.
Proof: drop-only frames are 0 px vs a full render of the same held scene + folder clock (both 12- and 24-row bands checked on host).
Only the 20 in-between drop frames differ from 0513879 at 30 ms; landed folders 0 px vs locked c08935a; non-Dojo 0 px vs dc98ba3.

Files: dev_time_18_vs_10brown_vs_combined_3runs.txt, drop_positions_per_folder.txt, gate_px_vs_dc98ba3.txt, landed_folder_gate_vs_c08935a.txt,
eviltwin_drop_device_timeline_before_after_2x.png (rows 18 / 10 brown / combined, panel every 40 ms device time from the first folder),
eviltwin_landed_locked_vs_10brown_vs_combined_3x.png, eviltwin_middrop_10brown_vs_combined_3x.png,
dojo_combined_device_timeline_eviltwin.gif (3-up, 20 ms device time per GIF frame, each column starts at its tap).
