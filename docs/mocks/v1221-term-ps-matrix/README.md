# MOCK 2026-11-25: TERMINALbot PowerShell + Matrix area-scan (LV_SC_TPS)

New TERMINALbot subscreen only. No firmware build, upload, or flash.
Type-out is built from our own TERMINALbot face-rain glyph law (`lv_tg_rain` / LV_F35) plus the existing `(lv_now/500)&1` cursor blink. The original Adv Matrix operator typewriter mock was not recovered (box data loss).

## Search (one pass, not found)
- `/workspace` phrase grep (typewriter / operator screen / type-out / text-reveal)
- `botcorps-adv` `git log --all` and local trees
- GitHub `backup/*` branches on origin, including `backup/adv-f1c08aa`, `backup/adv-90d5cc2-ref`, `backup/adv-land-e0e5ec3`, plus CYD backup branches
- Adv trees: advpaint, advpaint-ui-src, adv-ui-studio, adv-live, wt-f1c0 (f1c08aa is P0/hub/Gibson, not operator type-out), adv-90d5cc2-ref, adv-land-e0e5ec3
- `botcorps-backup` loose files (Flock/plume ADV sketches, HTML — not this mock)
- GitHub org `moes-bobandy`: 11 repos; `search_code` for typewriter/operator/matrix-terminal/type-out returned 0; `search_commits` only hit `gbcyd-meter` (Matrix-green build meter page), not an Adv operator terminal

Closest unrelated CYD hits kept out of this base: controlbot-colorui v5-matrix4 banners; morpheus-mix "Matrix HIGH towers"; TERMINALbot face rain.

## Entry / back
- TERMINALbot minterm **LIST** (SCAN / HELP / face / hunt cards unchanged)
- Sim / TTY: `psterm` or `tps`
- Back (banner BACK or strip B) returns to LV_SC_TERM
- Strip is the existing GIBSON pad pattern, on this screen only (A SCAN / B BACK / SPEED). Locked TERM keeps LV_TERM_STRIP 0.

## Look
- Locked LV_THC_* grey lip + gold terminal box around a dark blue-black PowerShell pane (`PS C:\BOTcorps>`)
- Live theme pin colours via `lv_thc_pin_*_px` (never hard-coded purple). Stills below are theme 0 GRAPE.
- Matrix green rain in the left/right gutters, cloned from the face-rain seed/trail law, slower (low power)
- Dim-backlight indicator in the banner; SAMPLE / EX.DATA labels
- Readout lines come from `lv_f[]` + `lv_hunt_match` over the 10 hunt classes. Sim canned radio is labeled SAMPLE.

## Gate
Tour `v1220_tour.txt` vs v1.2.21 head `d7b5545` sim dumps: 0 px on all 11 screens (see gate_px.txt).

## Branch
`mock/v1221-term-ps-matrix-2026-11-25` from `d7b55459` (build/v1.2.21-2026-11-25).
