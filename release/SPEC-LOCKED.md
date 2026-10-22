# BOTcorps Adv: SPEC-LOCKED (v1017a)

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
| H2 | The nav buttons are lit with the theme accent face and rim, not dark (v1019: a rim in the theme accent counts as lit) |
| H3 | Rain on the hub carries enough bots and devices: ≥150 coloured rain px |
| H4 | 12 bot tiles |
| | Checked in the default theme (hub, NEON GRAPE) and in theme 1 (hub-amber, HOT PINK; render name kept) |

## Only Gibson has main screens (HIT LIST #1, Oct 15; ac9c53b)
| id | rule |
|---|---|
| N0 | Only Gibson has main screens. Other bots appear only on the scan screen, as square chip blocks. There are no per-bot home screens (no home-wifi / home-ble / home-bot) and no bot radar. The firmware screen table has exactly 11 screens: rain, gibson, terminal, wifiradar, wifilist, btradar, btlist, scan, control, diag, gchat |
| N1 | `go`/`load wifi` opens wifibot's scan screen (the live Wi-Fi scan box). `go`/`load ble` opens bluetoothbot's scan screen (the live BLE scan box). `go`/`load bots` opens the terminal's 12 ESP32 chip tiles. Esc from each of them steps back terminal → hub → boot and never lands on a removed screen. `go diag` is the control that proves the typed command really runs |
| N2 | Terminal Select on any of the 12 chip tiles opens THAT bot's scan screen (Wi-Fi and BLE: their live scan box; the others: their matte-black scan board with the square chip). Esc goes back to the terminal |

## Rain (all screens)
- Bits stay the majority (about 56% of trail cells are 0/1, plus 4% code). Bots (22%) and devices (18%, neon colours) also ride inside the trails, not only as the lead glyph.
- v1017c (Dirt, bug #4): the rain carries ONLY our glyphs: 0/1 bits (~60%), the bots in their fixed colours (22%) and our ten device icons (ap, phone, laptop, tv, speaker, tracker, watch, camera, car, unknown; the mock RAIN_DEV art on the 8x8 cell, RAIN_DEV_NEON inks, 18%). No code symbols and no ASCII stand-ins. On Gibson's screens (hub, Gibson's scan board) the mixed-in bots are ivory Gibsons with black shades and the devices take the RAIN_DEV_GIB inks. Matrix style is unchanged: falling strands, white lead glyph, fading trail.
- v1017c (bug #6): the Terminal's rain has no bot emblems (bits + devices only), and its prompt panel has no bot emblem.

## Scan board, one per bot type (scanboard-<bot>, Gibson included)
| id | rule |
|---|---|
| C1 | A square chip_block (w == h) |
| C2 | The chip's long side is at least 60% of its slot |
| C3 | v1019 (Dirt LOCKED): every Gibson chip except the hub core and the centre radar die is the standard shape: ivory square face (S-18: 38 px face in the 56 px chip, ~3 px black window, scaled for smaller chips), lit top row, dark chin, square steel eyes raised to ~22% down the face, slim Matrix shades (style 1) over the eye row, slight black border, GOLD pins. The intro ID board keeps its approved dome figure (frozen.txt) |
| C4 | v1019: every other bot chip uses the same chip look and proportions in the bot's own fixed colour (never the theme), SILVER pins, "Morpheus A" glasses (round gold-rim pince-nez, mirror-steel lenses, thin gold bridge, no temple arms) **[visual]** |
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
| T2 | v1019y LOCKED (Mobandy, dce1c87): every unselected board tile is the detailed chip — dark well `#0A0C0E`, bright bot-colour chip glyph, IC pin comb, side via-holes, faint traces, corner marks, thin bot-colour border. The selected tile is that same art with every pixel inverted (full detail kept). A white dashed ring and a pale-magenta outline sit on top of the invert. Not a flat solid block |
| | Terminal rain is the dce1c87 canon: device-only per v1019o, faint (scale 1/4) behind the tiles, solid prompt plate with no rain. The half-strength log-area bump is not in this lock |
| | Terminal / terminal-tab relock is the HD+invert tile look only. G7 baselines and the terminal / terminal-tab / terminal-typed L1 hashes differ from the pre-HD lock by those tiles, not by a rain change |
| | terminal and terminal-tab stay frozen (G7) |
| C1 | v1017c (bug #3, hub model): arrows always move focus. Down from the bottom board row stops on the prompt (terminal-prompt), one more Down on the commands (≡) button (terminal-cmdbtn); Left/Right from either go back to the boards; typing focuses the prompt |
| C2 | Enter on it opens the commands menu, the 6x2 chip palette (terminal-cmdmenu); Enter on an empty focused prompt does nothing |
| C3 | Esc closes the menu back onto the button; Up walks button → prompt → boards |
| K1 | v1017c (bug #2): the prompt shows exactly what was typed: the real read_key on a scripted key matrix (rollover typing, a release + press in one scan, Backspace while a letter is held) types "scan wifi" exactly. Keys are edge-detected presses; a release never produces a key |
| K2 | One arrow chord (Fn + . released Fn-first) moves focus exactly one stop; plain ; . , / are arrows (never typed) in the Terminal even with text in the prompt |
| | v1017c: the prompt line has no inline ghost completion (it sat under the caret and read as a stray letter); completions stay in the candidate list |
| C4 | A command picked in the menu runs |
| | The prompt line's partial redraw uses the full painter clipped to the line, so it matches a full paint pixel for pixel (S1 terminal-typing) |

## Settings / control panel (settings-c0..c7, settings-amber)
| id | rule |
|---|---|
| P1 | 8 parts (the 8 settings) |
| P2 | Pin labels sit on the top rail above the chips |
| P3 | JUMP, CP and PWR are small parts (not full chips) |
| P4 | The selection outline is the theme selection colour (v1019 themes: NEON GRAPE, HOT PINK, DUOTONE, SYNTHWAVE, ULTRAVIOLET; Ultraviolet selects in pink, Duotone/Synthwave draw two-tone/gradient outlines) |
| P5 | The selected trace is the theme accent / theme wire |
| B1 | Board is permanent matte black (ESP32 shield look: white silkscreen, gold header pins) |
| R1 | v1017a: every control (8 settings + TERM + crown) is reachable by the cursor |
| R2 | Left/Right walk one ring through all 10 stops (incl. SCREEN SAVER U4 and the PWR LED); Up/Down go to the nearest stop above/below |
| R3 | Arrows only move focus; no arrow changes a setting |
| R4 | Arrows never leave the control panel (crown or Esc is back) |
| F2 | Enter cycles the focused setting forward through all its values (full cycle checked per setting) |
| F3 | NVS writes happen on Enter. Tested on settings 3 and 4; in the firmware only rain mode is persisted, which is pre-existing behaviour |
| | Settings keeps the UI colour selection and the bot colour/shape settings. No MASHUP overflow (G2/G4) |
| L1 | Layout and look are locked: settings-c0..c7 and settings-amber renders are byte-identical to 675c87d (release/gate/locked-675c87d.sha256, which also locks the terminal, scan, sbox, game and hub renders; v1017b re-locked only the terminal / terminal-typed / terminal-tab hashes (square chip-form tiles) and hub / hub-amber (strong selected-button halo + command-menu stop); v1017c re-locked every rain-on render (rain glyph set #4, host rain fidelity) with all their .norain twins unchanged, plus terminal.norain / terminal-typed.norain (bugs #2/#3/#6)); v1019 re-locked the theme (every settings / hub / terminal / scan / sbox / game render), Gibson/bot chip faces, Terminal prompt row + live rain ghost, and the Gibson ticker; a host-only legacy-accent build (HOST_EXTRA=-DBOTCORPS_LEGACY_ACCENT) shows the non-theme diffs are confined to chips/faces, the Terminal tiles + prompt row and the Gibson ticker) |

## Header rails (approved as rendered, v1019zj2)

Mobandy's zi/zj2 renders are the lock for the header-rail pins. This is not a B.4 item and it is not a rebaseline of the 675c87d hashes.

| surface | header-rail pads |
|---|---|
| Wi-Fi / BLE scan and the scan box (sbox) | bot colour |
| Non-Gibson scanboard | silver, Gibson's pin geometry |
| Gibson | gold, unchanged |

## v1019zl rebaseline (2026-10-20)

Mobandy approved a bounded rebaseline of the 63 LOCK_ONLY rows from tip `a331fce` (v1019zk). The matching L1 hashes in `release/gate/locked-675c87d.sha256` and the 16 G7 no-rain baselines now store those v1019zk renders. No other lock row was changed. The header-rail table above is unchanged.

## Game screens (game-board, game-menu, game-jackin = GCHAT)
- C1/C3 square Gibson block, G1–G8.

## Defend mini-games (v1019j, Dirt LOCKED: one game per device category; defend-raincatch / -beaconsort / -firewall / -maze)
Every Defend path (enter_defend: hub `defend [bot]`, Terminal `defend <bot>`, scan box `defend`; game chat `defend` /
`load game`) opens the Defend board AND starts the active bot's category game. Wi-Fi (wifi, hack) Rain Catch; BLE (ble,
airtag, meta, samsungtag) Beacon Sort; IoT (flock, keyfob, flipper) Firewall Wall; Rogue (deauth, drone, skimmer, Gibson)
Gibson Maze. Keys: ; . , / arrows, Enter action (launch / replay), Space second action (launch on the Firewall, pause
elsewhere), backtick quits to the Defend board (a second Esc goes back as before). EXT = play field (same top bar, matte-black
board, static rain around it); INT = game name, bot, score / lives / timer as plain text. Plain `jackin` (game-jackin) and
the board renders (game-board / game-menu) are unchanged.
| id | rule |
|---|---|
| DG1 | The game board is matte black (dominant luma <=24) |
| DG2 | v1019l (Dirt concept: games = software side): the active bot is its one ROUND sprite on the board, as in the per-bot hunt game (own shape + fixed colour; Gibson round ivory on his maze); no chip on the board |
| DG3 | The game's pieces are on the board, reused art only (device icons, 0/1 rain bits, jammer = deauth icon) |
| DG | Win and lose rules on the real step code: Rain Catch 20 caught = win, 3 missed icons or 3 bits = lose; Beacon Sort 25 = win, 3 mistakes = lose; Firewall Wall all 32 blocks = win, 3 packets = lose; Gibson Maze all pellets = win, caught 3x = lose |
| DGR | Each Defend path launches its category's game; backtick quits to the Defend board; Esc again goes back |
| | S1 (flicker sim), S3 (band slices) and L1 / G7 (byte lock + baseline) cover the four new renders |

## Diag
- G1–G8.

## No flicker (band simulator, v1017a)
The host harness emulates botcorps_band.cpp: one 120-row slice resident in the sprite; a patch in the other
slice recomposes it first; only changed rows are pushed. The real draw loop (botcorps_ui_draw) runs for 6 s
per screen, and every push is checked against a full paint at the same millis().
| id | rule |
|---|---|
| S1 | No push shows pixels that differ from a full paint (no flash): scan-gibson, scan-flock, scan-airtag, scan-wifi, scan-ble, game-board, game-jackin, terminal-typing, gibson-game, scan-sbox-hunt/info/gloss, control-nav, hub-to-control, control-to-hub |
| S2 | Scan screens: the idle animation never does a full-screen present (patches only) |
| S3 | v1017c: every gate screen painted the device way (two 120-row slices, oy 0/120, clipped to the 320x120 sprite) composes to exactly the one-shot frame, rain on and off |
| S4 | v1017c (bug #1): scan screens' idle ticks draw in place (dirty rect only, botcorps_band_patch_rect): zero slice recomposes in 6 s |
| | **[device]** The simulator proves the logic. Panel timing, SPI tearing and what you actually see can only be checked on the Adv

## Static RAM (ARCH freeze)
| id | rule |
|---|---|
| R0 | Static RAM is `.dram0.data` + `.dram0.bss` (PlatformIO "RAM: used N bytes"). `tools/gate/ram.py` PASSes at or under the cap in `release/gate/ram-caps.txt` and FAILs above it. Caps: cardputer-adv 71,264 B; cardputer-adv-dual 71,896 B; cardputer-adv-dual-launcher 71,896 B; dual with `BOTCORPS_LIVE_SCAN=1` 71,904 B. An elf older than `src/` or `platformio.ini` is FAIL. Growth has to be paid back with savings. The livescan env is a measurement build, not a flashed product (ARCH freeze, GIBSON-ratified 2026-10-19) |

## Scan-chip button (FINAL-v2 lock, 2026-10-22)

This is the current EXT chip-button lock. It is implemented on `9cc4c191b4ca085ee2ffe249573629f2b979e67a` (parent `d1773002674d4a2e64be8e05907ae539b967260d`).

| Source | sha256 |
|---|---|
| LOCK.md (FINAL-v2 look lock) | `bfe9ff935ea17693c06aeb6dcf135a8fe005e6327b3b08c5543f84ff071687a3` |
| ARCH-SCAN-CHIP-BUTTON-v004 (behaviour, gates, holds) | `b4d6925ceaf2dfe30846eb30d47ddcfa237a586728ade714b60ca794e8448704` |

Where an older paragraph in this file disagrees, this section wins. Superseded: the Terminal "12 ESP32 chip tiles" wording in N1 (the grid is 12 slots, KING then the bots, no KEYFOB tile); N2 Select indexes those slots (slot 0 enters Gibson's game); the dual-device chip centres at y=108; the grey strip in `paint_sbox`. HOLD-A1 (L1 `scanboard-gibson` lines 39–40 of `release/gate/locked-675c87d.sha256`) is not re-locked. HOLD-DIFF (0 px against the 17 shipped FINAL-v2 1× PNGs) stays held: the cut may land, merge stays red until that diff is 0.

| id | rule |
|---|---|
| CHIP | One `chip_button`. 54×54 at (34,67) on WIFIBOT, BLUETOOTHBOT, Gibson SCAN, AIRTAG (scan and Defend), and Gibson's game. Pins come only from `chip_shell` (`bot >= 12` gold, otherwise silver). Idle is a 1 px steel bevel. Press drops the face 1 px, then a clockwise rim runs 16 steps × 178 ms = 2848 ms. Scan bots commit results and return to idle. AIRTAG and Gibson's game start Defend. Esc mid-ring aborts with no partial results and no Defend start |
| KING | `s_term` is a slot 0..11. The table is `{12,0,1,2,3,4,5,6,8,9,10,11}`. Slot 0 is the KING tile (ivory rim, gold comb, label GIBSON). KEYFOB (bot 7) has no tile. Enter on slot 0 is `enter_defend(12)`. Left/right move between slots |
| KEYFOB | `go keyfob` and `load keyfob` set `s_scan_bot = 7` and open `SCR_SCAN`, in the Terminal `run_cmd` go/load block only. `scan keyfob` stays on `SCR_TERMINAL`, logs the same sweep as before, and leaves the cursor where it was |
| PANEL | Left panel inside (20,43)–(101,193). Silk from y=124 (pitch 7, Gibson pitch 6). Single device T=26 at (48,149). Gibson SCAN dual chips T=22 at (32,144) and (66,144), wire index 104, ticker 22×71 at (24,169). Gibson game is the 4×2 glossary grid. BT glyph is the existing `BOTCORPS_ICON_BT` |
| RAIN | Board rain is the background layer in RAIN_WIN (20,43)–(101,191), strand mode, faint 3/5, white lead. WIFI→AP, BLE→SPEAKER, AIRTAG→TRACKER, Gibson→all. Full-screen rain stays the firmware default. Gibson Defend is board rain only. No rain through Terminal tile faces |
| GREY | `sb_rect(g, dx - 9, dy + 44, 5, 44, 41)` is absent from `paint_sbox` (x97..101 y86..129) |
| ICONS | Plain Wi-Fi and BLE radar show 17 icons in 3 rows, no rail, console `sweep 17/17`. Gibson's crew drop-down is 3 rows for that lane. Lanes over 21 still use the rail. Scan-box overlays keep the previous 12-icon grid so their pixels stay inside the edit box |
| HEAP | `direct_tile_invert` logs `direct tile alloc fail w=%d h=%d largest=%u` once, then returns with no invert. After the I4 band line and before `botcorps_ui_mark_dirty`, the boot log prints `[botcorps] heap i4 free=%u largest=%u min=%u` |
| LIVE | Launcher `LIVE_SCAN=0`. NimBLE admit gate stays 48 KB. R0 caps unchanged |
| G7 | `release/gate/frozen.txt` masks only the scan-chip allow-list: Terminal tile slots, the grey strip on non-chip scan boards, and EDIT∪ZR (20,40)–(101,193) on the sbox, AIRTAG scan board, and game rows. `scan-wifi` and `scan-ble` may change freely and are not frozen. Settings, hub, hub-amber, and boot stay byte-identical to `locked-675c87d.sha256` |
