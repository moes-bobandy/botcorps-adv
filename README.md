# botcorps-adv

BOTcorps / CYBERbots Security — fresh Cardputer Adv dual-screen firmware.

Internal ST7789 **240×135** is the menu. External ILI9341 **320×240** is content. Both stay live. This tree is not a port of the CYD / COM13 `cyd28` firmware, and CYD bins do not belong on this board.

## Always / never

| Always | Never |
|--------|-------|
| Active BOTcorps line = fresh Cardputer Adv dual (INT ST7789 240×135 menu / EXT ILI9341 320×240 content), parallel product, same ideas | Treat CYD / COM13 BOTcorps as the active floor until Dirt unlocks it |

Slice 0 is the display split. Slice 1 is the hatch ENTER plate on the menu panel. The Ready Room and bot windows are later slices. A green `pio run` is not a field PASS. Do not flash a device from this tree until Dirt asks.

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

On a dual build the content panel is filled once with the Slice 0 field check (full 320×240, not a 240×135 corner) and then left alone. The menu panel shows the hatch until Enter. Enter opens the Slice 0 menu field check on the internal panel. A single-screen build shows the hatch on its only display, then the same menu field check.

## Hatch

`botcorps_enter` draws an olive-drab plate with flaked millscale in the margin, four bolts, a clean door, and khaki stencil caps: `BOTCORPS`, `HATCH`, `PRESS ENTER`, `TO OPEN`, `MENU`. Glyphs print left to right on the current menu rotation. It does not change the external panel's rotation, pins, or the SD quiesce path, and it has no rotate control.

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
src/main.cpp                     boot, hatch, then Slice 0 field check
src/enter/botcorps_enter.*       army-green millscale hatch on MENU
src/display/botcorps_display.*   menu/content split, quiesce, SCR_*
src/display/lgfx_ili9341.h       ILI9341 on SPI3 (dual builds only)
src/display/botcorps_pins.h      locked pin map
src/sd/botcorps_sd.*             shared-bus SD mount
```
