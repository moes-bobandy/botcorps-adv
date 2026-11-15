# towers3b: D (uniform), E (varied), F (lantern), mock, Mobandy 7:12 / 7:13 PM ET

**Mock only.** The v5f files and the earlier towers3 A/B/C are untouched. These looks were built from a copy of the v5f sources in /tmp/t3b by `src/towers3b_body.py`.

**What changed:** only the 13 non-Matrix tower bodies (front face, sides, cap, edges, emblem plate, file-row ink).

**What did not change:** the Matrix five (black glass with green code rain), emblems, folders, open sequence, depth, drift, floor and camera all run the v5f code.

**Pixel diff vs v5f** (`pixel_diff_vs_v5f.txt`):
- In the idle still, 0 pixels changed outside the tower boxes.
- In the close-up, the right side shows about 320-900 changed pixels, between x 88 and x 172. I believe they are the new bodies of the towers behind the open screen, but I did not confirm that pixel by pixel.

| look | type | body | what each part means |
|---|---|---|---|
| D polished gold | UNIFORM: all 13 identical | SCANbot-style gold, shaded lit left -> mid -> shadow right and darker toward the base. Darker gold sides, bright cap highlight, white lit edge, dark shadow edge. | The emblem sits on a dark engraved plate; file rows are in dark ink. |
| E own-colour enamel | VARIED: each tower different | Enamelled in its own device colour (the emblem colour), with the same lit/mid/shadow shading, darker sides, light cap. | The colour is the device. The lit rim's brightness is the device's example signal. Emblem on a dark plate. Ink is dark or white depending on how light the colour is. |
| F dojo lantern | Free pick | White porcelain lit from inside, with a red-lacquer cap, base and sides, shaded. | The warm amber core glow's strength is the device's example signal. |

Files per look: `idle_3x.png` (the same idle time as v5f), `closeup_3x.png` (FLOCK opened, 3.15 s), `idle_then_open_3x.mp4`.
