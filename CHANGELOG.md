# Changelog

## [Unreleased]

### Added

- Slice 0 scaffold for Cardputer Adv. PlatformIO envs `cardputer-adv`, `cardputer-adv-dual` (`-DBOTCORPS_DUAL_SCREEN=1`), and `cardputer-adv-dual-launcher` (dual firmware linked at launcher `ota_0` `0x170000`).
- `botcorps_display`: `botcorps_menu()` is the internal ST7789 240×135, `botcorps_content()` is the external ILI9341 320×240 when the panel comes up (ST7789 fallback otherwise), `botcorps_while_content()` paints on content and restores the previous surface. `BotcorpsDisplay` and `SCR_W` / `SCR_H` follow the active surface. Content is native 320×240, not a 240×135 corner, and the boot path does not park all UI on the external panel.
- External panel on SPI3, pins SCK 40 / MOSI 14 / CS 5 / DC 6 / RST 3 / SD_CS 12. GPIO5 idles HIGH. SD mount is wrapped in `botcorps_lcd_quiesce` / `botcorps_lcd_resume_after_bus`. RGB565 / COLMOD is boot-only.
- Slice 1R `botcorps_rain`: boot goes to bot rain. The internal panel shows the name BOTcorps on a slate bar. `loop()` advances the rain on the content panel. The Slice 0 navy and green size plates are not on the boot path.
- Slice 1R2 matches the approved rain mock: 90 ms steps with remainder carry, dense columns, bots and code only, 14-color bots, green code with a pale head, trails faded in the full-frame sprite (or an 8×8 cell table if that sprite cannot be allocated). The name stays on the internal panel. M5GFX is pinned at 0.2.32 and M5Unified at 0.2.13.
- Sprite fade and the bot backing rect now byte-swap `rgb565_2Byte` pixels, so field `0x0041` round-trips to near-black instead of being read as dark red. The cell fallback still paints through the library.
- Internal name bar is Font0 at text size 2, white on `#121821`, redrawn on the menu when that panel is painted. Rain does not draw it.
- S0 heap probe and S1 foundation. Rain step is 140 ms. INT rain, the intro screen, and the 180×180 radar sprite are gone. SD is not mounted at boot. NimBLE-Arduino 2.3.9 init and a Wi-Fi scan run only as a heap probe. Diagnostic numbers are on the diag screen. GPIO12 is parked high after M5 begin and before EXT begin. Heap lines record the largest free internal block before and after each init.
- S2 cell-grid rain uses the Tier 0 cell and column buffers and draws straight to the external panel. The 153600-byte rain sprite is gone. S3 puts icon buttons on EXT, the bot ID card on INT, and a terminal with boards, a prompt, and a command palette.
- Gate fix: DEVICES rain draws device icons, cursor and prompt edits repaint only the changed region, Terminal Esc closes the palette or sheet before leaving, and the radar sweep erases the previous rays instead of refilling the disc.
- Control panel row `RAIN COLOUR` (NVS namespace `botcorps`, key `rain_col`). Default MULTI keeps the 14-colour rain. Six solid inks are native RGB565 and fade toward field `0x0041`.
- Mock screens on the same boot path: rain, intro (ENTER powers on), Gibson and role card, terminal, Wi-Fi, Wi-Fi radar, Wi-Fi list, Bluetooth, Bluetooth radar, Bluetooth list, bot, bot radar, and the control panel. Keys are `;` `.` `,` `/`, Enter, and backtick. Wi-Fi and Bluetooth stay radio-off and show the sample list. The external sprite is parked on those screens when the largest free block is under 48 KB.
- Documented Cap conflict: LoRa1262 (NSS G5 / RST G3 / BUSY G6) and Hydra-on-G5 cannot coexist with this panel. guicmg RST GPIO13 / DC GPIO15 is not this pinout.

### Notes

- Active BOTcorps line is fresh Cardputer Adv dual. CYD / COM13 (`cyd28`) is parked and is not ported here.
- Build success is not a field PASS. No merge and no flash until Dirt asks.
