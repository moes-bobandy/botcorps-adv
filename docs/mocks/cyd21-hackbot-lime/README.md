# MOCK 2026-11-26 (CYD-21 v2): HACKbot solid lime-green plate + black code rain

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash, no SD, no merge.
Branch `mock/cyd21-hackbot-lime-2026-11-26` on `build/v1.2.23-2026-11-26` (`a87eb83`).
Flag: `LV_CYD21_HACK_LIME` (default 0). With 0 the firmware path and every render are byte-identical to v1.2.23.

When the flag is on, HACKbot's home face plate is ONE flat solid lime (`#7CFF00`, RGB565 `#78FC00`). No metal ramp, no gradient, no sheen. The house chip-plate (blob 2) is copied only so the art's locked black eye sockets stay black; then pitch-black code rain is drawn with `skip=1` via `lv_pg_keep_feat` (same rain path as LOGbot / gold TERMINALbot). The panel, board, banner, pips, and every other page are untouched.

v2 (Mobandy 10:31 AM ET "No, I wanted solid lime green"): dropped the metallic ramp + traveling sheen from v1.

Renders (Vapor Grid, theme 22) are in `/workspace/out/fw-bigbot-1117/cyd21-hackbot-lime/v2/`.

Gate (`gate_c21.txt`): OFF build vs base a87eb83: 0 px on all 12 pager stills (pages 0-5, mid + full, theme 22). ON build vs base: 0 px outside HACKbot's plate (x65-174, y88-197); other pages 0 px everywhere.
