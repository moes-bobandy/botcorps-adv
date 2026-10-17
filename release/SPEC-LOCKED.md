# BOTcorps Adv: SPEC-LOCKED (v1016n)

This replaces the old rules checklist (release/RULES-CHECKLIST.md R1, "Gibson chip on hub", is **superseded**).
Run `tools/gate.sh` to check the spec. It renders every screen with the real firmware paint code and the real
rain engine (host build, `-DBOTCORPS_HOST_RENDER`), then prints PASS/FAIL per item per screen.
The gate is green only when everything passes. Item ids below are the ids the gate prints.
Anything the gate can't check is marked **[visual]** and is checked by hand on the side-by-sides (release/sxs/).

Where the spec doesn't say otherwise, the mock (BOTcorps-Adv-master-v1016-1604.html) sets the geometry, palette and sprites.
Locked rules beat the mock where they conflict. Known cases: the scan and settings boards are matte black (the mock has them
teal or accent-tinted), and the mock's game is disabled (GAME_ENABLED=false). In the firmware the game screens are the GCHAT boards.

## Rules for every screen
| id | rule | how it's tested |
|---|---|---|
| G1 | No overlapping parts: no chip/button/tile/part/crown/core/prompt overlaps another, no text overlaps other text, no text crosses a solid part | logged bounding boxes, with overlay screens excluding what they cover |
| G2 | Every sprite sits inside its chip, tile or face | logged sprite boxes against their parent. Scan-board scout walkers are exempt by design (they walk the traces) |
| G3 | No clipped text | the text box sits inside the active clip rect |
| G4 | Nothing goes past the board or screen edge | boxes inside their parent board or 320x240 |
| G5 | The Matrix bit-rain layer is drawn and visible (mostly 0/1, bright lead glyph, fading trail, bots and devices inside the strands) | rain drawn, and the rain-on render differs from the rain-off render in at least 100 px. N/A on the full-screen found-alert overlay |
| G6 | No instruction text (keyboard only) | regex over every drawn string |
| G7 | No regression on screens that aren't meant to change | ≤40 px diff vs release/gate/baseline (approved v1016i). The list is release/gate/frozen.txt; masks cover only an intended change |
| G8 | The crown is the only back button and it's on screen; Esc also goes back (F1) | crown box logged. N/A for intro, boot and the overlay |
| ID badges stay | **[visual]** + G7 on the frozen screens |
| Tab hard edge stays | G7 on terminal-tab (only the log-text area is masked) + **[visual]** |

## Intro
- G1–G6. The ID board's Gibson stays the approved dome figure. G7 checks the intro is unchanged.

## Hub / Gibson main
| id | rule |
|---|---|
| H1 | The centre is the animated moving circles/rings core (HUBCORE), with **no** chip in the centre |
| H2 | The nav buttons are lit with the cyan/accent face and rim, not dark |
| H3 | Rain on the hub carries enough bots and devices: ≥150 coloured rain px |
| H4 | 12 bot tiles |
| | Checked in the UI colour (hub) and in amber (hub-amber) |

## Only Gibson has main screens (HIT LIST #1, Oct 15; ac9c53b)
| id | rule |
|---|---|
| N0 | Only Gibson has main screens. Other bots appear only on the scan screen, as square chip blocks. There are no per-bot home screens (no home-wifi / home-ble / home-bot) and no bot radar. The firmware screen table has exactly 11 screens: rain, gibson, terminal, wifiradar, wifilist, btradar, btlist, scan, control, diag, gchat |
| N1 | `go`/`load wifi` opens wifibot's scan screen (the live Wi-Fi scan box). `go`/`load ble` opens bluetoothbot's scan screen (the live BLE scan box). `go`/`load bots` opens the terminal's 12 ESP32 chip tiles. Esc from each of them steps back terminal → hub → boot and never lands on a removed screen. `go diag` is the control that proves the typed command really runs |
| N2 | Terminal Select on any of the 12 chip tiles opens THAT bot's scan screen (Wi-Fi and BLE: their live scan box; the others: their matte-black scan board with the square chip). Esc goes back to the terminal |

## Rain (all screens)
- Bits stay the majority (about 56% of trail cells are 0/1, plus 4% code). Bots (22%) and devices (18%, neon colours) also ride inside the trails, not only as the lead glyph.

## Scan board, one per bot type (scanboard-<bot>, Gibson included)
| id | rule |
|---|---|
| C1 | A square chip_block (w == h) |
| C2 | The chip's long side is at least 60% of its slot |
| C3 | Gibson (scanboard-gibson and the game boards): Gibson is drawn as the square block figure filling the chip, with black inside, a black border and steel pins |
| B1 | The board is permanent matte black (dominant luma ≤24) and doesn't follow the UI colour |
| | Other bot chips are square in their fixed bot colour **[visual]** |

## Wi-Fi / BLE scan (scan-wifi, scan-ble, sbox-*)
| id | rule |
|---|---|
| S1 | Device icon tiles replace the text list |
| S2 | The index is a small terminal prompt at the top left of the mini screen |
| S3 | Hunt stays inside the mini screen |
| B1 | Board is matte black |

## Terminal
| id | rule |
|---|---|
| T1 | Log text is pale cyan and "[ ok ]" is green (terminal-typed pixel classes) |
| | terminal and terminal-tab are frozen (G7) |

## Settings / control panel (settings-c0..c7, settings-amber)
| id | rule |
|---|---|
| P1 | 8 parts (the 8 settings) |
| P2 | Pin labels sit on the top rail above the chips |
| P3 | JUMP, CP and PWR are small parts (not full chips) |
| P4 | The selection outline is cyan/accent |
| P5 | The selected trace is cyan/accent |
| B1 | Board is permanent matte black (ESP32 shield look: white silkscreen, gold header pins) |
| F2 | Each setting can be edited |
| F3 | NVS writes happen. Tested on settings 3 and 4; in the firmware only rain mode is persisted, which is pre-existing behaviour |
| | Settings keeps the UI colour selection and the bot colour/shape settings. No MASHUP overflow (G2/G4) |

## Game screens (game-board, game-menu, game-jackin = GCHAT)
- C1/C3 square Gibson block, G1–G8.

## Diag
- G1–G8.
