# LOCKED: ui_home_tap_glow (home board tap glow, variant O)

- **Locked:** 2026-11-03 5:16 PM ET. Mobandy: "Yes lock it in" (about variant O, 9fede35).
- **Source:** `9fede35e29f7188e093aa1906492bf77080c8f43` on `backup/cyd-livescan-2026-10-30` (botcorps-adv), `docs/mocks/home-tap-pulse/`.
- **This is a mock only.** No firmware was built and nothing was flashed.

## What is locked
- **Breathing face glow (variant C's code, unchanged)** on all five tappable chips: MORPHEUSbot, CONTROLbot, GIBSONbot, SCANbot and TERMINALbot.
  - Formula: alpha = AMAX * ((7+1-d)/7)^2 * s, so the reach is 7 px at 1x.
  - Timing: s = 0.5 - 0.5cos(2*pi*n/42), a 1.4 s breath at 30 fps, all chips in sync.
  - Brightening: C's face midtone lift, 255*(x/255)^(1/(1+0.15 s)).
  - AMAX is 0.80 for MORPHEUS, CONTROL, GIBSON and SCAN. TERMINAL keeps C's original 0.95.
  - Each chip glows in its own face colour:

    | Chip | Glow colour |
    |---|---|
    | MORPHEUS | (222,232,252) |
    | CONTROL | (147,254,0) |
    | SCAN | (245,180,73) |
    | TERMINAL | (226,236,250) |
    | GIBSON | (247,242,230) |

  - The glow only touches each face and its face window. Chip art, labels, pins and borders are untouched.
- **MORPHEUS chip (J board):** locked MORPHEUS face, dark grey bezel (84,88,98), CONTROL-style body, MORPHEUS/bot label, TERMINAL-style pins. Its glow is lift-only on the shades rim, bridge and glint (pixels with RGB sum >= 330).
- **CONTROL chip:** TERMINAL-style pins.
- **GIBSON chip:**
  - The small pins are unchanged.
  - The cream face (1x x96..143, y56..103) is shaded with TERMINALbot's own face ramp, scaled from 39 px to 48 px, with the light from the top-left. Cream tones replace TERMINAL's tones step by step:

    | Step | TERMINAL tone | GIBSON cream tone |
    |---|---|---|
    | highlight | (255,254,255) | (255,254,250) |
    | light | (230,238,247) | (252,248,238) |
    | mid | (197,205,212) | (240,233,217) |
    | shadow | (139,153,165) | (208,197,178) |
    | deep | (105,119,131) | (176,165,147) |
    | edge | (40,54,66) | (120,111,98) |

  - His black shades (224 px) are pixel-identical.
  - His glow is clipped to the black pixels of the face window inside the grey (84,88,98) frame, x91..148, y52..109. That leaves 5 px on the left and right, 4 px at the top and 6 px at the bottom.
  - His locked rain is unchanged (LOCKED/ui_main_bot_rain, gaps3). Over 168 frames, O has 0 changes against N on the rain, black gaps, label, dim band, lime dashes, pins, border and window frame.

## Files
| File | What it is |
|---|---|
| `home_tap_faceglow_O_3x.mp4` | H.264, yuv420p, faststart, 720x960, 30 fps, 168 frames |
| `home_tap_faceglow_O_peak_3x.png` | peak still (frame 21), 3x |
| `home_tap_faceglow_O_trough_3x.png` | trough still (frame 0), 3x |
| `home_tap_faceglow_O_peak_1x.png` | peak, native 240x320 |
| `home_tap_faceglow_O_trough_1x.png` | trough, native 240x320 |
| `home_tap_faceglow_O_gibson_4x.png` | GIBSON next to the SCAN and TERMINAL faces |
| `tap_faceglow_O.py`, `tap_faceglow_O_zoom.py` | O generator and stills script (copies of the committed files) |
| `src/workspace/...` | every file O reads to regenerate, copied at their original absolute paths: the gaps3 rain chain, the morpheus tool modules, `g200.png`, `glossary_text.json`, the `tap_pulse.py` -> C and E..J -> N -> O scripts, and the `cyd-portrait-wt` `botcorps_rain.cpp`, `botcorps_ui.cpp`, `lv_font.h` and `lv_icons.h` |
| `regen_O.py` | self-contained regenerator |
| `frames_O_rgb_sha256.txt` | sha256 of the raw RGB of all 168 3x source frames |

## Regenerate and verify
Run `python3 regen_O.py [outdir]`. It does the following:
- Redirects every `/workspace/...` read and import to `./src/workspace/...`, and reports any read or module load that escapes to the live tree.
- Rebuilds all 168 frames and compares them with `frames_O_rgb_sha256.txt`.
- Re-encodes the MP4 with the same ffmpeg settings and prints its sha256 next to the locked MP4's.

Result at lock time:
- 168/168 frames match, 0 mismatched.
- 0 escaped reads and 0 modules loaded from the live tree.
- The re-encoded MP4 is byte-identical to the locked one: `d6d03986c859ddfb6ffe3eca493271dadc6bfe5117f38d448a2fa675f5118ebd`.
- Before locking, the live-tree regeneration matched the original `/tmp/htp_Os` source frames with 0 pixel mismatches.

## sha256 manifest (every file except LOCK.md and SHA256SUMS; `SHA256SUMS` also covers LOCK.md)
```
ffa1b1641bf7dead4aefa838f02419d6b9a37a10ab48ee1629c070a13a1a2eec  frames_O_rgb_sha256.txt
d6d03986c859ddfb6ffe3eca493271dadc6bfe5117f38d448a2fa675f5118ebd  home_tap_faceglow_O_3x.mp4
2de3f22d3555179ae57fbc3b58d1ba87db1c3525f88b1334cba3a43f30a569fa  home_tap_faceglow_O_gibson_4x.png
3f4031fb2d0c780cd4b0c898500662929ddfb369b2d927ed800755e182eec072  home_tap_faceglow_O_peak_1x.png
53c302a531989005f05fad0e64f082cc966921391e58ac8fa33fab262c7289b3  home_tap_faceglow_O_peak_3x.png
6ff9a8f833210cbe1373196fe3f2ea1f6cad395378157fc0d8fb2b4a19b023ad  home_tap_faceglow_O_trough_1x.png
d95ed0b2d429b76f115bac2dbd83cf505da9a96e33162a12db128c8cc43799e1  home_tap_faceglow_O_trough_3x.png
1715a3828d82f7f311a1af1971e8671802ac40e92db321db2c689cccf30ad6a1  regen_O.py
b21e7417cd5c1b37b61d5cba8cb3dbe2bdb0bacb98f450b0a222191802021cdb  src/workspace/cyd-portrait-wt/src/live/lv_font.h
c72cf0f344e1f6a368996d8476d0236029e30ea3c6b2f2887aca71f94c8282c6  src/workspace/cyd-portrait-wt/src/live/lv_icons.h
8f2e7d309ebf75ceb8ded0ebe92e94a078f6f59bbc99565870df68ce61200982  src/workspace/cyd-portrait-wt/src/rain/botcorps_rain.cpp
299abe1c1d0161d4f1f034c6e7542134bb0b8e49a6d7ca1848a25222c24c4d86  src/workspace/cyd-portrait-wt/src/ui/botcorps_ui.cpp
d96739b115a3f77b89c57e309ec58b20d1f1c1b3a36ec520d0658f5b55358546  src/workspace/out/home-tap-pulse/tap_faceglow_C.py
fe00fb4ff82ab8ec811e668fd417863a3da2b131942db1c2daf6b44e01083a70  src/workspace/out/home-tap-pulse/tap_faceglow_E.py
1e80de97d7f86f2f9e6398a4641069e121c484ba412d0ab2ca0ec8763bac363e  src/workspace/out/home-tap-pulse/tap_faceglow_F.py
f8bd3353de8d1aead0069252e0c70011cbbcd5b1acdb21c55370e2f455df4518  src/workspace/out/home-tap-pulse/tap_faceglow_G.py
df1d3a11deea79ea64782bb3d8ed1993ea74d2ae919043e116afb0d49a95025e  src/workspace/out/home-tap-pulse/tap_faceglow_H.py
a022b3b5096818b5780aa5885a50f5511c84ce44e01782026a9de90ba1fbdf6e  src/workspace/out/home-tap-pulse/tap_faceglow_I.py
beb8cc4dcb0e65c7d699d7f639e8e04f48a30c4f0719c0264dd3eb61703c3457  src/workspace/out/home-tap-pulse/tap_faceglow_J.py
50b02c86e240efcb829e12cb08b41fc671f606624d3cf8802c3c71002fb0b157  src/workspace/out/home-tap-pulse/tap_faceglow_N.py
bd135d653902ecdc8f5f0ccd2e9175ef207f79d530d7d10a549fad602e70b5c4  src/workspace/out/home-tap-pulse/tap_faceglow_O.py
2402a85d4f0eec33928efd50d3bd99d6731a9f2a05b0586e45e9a5c553367bd4  src/workspace/out/home-tap-pulse/tap_faceglow_O_zoom.py
d036c1a38a1fe10a1e2b15ff4492f8c8a163736cd45929db5117dd263c09bc26  src/workspace/out/home-tap-pulse/tap_pulse.py
185e4152131994207a43a0360045f321991dbc3864a95fa1aa26d53700670406  src/workspace/out/main-bot-rain/gaps.py
d2aa5f6038c0f4f682c7e717ccf2031f542b66ecfe17592850919ce9778e979b  src/workspace/out/main-bot-rain/gaps3.py
a3a52419575455d12957d0369b3a4a9c8794b71c381ec2cc347021e786c89cdd  src/workspace/out/main-bot-rain/mbr.py
1e2dad81af46aafa1a6d1f8a24b3cbd4603158f820f3bf354455c86f857c1ad0  src/workspace/out/main-bot-rain/moving.py
291f1438b597cfb571eec4841d03a2cd857bd866f577cb962d3d01b4a7ab3aa8  src/workspace/out/main-bot-rain/oneline.py
e8e0e2bfc28fc5e7302957a7299ad5258c123b579fbc8fc9d17b17f8ccdb3421  src/workspace/out/morpheus-glossary/tool/glossary.py
5cf0d79c03cfa06696aa0bc7eb77c4a0777d7340ef2fcc67e7b74e68ea3c3b11  src/workspace/out/morpheus-glossary/tool/glossary_text.json
0a212c8bc4514daa5e6994134908d2b1a14cc423c53bebb319c53e3a2f70e0d8  src/workspace/out/morpheus-glossary/tool/variant_big_morpheus.py
b3f191e57bd58123302d68ae0260c15fe3d9bd0e32629bc1c2a084a27dfc4a8d  src/workspace/out/morpheus-look-final/rain_glossary.py
afe2269025324570d8c35cc0ea3b91d280513c2e66355b46f6c2fce1368d9375  src/workspace/out/morpheus-rename/frames/g200.png
3afa2f031902c4377554f5265afc1f286e1b627463562ffdf36ec3d23eace115  src/workspace/out/morpheus-rename/tool/mlib.py
330572a576d911caaab2be05618ee04d86d4219eaa0c2c93d5a447faeb47ec86  src/workspace/out/morpheus-simple/tool/simple.py
bd135d653902ecdc8f5f0ccd2e9175ef207f79d530d7d10a549fad602e70b5c4  tap_faceglow_O.py
2402a85d4f0eec33928efd50d3bd99d6731a9f2a05b0586e45e9a5c553367bd4  tap_faceglow_O_zoom.py
```
