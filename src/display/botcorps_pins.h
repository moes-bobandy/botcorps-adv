#pragma once

/*
 * External ILI9341 + shared SD bus on Cardputer Adv.
 * HW pin PASS 2026-10-07. Do not invent another pinout.
 *
 *   SCK 40   MOSI 14   CS 5   DC 6   RST 3   SD_CS 12
 *   Bus: SPI3 (Arduino HSPI index clocks the SPI3 peripheral).
 *   MISO GPIO39 is the stock Cardputer SD data-in. The panel does
 *   not use it (write-only, pin_miso = -1).
 *
 * GPIO5 is the panel CS (active-low) and must idle HIGH. That
 * deselects the LCD and enables the Cardputer SD slot.
 *
 * Cap conflict (document only — no shared pinmux, no alternate
 * wiring): LoRa1262 uses NSS G5 / RST G3 / BUSY G6, and Hydra-on-G5
 * uses the same CS. Those hats cannot be attached with this panel.
 *
 * Never the guicmg cardputer_adv_external_screen map (RST GPIO13,
 * DC GPIO15). That is a different harness.
 */

#define BOTCORPS_PIN_SCK     40
#define BOTCORPS_PIN_MOSI    14
#define BOTCORPS_PIN_MISO    39
#define BOTCORPS_PIN_LCD_CS  5
#define BOTCORPS_PIN_LCD_DC  6
#define BOTCORPS_PIN_LCD_RST 3
#define BOTCORPS_PIN_SD_CS   12

static_assert(BOTCORPS_PIN_SCK == 40, "HW pin PASS: SCK is GPIO40");
static_assert(BOTCORPS_PIN_MOSI == 14, "HW pin PASS: MOSI is GPIO14");
static_assert(BOTCORPS_PIN_LCD_CS == 5, "HW pin PASS: CS is GPIO5");
static_assert(BOTCORPS_PIN_LCD_DC == 6, "HW pin PASS: DC is GPIO6");
static_assert(BOTCORPS_PIN_LCD_RST == 3, "HW pin PASS: RST is GPIO3");
static_assert(BOTCORPS_PIN_SD_CS == 12, "HW pin PASS: SD_CS is GPIO12");
static_assert(BOTCORPS_PIN_LCD_RST != 13, "never guicmg RST GPIO13");
static_assert(BOTCORPS_PIN_LCD_DC != 15, "never guicmg DC GPIO15");
