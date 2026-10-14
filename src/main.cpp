/*
 * Slice 1R2 boot. Hardware comes up on the rain screen. loop() reads
 * the keyboard and steps the rain. There is no color-fill size probe.
 *
 * Adv dual is the active BOTcorps line. This is not a CYD / COM13 port.
 * A successful flash is not a field PASS — nothing here is flashed.
 */

#include "display/botcorps_display.h"
#include "probe/botcorps_probe.h"
#include "ui/botcorps_band.h"
#include "ui/botcorps_ui.h"

#include <memory>
#include "utility/Keyboard/KeyboardReader/TCA8418.h"

#ifndef BOTCORPS_VERSION
#define BOTCORPS_VERSION "0.0.0-s0s1"
#endif

static void park_ir_led(void)
{
    /* Cardputer IR LED on GPIO44. HIGH keeps it off. */
    pinMode(44, OUTPUT);
    digitalWrite(44, HIGH);
}

static void park_sd_cs(void)
{
    /* After M5Cardputer.begin(), before EXT begin. GPIO12 only.
     * SD is not mounted at boot; a floating card can answer SPI3. */
    pinMode(BOTCORPS_PIN_SD_CS, OUTPUT);
    digitalWrite(BOTCORPS_PIN_SD_CS, HIGH);
}

void setup(void)
{
    park_ir_led();
    Serial.begin(115200);
    delay(100);
    botcorps_probe_before_m5();

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    /* Adv keyboard is TCA8418 on I2C. Forcing that reader keeps a
     * missed autodetect from taking G3–G7 as a GPIO matrix. Those
     * pins are the external panel on a dual build. */
    M5Cardputer.Keyboard.begin(std::make_unique<TCA8418KeyboardReader>());
    park_ir_led();

    /* Internal ST7789, landscape, keyboard at the bottom. Do not
     * change color depth — M5GFX already set RGB565 at boot. */
    M5Cardputer.Display.setRotation(1);

    const auto board = M5.getBoard();
    Serial.printf("\n[botcorps] %s\n", BOTCORPS_VERSION);
    Serial.printf("[botcorps] board=%d (%s)\n", (int)board,
                  board == m5::board_t::board_M5CardputerADV ? "Cardputer-Adv" :
                  board == m5::board_t::board_M5Cardputer    ? "Cardputer-K126" :
                                                               "other");
    botcorps_probe_after_m5();
    botcorps_probe_before_ext();
    park_sd_cs();

    const bool ext = botcorps_dual_begin();
    botcorps_lcd_resume_after_bus();
    botcorps_probe_after_ext(ext);
    if (!ext) {
        auto &menu = botcorps_menu();
        menu.fillScreen(0x0000);
        menu.setFont(&fonts::Font0);
        menu.setTextSize(2, 2);
        menu.setTextColor(0xFFFF, 0x0000);
        menu.setCursor(8, 40);
        menu.print("EXT fail");
    }

    Serial.printf("[botcorps] dual_flag=%d ext_ok=%d sd=0\n",
                  BOTCORPS_DUAL_SCREEN, ext ? 1 : 0);
    Serial.println("[botcorps] pins SCK40 MOSI14 CS5 DC6 RST3 SD_CS12 SPI3");
    Serial.println("[botcorps] cap: LoRa1262 NSS/RST/BUSY G5/G3/G6 and Hydra-on-G5 cannot share this panel");
    Serial.println("[botcorps] sd mount is on demand, not at boot");

    botcorps_band_touch();
    botcorps_frame_diag();
    botcorps_band_release();
    botcorps_probe_run();
    botcorps_band_rebuild();
    botcorps_ui_mark_dirty();

    /* Rain boot is the first real screen. loop() owns keys and rain.
     * The name bar is painted by the UI on the internal panel. */
    botcorps_ui_begin();
    botcorps_probe_screen("-", "rain");
    botcorps_ui_draw();
}

void loop(void)
{
    M5Cardputer.update();
    botcorps_ui_input();
    botcorps_ui_draw();
    delay(10);
}
