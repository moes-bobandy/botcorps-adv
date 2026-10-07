# Changelog

## [Unreleased]

### Added

- Slice 0 scaffold for Cardputer Adv. PlatformIO envs `cardputer-adv`, `cardputer-adv-dual` (`-DBOTCORPS_DUAL_SCREEN=1`), and `cardputer-adv-dual-launcher` (dual firmware linked at launcher `ota_0` `0x170000`).
- `botcorps_display`: `botcorps_menu()` is the internal ST7789 240×135, `botcorps_content()` is the external ILI9341 320×240 when the panel comes up (ST7789 fallback otherwise), `botcorps_while_content()` paints on content and restores the previous surface. `BotcorpsDisplay` and `SCR_W` / `SCR_H` follow the active surface. Content is native 320×240, not a 240×135 corner, and the boot path does not park all UI on the external panel.
- External panel on SPI3, pins SCK 40 / MOSI 14 / CS 5 / DC 6 / RST 3 / SD_CS 12. GPIO5 idles HIGH. SD mount is wrapped in `botcorps_lcd_quiesce` / `botcorps_lcd_resume_after_bus`. RGB565 / COLMOD is boot-only.
- Boot paints a distinct solid color and a small label on each surface so dual wiring can be checked later. That field check is what ENTER opens onto. No Ready Room or bot window.
- Documented Cap conflict: LoRa1262 (NSS G5 / RST G3 / BUSY G6) and Hydra-on-G5 cannot coexist with this panel. guicmg RST GPIO13 / DC GPIO15 is not this pinout.
- Slice 1 `botcorps_enter`: army-green millscale hatch drawn only on MENU (`botcorps_menu()`, ST7789 240×135). Stencil text prints left to right. Enter leaves the hatch. On a dual build the external panel stays on the Slice 0 content clear during the hatch. No rotate control, no external-panel pin or rotation change.
- Slice 2 `botcorps_ready`: Ready Room on MENU. Wi-Fi then Bluetooth, each with its own color and drawn icon before selection. `;` `.` `,` `/` move, Enter opens the seat, backtick returns. Dual builds paint the selected seat's name and color across the full external 320×240 only when the selection changes. Radio is not started. The seat state on the glass is `RADIO OFF` plus `2.4 GHz` (Wi-Fi) or `BLE` (Bluetooth).
- Slice 3 `botcorps_bot_window`: Enter on a seat opens that seat's window on the content panel, full native size. Millscale drift is drawn first; opaque header and footer (name, icon, `RADIO OFF`, the seat band, and backtick-back) are drawn after, so drift does not cross text. Frames are a sprite push (two sprites when the largest free block allows) or per-flake patches. The content panel is not cleared every frame. After 20s idle the screensaver runs on the external panel. Backtick restores the Ready Room.

### Notes

- Active BOTcorps line is fresh Cardputer Adv dual. CYD / COM13 (`cyd28`) is parked and is not ported here.
- Build success is not a field PASS. No merge and no flash until Dirt asks.
