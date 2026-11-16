# CONTROLbot Color UI (mock, EX.DATA): SECURITYbot Color UI v2 + coil and pins in the theme pattern
Mobandy 2026-11-15 9:36 / 9:38 PM ET. Mock only (no firmware commit or flash), no rain, no locked file edited.

## What it is
- **Base:** SECURITYbot Color UI v2 (locked, commit 2b79d83, `/workspace/securitybot/mocks/security-color-themes-v2/`). The renderer `sim/render_security_color_themes_v2.py` and `sim/render_controlbot_bigchip_v9c.py` are byte-identical copies; inputs are read from the securitybot repo, read only. The v2 pipeline output equals the locked v2 @1x PNGs for all 11 themes (`pipeline_equals_locked_v2: true` in `diff_mask_proof.json`).
- **The only change:** two new reink roles (`sim/controlbot_colorui_reink.cpp`; roles 0-7 are the v2 cases verbatim):
  - role 8 = the gold coil strip and the gold pin pads / castellations / bottom ticks;
  - role 9 = the grey pin labels (938 px). The mask totals 4817 px: 3879 pad + 938 label.
  
  Each takes the theme's FULL pattern (`sec_theme_at` / CYD `lv_theme_at`), mapped across the strip the way `sec_face_map` maps it across the face:
  - coil = the whole 240x320 pattern squeezed into the coil;
  - left / right pin columns = the left / right halves of the pattern over its full height;
  - bottom pads = the centre-row profile.
  
  Pads keep the gold's own shading. Labels pass the v2 legibility lift (`sec_legible`). The meaningful pads (red 3V3, grey GND, purple used pins) and the white bold labels stay exactly as v2.
- **Themes (23):**
  - 01-11 are the SECURITYbot v2 themes.
  - 12-23 are the CYD CONTROLbot themes from `/workspace/cyd-portrait-wt/src/live/lv_core.h` lines 91-130 (commit 049ff59): `LV_THEME_NAME`, `LV_ACC`, `LV_ACCD`, `LV_HDR`, `lv_multi_theme`, `lv_theme_at`. They are copied verbatim into `sim/cyd_themes_lv_core_copy.h`.
  - The CYD themes run through the same v2 role pipeline:
    - face uses multi_ink with `lv_theme_at`;
    - frame uses hue;
    - panel sentinels use the v2 formulas with `lv_theme_at`; the solid CYD themes use their real `LV_HDR` / `LV_ACCD`.

## Proof (diff_mask_proof.json)
- coil+pin mask = 00_coil_pin_mask@1x.png
- For every theme, the pixels that differ from the same pipeline without roles 8/9 all fall inside the mask (changed_outside_mask = 0).
- For 01-11, the pixels that differ from the locked v2 PNG all fall inside the mask (changed_outside_mask_vs_locked_v2 = 0).
- The grey board (v2 role 0 outside the mask) is identical across all 23 themes.

## Open question
- The header still reads **"SECURITYbot EX.DATA"** (v2 as is). Should CONTROLbot's header be renamed? Not guessed.

## Honest flaws
- **CYD themes on the SECURITYbot layout are my mapping, not a locked design:** CYD inks run through the v2 role / sentinel pipeline. Real LV_ACC / LV_ACCD / LV_HDR / lv_theme_at colours, but where the CYD UI would put each ink on this panel is inferred.
- **Pin mapping is a choice:** e.g. SIREN puts all left pins red and all right pins blue; radial IR / NV patterns run dark at the pin ends.
- **IR / NVG cold ends are dim:** the coil ends and the top/bottom pins go dark violet/green on the near-black board (pads keep the gold's shading, so they are darker than the face).
- **Fine patterns barely show:** at 1x, small 6 px pads show only a slice of fine patterns (NVG scanlines, PERIM grid, GLITCH bands); they read as flecks.
- **Bottom-left gold ring** (corner mark) is inside the side mask, so it is themed too.
- **mp4 is a scripted cycle:** hard cuts every 20 frames (0.67 s) through all 23 themes; there is no tap animation.
