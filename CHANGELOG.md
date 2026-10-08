# Changelog

## [Unreleased]

### Added

- Slice 0 scaffold for Cardputer Adv. PlatformIO envs `cardputer-adv`, `cardputer-adv-dual` (`-DBOTCORPS_DUAL_SCREEN=1`), and `cardputer-adv-dual-launcher` (dual firmware linked at launcher `ota_0` `0x170000`).
- `botcorps_display`: `botcorps_menu()` is the internal ST7789 240×135, `botcorps_content()` is the external ILI9341 320×240 when the panel comes up (ST7789 fallback otherwise), `botcorps_while_content()` paints on content and restores the previous surface. `BotcorpsDisplay` and `SCR_W` / `SCR_H` follow the active surface. Content is native 320×240, not a 240×135 corner, and the boot path does not park all UI on the external panel.
- External panel on SPI3, pins SCK 40 / MOSI 14 / CS 5 / DC 6 / RST 3 / SD_CS 12. GPIO5 idles HIGH. SD mount is wrapped in `botcorps_lcd_quiesce` / `botcorps_lcd_resume_after_bus`. RGB565 / COLMOD is boot-only.
- Slice 1R `botcorps_rain`: boot goes to bot rain. The internal panel shows the name BOTcorps on a slate bar. `loop()` advances the rain on the content panel every pass, then redraws the opaque name bar so marks stay behind the glyphs. The Slice 0 navy and green size plates are not on the boot path.
- Documented Cap conflict: LoRa1262 (NSS G5 / RST G3 / BUSY G6) and Hydra-on-G5 cannot coexist with this panel. guicmg RST GPIO13 / DC GPIO15 is not this pinout.

### Notes

- Active BOTcorps line is fresh Cardputer Adv dual. CYD / COM13 (`cyd28`) is parked and is not ported here.
- Build success is not a field PASS. No merge and no flash until Dirt asks.
