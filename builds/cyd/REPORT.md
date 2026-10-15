# BOTcorps CYD Touch Port Report

## Tip SHA
7eac44a

## Build
- Environment: `cyd`
- Board: ESP32 Dev Module (ESP32-2432S028 CYD)
- Display: ILI9341 320x240 via LovyanGFX
- Touch: XPT2046 configured; basic tap-as-OK wired in read_key
- Partition: huge_app.csv

## Memory
- RAM: ~72 KB used / 320 KB (≈22%)
- Flash: ~1.38 MB (fits huge_app)

## Screens
Reuses Adv drawing code. Single 320x240.
- SCR_RAIN (boot)
- SCR_GIBSON
- SCR_TERMINAL
- Wi-Fi / BLE lists, radars
- Bot screens
- Control / ID folded into the single surface

## Matching
- One screen, INT folded into EXT layout.
- Per-bot colors and ID badge kept.
- No instruction text added.
- Terminal v3.2 structure preserved.
- Typing path does not fire hotkeys (touch only returns K_OK).
- Fits RAM/flash. Built-in fonts.

## Incomplete
- Full on-screen keyboard not yet implemented (tap currently acts as select/OK).
- Optional external I2C keyboard not wired.
- Exact visual match to mocks needs device verification; layout fold of former INT may need position tweaks.
- Rain behind terminal is controlled by existing conditions.

## Build
`pio run -e cyd`
Bin: builds/cyd/botcorps-cyd.bin
