# BOTcorps CYD Touch Port Report

## Tip SHA
f94e599

## Build
- Environment: `cyd`
- Board: ESP32 Dev Module (ESP32-2432S028 CYD)
- Display: ILI9341 320x240 via LovyanGFX
- Touch: XPT2046 (calibration placeholders 300..3900; NVS storage pending)
- Partition: huge_app.csv

## Memory
- RAM: ~72 KB used / 327 KB (≈22%)
- Flash: ~1.40 MB (fits huge_app)

## Screens
Single 320x240 surface. Adv drawing paths reused where possible.
- SCR_RAIN (boot / intro)
- SCR_GIBSON
- SCR_TERMINAL
- Wi-Fi / BLE lists, radars
- Bot screens
- Compact ID badge (right-hand region) — full INT card skipped on CYD

## Matching / Status (honest)
- Compact ID badge implemented; full paint_int / id_tick skipped on CYD.
- OSK uses shared rect table matching paint geometry; real SPACE/BKSP/ENTER/hide; painted only on open/change (not every loop).
- Touch: focus-then-select on board tiles; top-bar / dropdown / back hit-tests; no blanket K_OK.
- Palette constexpr assert restored and enabled for CYD.
- builds/cyd/botcorps-cyd.bin untracked (covered by *.bin).
- Typing path: OSK → K_CHAR → on_char; hotkey isolation confirmed in code (device verify still needed).
- Per-bot color path for Terminal replies present (device confirm vs mock).
- Board labels: focused-tile only (verify on device).
- Rain: Terminal excluded; intro density vs v11 needs device check.
- Adv VIEW_MAX / comments restored if previously stripped.
- NimBLE added to shared deps; Adv envs build check pending.
- Bus: lgfx_cyd 3-wire vs MISO 12 noted; prefer 4-wire typical for CYD.
- Optional I2C keyboard not wired.
- Full visual match to mocks and device flash test still required.
- Nothing flashed or merged.

## Build
`pio run -e cyd`
Bin (local, not committed): builds/cyd/botcorps-cyd.bin
