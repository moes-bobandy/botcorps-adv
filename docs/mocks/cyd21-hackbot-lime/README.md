# MOCK 2026-11-26 (CYD-21): HACKbot lime-green metallic plate + black code rain

Mock only, sim-only, not yet tested on your CYD. Not in any bin. No build, no flash, no SD, no merge.
Branch `mock/cyd21-hackbot-lime-2026-11-26` on `build/v1.2.23-2026-11-26` (`a87eb83`).
Flag: `LV_CYD21_HACK_LIME` (default 0). With 0 the firmware path and every render are byte-identical to v1.2.23.

When the flag is on, HACKbot's home face plate uses the same finish + black-rain pipeline as LOGbot (`696080e`): the house chip-plate (blob 2, the same art TERMINALbot and LOGbot already use) is remapped through a metal ramp + traveling sheen, then pitch-black code rain is drawn with `skip=1` via `lv_pg_keep_feat`. Only the colour ramp is swapped to lime green (mid around `LV_PG_LIME` 0xC8FF10). The art's locked black eye sockets stay black and on top of the rain. No lime-block eyes. The panel, board, banner, pips, and every other page are untouched.

Renders (Vapor Grid, theme 22) are in `/workspace/out/fw-bigbot-1117/cyd21-hackbot-lime/` (not in this folder): the @3x still and a 20-frame @3x face-crop sheen GIF (100 ms, loops over the 4 s sheen period).

Gate (`gate_c21.txt`): OFF build vs base a87eb83: 0 px on all 12 pager stills (pages 0-5, mid + full, theme 22). ON build vs base: 0 px outside HACKbot's plate (x65-174, y88-197); other pages 0 px everywhere.
