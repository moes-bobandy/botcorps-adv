# TERMINALbot face mocks (not in firmware)

Mobandy 1:47–1:49 PM ET: pick a TERMINALbot face. These are mock-first stills of the locked big-bot TERMINALbot page from the real C++ `sim_live` renderer. Only the face box (65,88)–(174,197) is overpainted. Nothing else on the page changes. The face is not in the v1.2.4 firmware.

Eyes stay in the locked 22×22 boxes at face-local (22,22)–(43,43) and (66,22)–(87,43). No `>_`, no name.

## A — 4×4 color chips

`terminalbot_face_A_colorchips@3x.png`

16 equal 27×27 squares (plus a 1 px black frame so they fill the locked 110×110 box exactly), each a different color, with the two black eyes on top.

Vs the locked page at the same tick: 11,132 px differ, all inside the face box; 0 px outside.

## B — Matrix rain

`terminalbot_face_B_matrix@3x.png`
`terminalbot_face_B_matrix_rain@3x.gif` (24 frames, 80 ms, loop)

Black square, green Matrix rain (5×7 GLCD glyphs, bright head, fading trail) falling over it, green Matrix-style eyes in the same boxes (green bezel + slit).

The GIF is the rain moving. A still vs the locked page, after masking the ticker (one 30 ms tick between dumps), differs only inside the face box.

## How to regenerate

From the repo root, with the host sim (the `termface` command is sim-only and is not compiled into firmware):

    sim_live <script> <outdir>

`termface a` and `termface b [PHASE]` overpaint the already-rendered page.
