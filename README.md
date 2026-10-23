# botcorps-adv

BOTcorps / CYBERbots Security — fresh Cardputer Adv dual-screen firmware.

Internal ST7789 **240×135** is the menu. External ILI9341 **320×240** is content. Both stay live. This tree is not a port of the CYD / COM13 `cyd28` firmware, and CYD bins do not belong on this board.

## Always / never

| Always | Never |
|--------|-------|
| Active BOTcorps line = fresh Cardputer Adv dual (INT ST7789 240×135 menu / EXT ILI9341 320×240 content), parallel product, same ideas | Treat CYD / COM13 BOTcorps as the active floor until Dirt unlocks it |

Slice 0 is the display split. Slice 1R boots straight into bot rain. A green `pio run` is not a field PASS. Do not flash a device from this tree until Dirt asks.

## Build

PlatformIO, three environments, no upload:

```bash
pio run -e cardputer-adv
pio run -e cardputer-adv-dual
pio run -e cardputer-adv-dual-launcher
```

| Env | What it is |
|-----|------------|
| `cardputer-adv` | Internal ST7789 only. `botcorps_menu()` and `botcorps_content()` are both `M5Cardputer.Display`. |
| `cardputer-adv-dual` | Dual split, built with `-DBOTCORPS_DUAL_SCREEN=1`. |
| `cardputer-adv-dual-launcher` | Same dual firmware, linked into a launcher `ota_0` slot at `0x170000` (`support_files/launcher_8Mb.csv`) so a dual-screen launcher can start the bin. |

Board is the Cardputer Adv's Stamp-S3 (`m5stack-stamps3` on this platform). There is no `cyd28` env.

Boot check: `setup()` opens the rain screen and returns. `loop()` reads the keyboard and calls `botcorps_rain_frame()` while the external rain is on. There is no `paint_menu`, no `paint_content`, and no width-by-height label on the glass. The Slice 0 navy `MENU` plate and green `CONTENT` plate are not a boot destination.

The internal panel is the menu: dim bot-rain behind a 22px slate bar, white Font0 "BOTcorps" at text size 2, and the mock's grey-blue rule. The external panel is the main UI. On the rain screen it is bot rain only (dense columns, bots and code, 90 ms, same 14-color palette). Marks step every 90 ms; leftover milliseconds carry into the next step. Sprite fade swaps `rgb565_2Byte` pixels on read and write so the field stays near-black. If the sprite cannot be allocated, or a Wi-Fi/Bluetooth screen needs the heap and the largest free block is under 48 KB, rain uses the 8×8 cell table. Serial prints `rain path=sprite` or `rain path=fallback`, plus `heap` lines at boot and when those radio screens open. Wi-Fi and Bluetooth do not start a radio; the lists are idle samples.

## Pins

HW pin PASS 2026-10-07. SPI3. Do not substitute another harness.

| Signal | GPIO |
|--------|------|
| SCK | 40 |
| MOSI | 14 |
| CS | 5 |
| DC | 6 |
| RST | 3 |
| SD_CS | 12 |

GPIO5 idles HIGH: the panel CS is active-low, and the Cardputer SD slot is enabled only while this pin is high. SD mounts go through `botcorps_lcd_quiesce` / `botcorps_lcd_resume_after_bus`. RGB565 / COLMOD is programmed at boot only.

MISO GPIO39 is the stock Cardputer SD data line. The panel does not use it.

**Cap:** LoRa1262 (NSS G5 / RST G3 / BUSY G6) and Hydra-on-G5 cannot coexist with this panel. This tree does not invent a shared mux.

**Never** the guicmg external-screen map (RST GPIO13, DC GPIO15).

## Display API

`BotcorpsDisplay` follows the active surface. It does not mean "draw everything on the external panel".

| Call | Dual, panel up | Otherwise |
|------|----------------|-----------|
| `botcorps_menu()` | Internal ST7789 | `M5Cardputer.Display` |
| `botcorps_content()` | External ILI9341 320×240 | `M5Cardputer.Display` |
| `botcorps_while_content(fn)` | Run `fn` on content, then restore the previous surface | Run `fn` on the only panel |
| `SCR_W` / `SCR_H` | 240×135 on menu, 320×240 on content | 240×135 |

## Layout

```
src/main.cpp                     boot into the rain screen; loop drives keys and rain
src/rain/botcorps_rain.*         external bot rain (sprite or cell fallback)
src/ui/botcorps_ui.*             mock screens, internal menu, keyboard
src/display/botcorps_display.*   menu/content split, quiesce, SCR_*
src/display/lgfx_ili9341.h       ILI9341 on SPI3 (dual builds only)
src/display/botcorps_pins.h      locked pin map
src/sd/botcorps_sd.*             shared-bus SD mount
```
