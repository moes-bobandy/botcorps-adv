# towers3c / EB: E colours + B rack LEDs (Mobandy 7:50 PM ET)
All 18 towers: own-emblem-colour enamel (look E), no Matrix bodies. Each tower has a right-rail LED column inside a dark recessed well (#08090b).
- Top slot is the threat lamp: steady, in threat colour (HIGH red, MED amber, LOW green).
- Lit LEDs = round(example signal x (slots-1)). Lit LEDs flicker at 1 + 7 x signal Hz (on 60% of each cycle). Unlit slots are dark #1c2420.
- LED size: 4 px for close-up faces (fw>=40), 3 px for near idle faces (fw>=20), and 2 px for far faces. No column on faces under 8 px. Lit LEDs >=3 px get a specular dot.
- The file rows stop before the rail, and the emblem sits above the column.
Everything else is the v5f-locked code path (placement, drift, camera, emblems, open sequence, and folders). Built with real C++ sim (zig), MO_BODY=5 plus EB patch; see ../src/towers3c_body.py.
