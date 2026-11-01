# MORPHEUSbot simple crew-style chip face + Morpheus shades (MOCK ONLY)

Mock only: no firmware change, no bins, no flash, no merge, no lock changes. Locked clips untouched.
This replaces the rejected M1/M2/M3 human-face looks (docs/mocks/morpheus-look).

The face is a crew-style chip-bot panel, like TERMINALbot and SCANbot. It has a light top and left edge, a dark right and bottom edge, a drop shadow and diagonal shine bands.
The panel is bronze and wears small round solid-black pince-nez lenses with thin frames, a bridge and a white glint.
There are no human features.

Pins and wave: the "wave" is the square-wave meander in the chip header.
On the boot screen, the pins, the wave and the inner pads change from fixed blue to the active UI theme colour, keeping their shading. The renders show purple, which is the boot board's theme, and green.
On the home board they already use the theme colour (renders: green f200, orange f380).

Variants:
- **S1 (primary):** the current BOOTbot chip with only the face swapped, plus themed pins and wave.
- **S2:** S1 with a subtle dark graphite chip body.
- **S3:** S2 with a Matrix-green accent on the lens frames and glint.

The labels (MORPHEUSbot / BOOTING at boot, MORPHEUS / bot at home) come from the approved rename mock.
Everything outside the chip box is pixel-identical; `tool/render.py` asserts this.

Adopting a variant needs a re-render of the locked clips, a re-lock approved by Mobandy, and a lock_blob rebuild.
lv_core's runtime BOOTbot tint would also need review.

## S2b (2026-11-01): S2 with a black face

S2b keeps S2's dark graphite body and theme-coloured pins and wave.
The face panel is back to black, with the crew shading kept in near-black tones: a lighter top and left edge, a dark right and bottom edge, a drop shadow and diagonal shine stripes.
The shades are round black lenses with a silver rim, bridge and white glint at boot size. At home size the rim and bridge are a dimmer grey, because a bright rim made the lenses look hollow there.
Files are in `S2b/`. Rebuild with `python3 tool/render_s2b.py`.

## S2c (2026-11-01): S2b with brighter shades on the home board

S2c is S2b with the shades brightened at home-board size only. The boot-size S2b face is unchanged.
The small lenses get a shaded rim, lit grey at the top-left and dimmer at the bottom-right, plus a brighter bridge.
The lens centres stay solid black, each with a white glint pixel, so they read as round dark discs rather than hollow rings.
Files are in `S2c/`. Rebuild with `python3 tool/render_s2c.py`.
