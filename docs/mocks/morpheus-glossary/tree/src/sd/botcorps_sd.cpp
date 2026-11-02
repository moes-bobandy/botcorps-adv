/*
 * One SD mount path. Features in later slices call botcorps_sd_mount()
 * instead of SD.begin() so the shared-bus policy cannot be skipped.
 */
#include "sd/botcorps_sd.h"

#ifdef BOTCORPS_CYD

/* CYD has no Cardputer SD ribbon; the mount stays a stub there. */
bool botcorps_sd_mounted(void) { return false; }
bool botcorps_sd_mount(void) { return false; }

#else
#include "display/botcorps_display.h"

#include <SD.h>
#include <SPI.h>

/* 20 MHz is reliable on the Cardputer SD ribbon. Slower retries below. */
static const int BOTCORPS_SD_HZ = 20000000;

/* Arduino HSPI on ESP32-S3 is peripheral index 1, which clocks SPI3
 * (PERIPH_SPI3). LovyanGFX uses IDF SPI3_HOST (numeric 2) for the same
 * peripheral. Do not pass SPI3_HOST into SPIClass, and do not move SD
 * onto FSPI — that is the internal ST7789. */
static SPIClass s_sd_spi(HSPI);
static bool s_mounted = false;

bool botcorps_sd_mounted(void)
{
    return s_mounted;
}

static void sd_pins_idle(void)
{
    pinMode(BOTCORPS_PIN_MISO, INPUT_PULLUP);
    pinMode(BOTCORPS_PIN_MOSI, INPUT_PULLUP);
    pinMode(BOTCORPS_PIN_SCK, INPUT_PULLUP);
    pinMode(BOTCORPS_PIN_SD_CS, INPUT_PULLUP);
    pinMode(BOTCORPS_PIN_SD_CS, OUTPUT);
    digitalWrite(BOTCORPS_PIN_SD_CS, HIGH);
}

static bool try_mount(int hz, const char *tag)
{
    /* Dual: deselect the ILI9341 before this bus is reused.
     * Single-screen: quiesce is a no-op; GPIO5 is idled below. */
    botcorps_lcd_quiesce();

#if !BOTCORPS_DUAL_SCREEN
    /* No panel owns GPIO5. INPUT_PULLUP idles it high (SD enable)
     * without fighting a Cap hat's NSS or GDO0. Dual builds do not
     * take this branch — the LCD CS driver holds GPIO5. */
    pinMode(BOTCORPS_PIN_LCD_CS, INPUT_PULLUP);
#endif

    SD.end();
    s_sd_spi.end();
    sd_pins_idle();
    delay(10);

    s_sd_spi.begin(BOTCORPS_PIN_SCK, BOTCORPS_PIN_MISO,
                   BOTCORPS_PIN_MOSI, BOTCORPS_PIN_SD_CS);
    const bool ok = SD.begin(BOTCORPS_PIN_SD_CS, s_sd_spi, hz, "/sd", 2, false);
    Serial.printf("[botcorps] sd %-8s @ %d Hz -> %s\n",
                  tag, hz, ok ? "OK" : "absent");

    botcorps_lcd_resume_after_bus();
    return ok;
}

bool botcorps_sd_mount(void)
{
    if (s_mounted) return true;

    if (try_mount(BOTCORPS_SD_HZ, "fast")) {
        s_mounted = true;
        return true;
    }
    if (try_mount(10000000, "half")) {
        s_mounted = true;
        return true;
    }
    if (try_mount(4000000, "slow")) {
        s_mounted = true;
        return true;
    }
    return false;
}
#endif /* BOTCORPS_CYD */
