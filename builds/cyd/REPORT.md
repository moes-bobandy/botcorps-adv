# BOTcorps CYD Touch Port Report

## Tip SHA
To be filled after commit.

## Build
- Environment: `cyd`
- Board: ESP32 Dev Module (ESP32-2432S028 CYD)
- Display: ILI9341 320x240 via LovyanGFX (HSPI: MOSI13 SCLK14 CS15 DC2 BL21)
- Touch: XPT2046 (VSPI: CLK25 MOSI32 MISO39 CS33 IRQ36) — configured, basic stub
- Partition: huge_app.csv (fits ~3MB app on 4MB flash)

## Memory
- RAM: ~72 KB used / 320 KB (≈22%)
- Flash: ~1.38 MB (fits with huge_app)

## Screens
Reuses Adv drawing code (botcorps_ui.cpp paint_* functions, band system, rain, glyphs, ID strip logic folded into single screen).
- SCR_RAIN (boot)
- SCR_GIBSON
- SCR_TERMINAL (v3.2 look preserved in draw)
- SCR_WIFI / RADAR / LIST
- SCR_BT / RADAR / LIST
- SCR_BOT / RADAR
- Control / ID elements folded into EXT layout (no separate INT panel)

## Matching notes
- Single 320x240 screen. INT content folded (ID badge, name bar drawn on main surface where paint_int calls are routed to the one display).
- Per-bot fixed colors and ID badge retained via existing bot_color / botcorps_id.
- No instruction text added; existing visual-only chrome kept.
- Rain behind terminal suppressed in draw paths; intro/rain has the existing rain.
- Terminal command palette / dropdown structure preserved in paint_terminal.
- Typing path stubbed so no hotkeys fire from touch.
- Built-in fonts (Font0 etc.).
- Fits CYD RAM/flash.

## Incomplete / not fully matched
- Touch navigation is currently a no-op stub in read_key (returns K_NONE). Full on-screen keyboard and tap-to-focus / tap-to-select not yet wired into the input layer.
- External I2C BlackBerry-style keyboard not implemented (optional per spec).
- Some dual-screen surface switches are no-op; INT paint is called but draws on the single panel (may overlap or need layout tweaks for perfect fold).
- Exact pixel match to mocks requires visual verification on device; drawing code reused but rotation/offset and band present may need calibration.
- No rain on terminal enforced only via existing conditions.

## Build command
`pio run -e cyd`

Bin: builds/cyd/botcorps-cyd.bin
