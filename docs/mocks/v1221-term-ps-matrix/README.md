# MOCK 2026-11-25: TERMINALbot pre-scan typists A / B (LV_SC_TPS, LV_SC_TPSB)

New TERMINALbot subscreens only. No firmware build, upload, or flash.

Two full-screen, no-chrome typists. The screen is not interactive: it is a
passive pre-scan terminal that auto-types locked BOTcorps commands and lists
everything around by name and device-bot class. Power-save stays on while
scanning continues. SAMPLE data from the host-sim canned wifi/ble set.

The original Adv Matrix operator typewriter mock was not recovered (box data
loss; one search pass, including GitHub `backup/*` and Adv trees). Type-out
is our own typist (LV_F35 glyphs, `(lv_now/500)&1` block cursor), not a
clone of the film or of that missing mock.

## Command language (locked, not invented)

Auto-typed commands and output labels come from the locked Defend command
sheet and the live TERM device-bot names:

- Syntax (src/ui/botcorps_defend.inc `LOCK-DEFEND-CMDS`): typed
  `verb + device name [+ sub]`. Core verbs on every board: `scan`, `lock`,
  `zap`, `purge`, `restart`. A bare device name is `zap`. This screen only
  auto-types `scan` — it does not issue lock/zap/purge or signature
  subs, and it does not take input.
- Device names: `BOTS[].short_name` / `LV_HUNT_NAME` —
  `wifi`, `bluetooth`, `deauth`, `hack`, `flock`, `drone`, `airtag`,
  `keyfob`, `meta`, `skimmer`, `flipper`, `samsung`.
- Prompt and result tags from `zap_try_cmd`: `gibson> …`,
  `[ .. ] SCAN NAME N LIVE`, `[ -- ] NO NAME ON GRID`.
- Device-bot class labels: `LV_HUNT_BOT` (`AIRTAGbot`, `FLOCKbot`, …) on A;
  short_name in the B CLASS column.
- A also auto-types the TERM radio verbs `scan wifi` / `scan bluetooth`
  (same names as Defend devices 0 and 1) before the per-class scans.
- Core row `core  scan lock zap purge restart` is the command-sheet
  core line (botcorps_ui.cpp), shown as a reminder only.
- DEAUTH is not in `LV_J_ALL` / the canned wifi-ble set, so both mocks
  print the locked `[ -- ] NO DEAUTH ON GRID` (no invented radio row).
- `TELLO-5A21B3` matches both HACKbot (open/WEP wifi, `auth <= 1`) and
  DRONEbot (name/icon). It is listed under each hunt that matches, labeled
  with that section's class.

## Entry / exit (no on-screen controls)

- Enter A from TERMINALbot minterm **LIST**, or TTY/sim `psterm` / `tps` /
  `tpsa`.
- Enter B from TTY/sim `tpsb` / `psterm-table`. There is no extra on-device
  chrome entry for B.
- Exit either screen by tapping anywhere: back to TERM. Leave restores the
  saved backlight. No banner, frame, strip, pins, or buttons.

## Look

Full 240×320 console, no border/frame/strip/banner. Field `#000200`, body
ink `#33e59a`, 1 px trail glow `#00351d`, fresh glyph `#5FFFAF`, block
cursor `#B2FFFE`. 14 px left / 16 px top, 14 px rows (7 px glyph + 7 px
black), 35 columns, wrap or Windows-style `...` truncation. No digital
rain (HACKbot `dl_intro` typist has none; we followed that). Original
wording only — no film lines.

This is a BOTcorps terminal, not a Matrix replica. The references below
were used only to set hue, glow, type-out cadence, cursor, and spacing.

- **A** — scrolling typist log: radio scans, then `gibson> scan <class>`
  plus the locked SCAN / NO-GRID tag and one name + class + RSSI/CH line
  per find.
- **B** — the same commands and tags, then a Format-Table (header + dash
  underline, aligned `NAME CLASS RSSI CH` columns) for each of the 10
  hunt classes. A 240×320 / 14 px-row pane shows ~21 lines, so the B
  (and A) "full" still is the tail of the log; the GIF is the mid typist
  pass over earlier classes.

Idle A and idle B match at the captured typist moment (shared header is
still being typed; the two mocks have not diverged yet).

## Power-save (what the code actually lowers)

Measured on the host sim at enter (`t=2400` in the shot scripts):

- Backlight PWM: `lv_cb_blset(LV_TPS_DIM_BRI=20)` → **20% (duty 51/255)**.
  Boot was `lv_cb_set.bri` **80% (duty 204/255)**. The saved setting is
  not mutated; `lv_tps_leave` restores it.
- Frame interval: **120 ms** (`LV_TPS_PERIOD`).
- Redraw: first frame after enter is a full `lv_blit` (`lv_tps_full`);
  later frames use `lv_blit_dirty` (changed rows only).
- Typist: **80 ms/char** + 3 char-times of pad per line. Film "Wake up"
  lines on the references run ~150–300 ms/char; 80 ms is a deliberate
  compromise so a full log / short GIF is reachable in sim time.
- Scanning continues: enter starts `LV_J_ALL`; `lv_job_tick` still runs
  before the TPS early-return in `lv_tick`.

## Reference research (guides, not a redesign; comparison only)

Local copies live in `/workspace/refs/matrix-terminal/` on the box. They
are **not** in this commit and must not be shipped in a firmware bin.

| Source | Local path | What was taken |
| --- | --- | --- |
| Jonny Campbell, film-computer / Matrix opening stills | `jonny-campbell-og.jpg` (1200×630) | Field `#000200`, trail/fade `#00351d`; sparse top-left type-out with lots of black; IBM-CRT-like green as a direction, not a hex-accurate film grade |
| kirocrew theme-matrix 4K samples | (comparison pass; hex from that theme) | Body text `#33e59a` (~hue 154°), pale block cursor `#B2FFFE` slightly taller than the 7 px glyph |
| narkhy/MatrixHasYou reconstruction | (comparison pass) | Closest 8-bit fresh-glyph `#5FFFAF`; film per-char times measured ~150–300 ms on the opening lines |
| xscreensaver `xmatrix.c` trace typist | `xmatrix-trace.c.txt` | Confirmed a typist exists on the "trace programme" family of screens; that source's ~10 ms/char + 1.5 s line delay is *faster* than the film opening — we did not use that rate |
| Microsoft Learn Format-Table | `powershell-format-table-example.txt` | Header row, dash underline as wide as each column, spaces between columns, left-aligned text, numeric columns right-aligned, Windows end-ellipsis `...` on a long NAME |

Do not copy movie lines verbatim. Do not ship copyrighted frames.

## Gate

Tour `tools/livescan/scripts/v1220_tour.txt` vs v1.2.21 head `d7b55459`
sim dumps (`/tmp/v1220/gate121s`, no `nocal`): **0 px on all 11 screens**.
See `gate_px.txt`.

## Shots

Host sim `tools/livescan/scripts/tps_shots_A.txt` / `tps_shots_B.txt`.
`@1x` is 240×320; `@3x` is nearest-neighbor 720×960.

- A: `tps_A_idle`, `tps_A_scanning`, `tps_A_full` `@1x`/`@3x` + `tps_A_scroll@3x.gif`
- B: `tps_B_idle`, `tps_B_scanning`, `tps_B_full` `@1x`/`@3x` + `tps_B_scroll@3x.gif`

## Branch

`mock/v1221-term-ps-matrix-2026-11-25` from `d7b55459` (build/v1.2.21-2026-11-25).

---

## Variants C & D (new, keeping A & B untouched): Rain-Lock & Backdrop-Emerge

Added `LV_SC_TPSC` (`tpsc` / `psterm-rain`) and `LV_SC_TPSD` (`tpsd` /
`psterm-veil`) on top of `8f43e93`. A and B files and behavior remain
untouched.

- **C (`tpsc`) — Rain-Lock Materialize**: Pure `#000000` black field with
  39 columns × 22 rows (`13 px` vertical pitch: `7 px` glyph + `6 px`
  black). Matrix-inspired 5×7 katakana/numeral rain glyphs (`LV_TPS_RAIN_GL`,
  synchronized 3-frame cell swap, `#E0FCD9` leaders, `#43FF33 -> #00990B ->
  #00351d -> #001F04` decay, 3×3 max-RGB phosphor bloom) descend down
  columns into each forming line's character positions. Characters lock
  with organic per-column scatter from swirling code glyphs into crisp
  BOTcorps ASCII with a bright `#E0FCD9` flare, then cool to `#33e59a` /
  `#5FFFAF` while sparse background rain continues falling through empty
  rows and gutters.
- **D (`tpsd`) — Backdrop-Emerge + Rain-Veil**: Sparser, darker ambient
  rain (`~20%` columns, `#001F04 / #00351d`), where lines ghost upward
  directly out of `#000000` through `#001F04 -> #00351d -> #00990B`
  code shimmer, flash `#E0FCD9` on lock with a short drip beneath, and
  settle on pitch black.
- **Content**: Passive pre-scan using the same locked Defend + `BOTS[]`
  command language (`gibson> scan wifi`, `gibson> scan bluetooth`,
  `gibson> scan <class>`, `[ .. ] SCAN ... LIVE`, `[ -- ] NO DEAUTH ON GRID`)
  plus uncategorized `WIFIbot` / `BLEbot` entries so every device in
  `lv_f[]` is listed by name and device-bot class.
- **Power-save (unchanged)**: `lv_cb_blset(20)` → 20% backlight (duty
  51/255 vs 204/255 boot), 120 ms frame interval, first frame `lv_blit`
  then `lv_blit_dirty` (changed rows only), `LV_J_ALL` keeps running.
- **Additional references**:
  - `carlnewton.github.io/digital-rain-analysis/` + local `/workspace/refs/matrix-terminal/digital-rain-01.gif` / `digital-rain-01-frame.png`: fixed-cell descending edge, synchronized 3-frame swap, 1-in-5 highlighted leaders, deletion gaps.
  - `matrix.watch/styles/authentic.html` + local `/workspace/refs/matrix-terminal/matrix-watch-og.png`: 1999 4-stop ramp `#000000 / #001F04 / #00990B / #43FF33 / #E0FCD9` + bloom.

### C & D Shots

Host sim `tools/livescan/scripts/tps_shots_C.txt` / `tps_shots_D.txt`.

- C: `tps_C_materialize`, `tps_C_full` `@1x`/`@3x` + `tps_C_scroll@3x.gif`
- D: `tps_D_materialize`, `tps_D_full` `@1x`/`@3x` + `tps_D_scroll@3x.gif`

---

## Variant E (new, A–D untouched): Table rain-lock on a dark backdrop

Added `LV_SC_TPSE` (`tpse` / `psterm-table-rain`) as a blend of B, C, and D.
A, B, C, and D files and behavior are unchanged.

- **Layout**: mock B's Format-Table output. One table per hunt class
  (`gibson> scan <device>`, locked `[ .. ] SCAN … LIVE` / `[ -- ] NO … ON GRID`,
  then `NAME CLASS RSSI CH` with a dash underline and aligned rows). DEAUTH
  stays `[ -- ] NO DEAUTH ON GRID` with an empty table (no invented find).
  `SAMPLE` at the end.
- **Materialize**: mock C's rain-lock — Matrix-inspired glyphs descend the
  column into each table header/row cell, lock with column scatter and an
  `#E0FCD9` flare, then cool to `#33e59a` / `#5FFFAF`.
- **Backdrop**: mock D's sparse, dark ambient rain (~20% of columns, short
  trails, `#001F04` / `#00351d` decay, no bright leaders) on pure `#000000`.
- Entry: TTY/sim `tpse` / `psterm-table-rain`. Exit: tap anywhere → TERM.
  No on-screen chrome. Power-save unchanged (20% backlight, 120 ms dirty-row
  frames, `LV_J_ALL` keeps running).

### E Shots

Host sim `tools/livescan/scripts/tps_shots_E.txt`.

- `tps_E_materialize`, `tps_E_full` `@1x`/`@3x` + `tps_E_scroll@3x.gif`

---

## Variants F & G (new, A–E untouched): One-screen compact Trace & Cursor-Type (no rain)

Added `LV_SC_TPSF` (`tpsf` / `psterm-trace`) and `LV_SC_TPSG` (`tpsg` /
`psterm-wake`). A–E image files and behavior remain untouched.

Both use `LV_F35` (3×5 caps, `4 px` horizontal pitch = 56 columns, `8 px`
row pitch = 38 rows) so **every detected device across all 10 device-bot
classes plus WIFI/BLE fits on ONE 240×320 screen with no scroll**:

- 4 header rows (`NAME / CLASS / RSSI / CH` + dash underline)
- 12 locked BOTcorps command/status rows (`GIBSON> SCAN <DEV>` +
  `[ .. ] SCAN <DEV> <N> LIVE` or `[ -- ] NO DEAUTH ON GRID`)
- All 21 device rows (`WIFIbot`×5, `BLEbot`×5, `AIRTAGbot`, `KEYFOBbot`,
  `METAbot`, `HACKbot`×3, `SKIMMERbot`, `FLOCKbot`, `FLIPPERbot`,
  `DRONEbot`, `SAMSUNGbot`) + trailing `GIBSON>` = 38 rows (`y = 8..312`).
- Pure `#000000` black, no rain, no border/UI/controls. Enter: `tpsf` /
  `tpsg` (`psterm-trace` / `psterm-wake`). Exit: tap anywhere → TERM.
  Power-save unchanged (20% backlight, 120 ms dirty-row frames, `LV_J_ALL`).

- **F (`tpsf`) — Trace Program look**: Guided by the 1999 *Matrix* opening
  *"Call trans opt: received ... Trace program: running"* sequence
  (`overblack.org/script/the-matrix-1999`, `trace-program-ref.txt`).
  Unlocked cells cycle racing green digits (`0..9`, 90 ms cadence), then
  snap and lock character-by-character with a brief `#E0FCD9` flare into
  `NAME / CLASS / RSSI / CH` until line 0 flips from `TRACE: RUNNING`
  to `TRACE: COMPLETE`.
- **G (`tpsg`) — "Wake up, Neo..." look**: Guided by Neo's opening
  terminal (`narkhy/MatrixHasYou`, `leozhad/kirocrew-theme-matrix`
  `kirocrew-splash.svg`, `jonnycampbell.com/notes/film-computer-type/`).
  Starts on empty `#000000` black with a blinking `#B2FFFE` block cursor
  (`4×7`), typing BOTcorps commands and device rows with cadence jitter
  and deliberate cursor-blink pauses (`700 ms` / `450 ms` / `320 ms`)
  without scrolling.

### F & G Shots

Host sim `tools/livescan/scripts/tps_shots_F.txt` / `tps_shots_G.txt`.

- F: `tps_F_materialize`, `tps_F_full` `@1x`/`@3x` + `tps_F_scroll@3x.gif`
- G: `tps_G_materialize`, `tps_G_full` `@1x`/`@3x` + `tps_G_scroll@3x.gif`
---

## Variants F2 & G2 (new, A–G untouched): Mixed-case 5×7 CRT Trace & Wake-Up Cursor

Added `LV_SC_TPSF2` (`tpsf2` / `psterm-trace2`) and `LV_SC_TPSG2` (`tpsg2` /
`psterm-wake2`). All A–G image files and screens remain untouched.

Solves the 3×5 readability & case-loss issue (`WIFIBOT` → `WIFIbot`) while
still fitting every detected device on **one 240×320 screen with no scroll**:

- **Typography & columns**: Uses mixed-case `5×7 LV_GLCD` (`6 px` pitch = 38
  columns at `x = 6`, `11 px` row pitch at `y0 = 10`). Tighter columns:
  `NAME` (16 chars, truncated cleanly with `...`), `CLASS` (10 chars, e.g.
  `AIRTAGbot`, `WIFIbot`), `RSSI` (4), `CH` (3).
- **Compact layout (27 rows total for SAMPLE 20-device deduped set)**:
  - Row 0: `CALL OPT:RX gibson> trace:run  SAMPLE` (F2; flips to `trace:done`)
    or `gibson> scan all  SAMPLE` (G2)
  - Row 1: `core: scan lock zap purge restart`
  - Row 2: `[ .. ] SCAN WIFI 9 LIVE  BLE 11 LIVE`
  - Row 3: `[ -- ] NO DEAUTH ON GRID`
  - Rows 4–5: `NAME              CLASS       RSSI CH` + dash underline
  - Rows 6–25: 20 deduped detected devices (`TELLO-5A21B3` deduped to its
    primary `DRONEbot` class instead of repeating under `HACKbot`; ordered by
    BOTcorps hunt class then `WIFIbot`, `BLEbot`)
  - Row 26: `gibson>`
- **Overflow rule (`lv_tps2_build`)**:
  - `<= 20` devices: `5×7 LV_GLCD`, `11 px` pitch (up to 27 total rows).
  - `21..24` devices: `5×7 LV_GLCD`, `10 px` pitch (`y0 = 6`, up to 31 rows:
    6 header + 24 devices + footer).
  - `25..31` devices: auto-switches to `3×5 LV_F35`, `8 px` pitch (`y0 = 8`,
    22-char `NAME`, up to 38 rows: 6 header + 31 devices + footer).
  - `> 31` devices: stays in `3×5 LV_F35`, sorts devices by strongest RSSI
    descending, shows the strongest **30** devices plus 1 overflow row
    `+K more (strongest 30 shown)` (38 total rows).
- **CRT / Matrix refinement**:
  - Authentic 1999 Matrix phosphor palette (`#000000` black, `#00351d` bloom,
    `#00990B` dim/trail, `#33e59a` / `#5FFFAF` crisp body/headers, `#43FF33`
    racing trace wave peaks, `#E0FCD9` lock-snap flash, `#B2FFFE` block cursor).
  - Subtle 4-neighbor `max-RGB` `#00351d` phosphor bloom (with `#00990B` cross
    + `#00351d` diagonal halo on `#E0FCD9` lock flashes) and faint `7/8`
    odd-scanline CRT modulation over lit/bloomed pixels.
  - **F2 (`tpsf2`)**: Racing vertical green-wave digits (`0..9`) across
    unlocked cells that lock character-by-character (`~15.6 s` full lock).
  - **G2 (`tpsg2`)**: Empty black start (`700 ms` blink pause), `6×8` `#B2FFFE`
    blinking block cursor (`350 ms` half-period), ominous slow typing cadence
    with pauses (`850 / 450 / 420 / 220 ms`).

### F2 & G2 Shots

Host sim `tools/livescan/scripts/tps_shots_F2.txt` / `tps_shots_G2.txt`.

- F2: `tps_F2_materialize`, `tps_F2_full` `@1x`/`@3x` + `tps_F2_scroll@3x.gif`
- G2: `tps_G2_materialize`, `tps_G2_full` `@1x`/`@3x` + `tps_G2_scroll@3x.gif`

---

## Live Theme Color UI Support on F2 & G2 (existing green baselines untouched)

F2 (`tpsf2` / `LV_SC_TPSF2`) and G2 (`tpsg2` / `LV_SC_TPSG2`) now follow the
firmware's selected Color UI (`lv_theme`) — including all single-color themes
and all multi-color themes (`lv_multi_theme(lv_theme)`):

- **Reused firmware theme getters (verbatim)**:
  - `lv_theme` (`src/live/lv_core.h:280`)
  - `lv_multi_theme(int t)` (`src/live/lv_core.h:189`)
  - `lv_theme_at(int t, int x, int y)` (`src/live/lv_core.h:201-217`, which
    returns `LV_ACC[t]` for single-color themes and delegates `7..10`
    `SYNTHWAVE/AURORA/PRISM/GLITCH`, `13..21` `sec_theme_at`, and `22..25`
    `ext_theme_at` per pixel `(x, y)`)
  - `lv_mix3()` (`src/live/lv_core.h:195-200`) and `sec_legible()`
    (`src/live/controlbot/themes_spec_v2.h:117-121`)
  - `lv_blit_hw` / `lv_blit_dirty_hw` (`src/live/lv_core.h:273, 2684`) so
    final resolved multi-color pixels bypass `lv_theme_ink()` sentinel
    double-re-inking.
- **Same 7-role shade ramp**:
  - `T1`: `mix3(T, black, 79, 100)` (~21%, bloom + trace trail)
  - `MID`: `mix3(Tl, black, 40, 100)` (60%, mid trace + `NO DEAUTH` + flash halo)
  - `BODY`: `mix3(Tl, white, 1, 8)` (device rows)
  - `TAG`: `mix3(Tl, white, 2, 5)` (headers / status / cooling, matching `LV_HDR`)
  - `HOT`: `Tl` (100% legible accent, wave peak + overflow row)
  - `LEAD`: `mix3(Tl, white, 4, 5)` (lock snap flash + fresh typed glyph)
  - `CUR`: `mix3(Tl, white, 7, 10)` (G2 block cursor)
  where `Tl = sec_legible(T)`.
- **Note on theme 0**: In `lv_core.h`, `lv_theme = 0` is `NEON GRAPE`
  (`0xb324fd`, purple), not Matrix green (`0x33e59a` was a Matrix-only constant
  in `lv_term_ps.h`); `TOXIC` (`theme 2`, `0x92ff02`) and `NVG` (`theme 13`)
  are the firmware's green themes. All existing green `tps_F2_*` / `tps_G2_*`
  baseline files remain untouched (`0 px` non-black layout/bloom/scanline mask
  difference across all themed renders).

### Theme-Aware F2 & G2 Renders
- Single-color: `grape` (`0`), `toxic` (`2`), `ice` (`3`), `orange` (`5`) ->
  `tps_{F2,G2}_{grape,toxic,ice,orange}_full@3x.png`
- Multi-color: `synth` (`7`) `full@3x.png`, `prism` (`9`) `full@3x.png` +
  `tps_{F2,G2}_prism_scroll@3x.gif`
