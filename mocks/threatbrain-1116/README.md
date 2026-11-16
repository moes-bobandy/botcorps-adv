# THREAT BRAIN / NEURAL TRACKS — TERMINALbot neural-net mock (2026-11-16)

MOCK ONLY. Nothing here is built into firmware; base branch backup/cyd-fw-3locks-2026-11-16 @9bc3569 (source c1b5659) untouched.

- **A THREAT BRAIN**: each of the 10 device bots is a node; axons run through the dark gaps to TERMINALbot. Node halo + lines brighten with scan activity; each detection sends a pulse to the hub, which flashes on arrival.
- **B NEURAL TRACKS**: no new lines; the locked circuit traces become nerves. A glow hugs them (brighter near busy bots) and each detection sends a signal front climbing the traces from the bot to TERMINALbot.

Colours = selected CONTROLbot theme ink (`lv_theme_at`). Shown: NEON GRAPE (default), GOLD RUSH, SONAR.
Activity = scripted EXAMPLE feed (`src/tb_feed.h`), stamped EX.DATA on screen. Not radio data.

## How it is rendered
`src/sim_tb.cpp` = `tools/livescan/sim_live.cpp` @9bc3569 (real `src/live/lv_core.h` + locked clip blob) + `themeany` + `tb*` commands.
The net layer (`src/lv_threatbrain.h`) is composited onto a COPY of the firmware framebuffer and may only change a pixel that is
outside every bot board / the hub / pin rails / ticker / terminal box AND dark (background) in that exact frame.

Build (from repo root): `zig c++ -O2 -std=gnu++17 -I. -I<zlib include> -Imocks/threatbrain-1116/src -o sim_tb mocks/threatbrain-1116/src/sim_tb.cpp -lz`
Run: `./sim_tb mocks/threatbrain-1116/scripts/A_theme0.txt raw2` (etc.), then `python3 mocks/threatbrain-1116/tools/make_outputs.py raw2 <plain locked renders> out`.

## Proof (`out/diff_proof.json`)
Per variant x theme, 300 frames (9 s) + idle: harness base frame == plain locked render 0 px; changed px outside the net layer 0;
on bot/hub/rail/text rects 0; on lit (non-background) locked px 0. `out/proof_*_locked_mock_diff@3x.png`: locked | mock | diff (cyan = net px, red = violation).

## Example feed (hits in the 9 s clip; before-clip warm-up hits not counted)
DEAUTH 11, AIR TAG 7 (+2 before), FLOCK 6, SAMSUNG 3 (+1), HACK 2 (+1), META 2, SKIMMER 1, KEYFOB 1, DRONE 1, FLIPPER 0.
