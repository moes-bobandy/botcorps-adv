# CONTEXT.md — BOTcorps CYD touch port (for Super Grok)

> DRAFT 2026-10-16 4:43 PM ET. Reread this file every session, then BRIDGE.md.
> Repo `moes-bobandy/botcorps-adv` · branch `grok/cyd-touch-port`. Owner: Dirt (Moes Bobandy). Reviewer: GIBSONbot.
> Anything marked **TBD** is not confirmed yet. Don't guess it. Ask in BRIDGE.md.

---

## 1. What BOTcorps is
BOTcorps Adv is handheld firmware for the M5Stack Cardputer-Adv (ESP32). It runs a crew of 12 bots, each with its own fixed colour and ID badge, plus Gibson, the lead/PRIME bot with its own hub and `gibson>` prompt. You use it through a Matrix-style terminal and scan screens. This branch ports it to the CYD (ESP32 "Cheap Yellow Display").

**Mission:** BOTcorps exists to alert, track and hunt. It scans Wi-Fi and BLE and shows what's nearby. When something is found, it takes over the screen. From there you can Hunt, Info, Tag or Ignore it. Every pixel serves that job, in a deep, saturated, Matrix-style look that matches Dirt's mocks exactly.

## 2. Hardware
| | Cardputer-Adv (main product line) | CYD (this port) |
|---|---|---|
| Screens | EXT plus a small INT screen (240x135 ID card) | **One 320x240 screen. No INT screen.** |
| Input | Built-in keyboard only. No touch. | **Touch (XPT2046) is the main way to navigate**, plus an on-screen keyboard (OSK). A BlackBerry-style I2C keyboard is planned but not wired yet. |
| Panel | n/a | ILI9341, 4-wire SPI (MISO 12) |
| Build | `cardputer-adv`, `-dual`, `-dual-launcher` | `pio run -e cyd`, huge_app partition |

- **EXT ILI9341 washes out pale colours.** Pale tints and greys look hazy on the glass, so use deep, saturated colours and deep black (#020306 / #030407). The pale haze was a field FAIL (Oct 15).
- CYD code lives under `#ifdef BOTCORPS_CYD`. It must never change Adv behaviour. Change Adv files only by adding `#ifdef` blocks.
- XPT2046 calibration is still the placeholder 300..3900. It will be stored in NVS (`tcal=1`), but that has to be done on the device: **TBD**.

## 3. Core concept
- **Each bot IS the ESP32 chip** drawn on a CYD / dev-board layout: ESP32-WROOM-32 module, castellated pads, mounting holes, pin headers, USB-C/BOOT/RST pads, silkscreen bot name and a red status LED, on a deep-blue PCB (#0A2F7A). It should look **like an ESP32 board**. Gibson is the gold PRIME module (crown, "CORE ★ PRIME").
- The Terminal shows the 12 bots as 12 ESP32 chip tiles in their fixed colours. Selecting a bot goes to its scan. There are no separate per-bot screens.
- **Matrix bit rain:** each column is a falling code strand, and the **bots and devices move inside the strands**. They are the main focus; bits and code are filler.
  - Weights: bots 35 / devices 25 / bits 30 / code 10.
  - Gibson screen: ivory Gibsons 35 / devices 35 / bits 22 / code 8.
  - Ten device types: AP, phone, laptop, TV, speaker, tracker, watch, camera, car, unknown. Each has its own saturated colour.
  - Rain is faint (42%) behind scan and strong everywhere else, masked so it never sits under UI. There is no rain on the Terminal.
  - The rain mode (BACKGROUND) stays a selectable setting.

## 4. Locked rules
- **Always** give each bot ONE fixed colour (`bot_color(i)`) on every screen: tiles, ID, replies, scan. **Never** recolour per screen.
- **Always** keep the ID badges. **Never** remove them. A badge tap opens the OSK (Dirt asked for this).
- **Always** keep the Tab sheet's hard edge (locked 8:21 PM ET Oct 15). **Never** add a glow, halo, bloom or shadow outside it.
- **Always** use the crown as the only back button, and Esc goes back (Esc closes the OSK first). **Never** add other back buttons or labels.
- **Never** put instruction text on screen: no "press ENTER", "TAP TO ENTER", hints or key legends. Use icons only.
- **Always** match the mock exactly. **Never** invent a style.
- **Always** change the mock first, then the firmware. **Never** add UI that isn't in a mock.
- **Always** keep all scanning inside the scan mini screen. That means:
  - device icons;
  - the in-box Hunt / Info / Tag / Ignore / Back prompt;
  - the hunt meter;
  - the GLOSSARY prompt at top-left.

  **Never** spill scan UI outside the box.
- **Always** limit live scanning to the Wi-Fi and BLE bots. **Never** fake live scan on other bots.
- **Always** let the full-screen DEVICE FOUND alert take over any screen and stay until Hunt / Info / Ignore / Back.
- **Always** build for the core purpose: alerting, tracking and hunting.
- **Always** use a deep, saturated palette. **Never** use pale washes (ILI9341).
- **Always** use the maximum app partition (huge_app).
- **Never** let typing trigger hotkeys or wipe the prompt. The prompt clears only after Enter.
- **Always** follow the touch rule: the first tap focuses, the second tap selects. Labels show only on the focused tile.

## 5. Process rules for Super Grok
- **Always** push only to `grok/cyd-touch-port`. **Never** merge, flash or open a release.
- **Always** be honest in REPORT. **Never** claim a field, glass or device PASS. A successful build is not a PASS. Say "no board" when there isn't one, and call stub frames stubs.
- **Always** set the REPORT tip SHA and the mock subtitle to the final HEAD after your last push, or bump them only when firmware changes. This ends the tip-lag loop.
- **Always** author commits as Super Grok. (At present they show as `Grok <grok@x.ai>`; the exact name and email to use are **TBD**.)
- **Always** stay within the Super Grok allowance. The limit is **TBD**.
- **Always** reply in BRIDGE.md: replace the "FROM SUPER GROK → GIBSONbot" block and add a log line (tip SHA, fw SHA, changes, bin size, RAM/flash, live status, what's still open).
- **Never** commit build binaries (`builds/cyd/*.bin`). Don't include secrets.

## 6. Current state and open items (TASKS.md / BRIDGE.md, Oct 16)
- Branch tip `9cf96fd`, firmware `acfc0e3` (last `src/` change). The last round changed only meta files.
- `pio run -e cyd` (Super Grok-reported, not verified on the box):
  - RAM 72,076 / 327,680 (22.0%)
  - flash 1,404,031 / 3,145,728 (44.6%)
  - firmware.bin 1,404,432 B
- Host type gate: PASS (OSK table, 42 keys).

Done:
- touch nav (focus/select)
- debounce
- OSK paint and hit-test from one table
- badge→OSK
- Esc closes the OSK
- band clip at y=151
- no instruction words in the CYD mock

Open:
1. The REPORT tip and mock subtitle lag HEAD (they say `dddee10`; tip is `9cf96fd`).
2. Soft: show the live LV instead of the hardcoded "LV 01".
3. Soft: real CYD host frame dumps. The frames are still placeholder stubs.
4. Device: XPT2046 calibration to NVS, plus glass shots. Colours, labels and rain haven't been checked on glass.
5. P1: rebuild the Adv envs (check that NimBLE doesn't pull in a second BLE stack); wire the I2C BlackBerry keyboard.
6. Port the latest master mock (`BOTcorps-Adv-master-v1016-1604`): scan mini screen, hunt prompt, hunt meter, GLOSSARY, full-screen found alert. When this happens on CYD is **TBD**; wait for a task in BRIDGE.

## 7. Decision history (ET)
- **Oct 15, ~11:40 AM:** Field video of the Adv build was a FAIL. The HIT LIST: drop per-bot screens, prompt nearer v3.2, Tab menus, Gibson prompts, radar banner, no Enter flash, typing vs hotkeys, `scan all`, pale haze.
- **Oct 15:** GIBSONbot's first CYD review (b433b2e). It set the P0 list: touch nav, debounce, OSK, no INT overdraw.
- **Oct 15, 8:21 PM:** Tab sheet hard edge locked (no outer glow).
- **Oct 15, ~9 PM:** Dirt confirmed that a badge tap opens the OSK.
- **Oct 15, ~10 PM:** No instruction words on the 320x240 panel; icons only.
- **Oct 16 AM:** Scan chip became an ESP32-WROOM module on a deep-blue CYD board (v5-v16), and Gibson became the gold PRIME chip.
- **Oct 16, 10:36-11:03 AM:** Rain v4b, v5 and v6 locked: bots and devices lead, Gibson rain has devices, faint 42% on scan, strong elsewhere, masked under UI. The rain mode stays selectable.
- **Oct 16, 11:05 AM:** "Add the whole fail list to the mock before going forward." This produced FAILLIST-MASTER (F1-F18) and a GAME button next to scan on the Gibson hub.
- **Oct 16:** BRIDGE.md created as the GIBSONbot ⇄ Super Grok channel.
- **Oct 16 (afternoon):** Master mock v1016-1604 brought back the in-box Hunt / Info / Tag / Ignore / Back list, and the full-screen FOUND alert now takes over any screen. Exact lock times are **TBD**.

## 8. Paste-in prompt (Dirt → Super Grok, each session)
```text
Read CONTEXT.md on grok/cyd-touch-port first, then BRIDGE.md (block "FROM GIBSONbot → SUPER GROK").
Do that task only, following every locked rule. Push only to grok/cyd-touch-port, no merge/flash.
Reply in BRIDGE.md with tip SHA, fw SHA, bin size, RAM/flash, and an honest live status.
```
