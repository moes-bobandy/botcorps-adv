# Big-bot Color UI v6 mocks — v3.1 face-size lock + names off (MOCK ONLY)

Mobandy 2026-11-16 5:45–5:46 PM ET (America/New_York): GIBSONbot’s face was slightly smaller than CONTROLbot’s, and BOTpheus’s was a lot smaller (the dark square sat inside the box with a margin). “Same size as control.” Also: “Take the names off for right now… Like control.”

**Mock only. No firmware commits to the build branch, nothing flashed, nothing copied to an SD card.**

v3 (smooth instrument panels, undersized BOTpheus/GIBSON faces, on-screen names) remains at `../v3/` on this same branch as commit `7516774`. This folder is v3.1 only. v3 stills were the starting point, so bottom panels and everything outside the face slot and the under-face name pad are byte-identical to v3.

## What was measured, and what was fixed

CONTROLbot’s face square, measured on `stills/control@1x.png` (identical to locked v6 NEON GRAPE):

- `THEME_FACE` inclusive **(65, 88)–(174, 197), 110×110**
- Every edge pixel of that slot is face art (0 letterbox). The darker purple strip on the bottom edge is part of the face, not a margin.

v3 measurements, same slot:

| bot | v3 face-art bounds in the slot | size | note |
|-----|--------------------------------|------|------|
| CONTROLbot | (65, 88)–(174, 197) | 110×110 | fills the box |
| HACKbot | (65, 88)–(174, 197) | 110×110 | already filled (black pixels are inset eyes) |
| TERMINALbot | (65, 88)–(174, 197) | 110×110 | already filled (black pixels are inset eyes) |
| GIBSONbot | (65, 88)–(173, 196) | 109×109 | 1 px black letterbox on the right and bottom (`nn_blit_fit` integer remainder) |
| BOTpheus | (75, 96)–(164, 185) | 90×90 | ~10 px black margin on all sides (the 58×58 source, including empty screen chrome, was fitted into 110, so the 48×48 face did not fill the box) |

v3.1: every bot’s face square is **(65, 88)–(174, 197), 110×110**, 0 letterbox on any edge, matching CONTROLbot exactly.

- **BOTpheus:** the 48×48 locked shades-face content (glasses, dark sheen, bevel) is nearest-neighbour filled into the 110×110 slot in one step.
- **GIBSONbot:** the same 49×49 cream+shades crop as v3 is nearest-neighbour filled into 110×110 (no remainder letterbox).
- **HACKbot / TERMINALbot:** faces already filled the slot; they are copied from v3 unchanged (gold sheen, silver, no `>_`).
- **CONTROLbot:** byte-identical to v3 and to locked NEON GRAPE.

## Names off, like CONTROLbot

On-screen name plates (`BOTpheus`, `GIBSONbot`, `HACKbot`, `TERMINALbot`) are removed. The under-face pad `LV_CB_BOT` y = 198–210, x = 54–185 is restored from CONTROLbot (pure black on the locked still). CONTROLbot itself had no label and is unchanged. Contact-sheet captions above each still are not on-screen UI.

## Pixel diff vs v3

Allowed change regions: `THEME_FACE` (65, 88)–(174, 197) and the under-face name pad (54, 198)–(185, 210). 0 px outside those rects. 0 px in the bottom panel. 0 px in y = 211–219.

| bot | changed px vs v3 | diff bbox (x0,y0,x1,y1) | what changed |
|-----|------------------|-------------------------|--------------|
| BOTpheus | 6530 | (65, 88, 174, 207) | face filled to 110×110 + name removed |
| GIBSONbot | 1193 | (65, 88, 174, 206) | face filled to 110×110 + name removed |
| CONTROLbot | 0 | none | identical |
| HACKbot | 100 | (99, 200, 139, 206) | name removed only |
| TERMINALbot | 163 | (87, 200, 151, 206) | name removed only |

`@3x` (720×960) is a pixel-exact 3× nearest-neighbour of `@1x`. CONTROLbot `@1x` sha256 matches the locked grape still:
`48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779`

TERMINALbot CIRCUIT / ICE / GOLD RUSH stills: faces already filled; only the name plate was removed (163 px, same bbox as the default-theme name). 0 px outside the pad.

## sha256

Default theme (NEON GRAPE):

```
b581422414e0b331e9af3f0e566b9d2c3f4c219db7f2bf25ad943cb87e50841d  stills/botpheus@1x.png
0c72a3940ef7157c42bc613a75b99feef120eba7583ff8604287f34d2bd7f255  stills/botpheus@3x.png
48096cc6ab73a35a674de412b42c6b853a244830c362b888b4b980cafa0a8779  stills/control@1x.png
c2ca9d57d814783c4f003dced9e9c36d33262a92487dad6d8887d2921c94fc1a  stills/control@3x.png
b3604e5ca6c3876eac527d3a3d53ad11c68c60db9407d48422226d2ecc36fc85  stills/gibson@1x.png
0e652004b435d0dbbe205ca803014ed91ff9d247c733b92eed4cec54803ced24  stills/gibson@3x.png
b5c459d8c56d77c741ee376a89bd70ccfe97c0f58a3f75e4b7e70a9fba53dc29  stills/hackbot@1x.png
179ef1a33c9ee9f9d4191c11691915c9eeaa472cf01b13f29f20c26a878294d7  stills/hackbot@3x.png
8556eff4744c2da44f2196753603d89fb9edb69a4adb10cae9112afaa97bf82c  stills/terminal@1x.png
3cb4c37908fffca9f970dd5645fa80440516524f7ed8df88957bcd31a322c2bc  stills/terminal@3x.png
762d37c28ed794a80e2e98aa9f2a616c157f1b99ccbceb3c0187e61de9a40545  contact_sheet_5bots@3x.png
c706235f93a6df98e69589f0abfa8e844d3641faa97289f1dd41370eb504440b  terminal_3themes@3x.png
```

Themes:

```
0409560b930c6f91b26815b337acb215f7c6208b9de2af9b05ca394595f1a76c  themes/terminal_01_circuit@1x.png
12481f99e8cd2ffe57227af926a248de6fb4afaa92ef2b08bc42462e28b8988a  themes/terminal_01_circuit@3x.png
d5d6e50d3a47096f9d91e7a03a98d071ac4b836566403d0e8091e72e5ffd2508  themes/terminal_15_ice@1x.png
3c8f40121f0a436617b104c85723613844e49df613fa4265e593137b5bfe299b  themes/terminal_15_ice@3x.png
27bcd508730690874a6cf511609b13464857005871195768e5774da6cb8ba4a4  themes/terminal_18_gold_rush@1x.png
746634c16155210354aea8ac41a840037965b39015f22a7607679c1e7829a3cc  themes/terminal_18_gold_rush@3x.png
```

## Caveats

- Mock only. v3 instrument panels and EX.DATA snapshots are unchanged.
- Face fill is nearest-neighbour from the locked BOTpheus 48×48 content and the official GIBSONbot 49×49 crop, so the art is the same, just sized to the locked slot.
