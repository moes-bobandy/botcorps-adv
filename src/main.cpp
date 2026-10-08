/*
 * Slice 1R boot. Hardware comes up, then the glass is the name bar
 * and bot rain. There is no color-fill size probe on either panel.
 *
 * Adv dual is the active BOTcorps line. This is not a CYD / COM13 port.
 * A successful flash is not a field PASS — nothing here is flashed.
 */

#include "display/botcorps_display.h"
#include "rain/botcorps_rain.h"
#include "sd/botcorps_sd.h"

#include <memory>
#include "utility/Keyboard/KeyboardReader/TCA8418.h"

#ifndef BOTCORPS_VERSION
#define BOTCORPS_VERSION "0.0.0-slice1r"
#endif

static void park_ir_led(void)
{
    /* Cardputer IR LED on GPIO44. HIGH keeps it off. */
    pinMode(44, OUTPUT);
    digitalWrite(44, HIGH);
}

void setup(void)
{
    park_ir_led();

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

    Serial.begin(115200);
    delay(100);
    const auto board = M5.getBoard();
    Serial.printf("\n[botcorps] %s\n", BOTCORPS_VERSION);
    Serial.printf("[botcorps] board=%d (%s)\n", (int)board,
                  board == m5::board_t::board_M5CardputerADV ? "Cardputer-Adv" :
                  board == m5::board_t::board_M5Cardputer    ? "Cardputer-K126" :
                                                               "other");

    const bool ext = botcorps_dual_begin();
    const bool sd = botcorps_sd_mount();
    /* Idempotent if the mount already resumed. Keeps GPIO5 high when
     * the panel came up and SD did not. */
    botcorps_lcd_resume_after_bus();

    Serial.printf("[botcorps] dual_flag=%d ext_ok=%d sd=%d\n",
                  BOTCORPS_DUAL_SCREEN, ext ? 1 : 0, sd ? 1 : 0);
    Serial.println("[botcorps] pins SCK40 MOSI14 CS5 DC6 RST3 SD_CS12 SPI3");
    Serial.println("[botcorps] cap: LoRa1262 NSS/RST/BUSY G5/G3/G6 and Hydra-on-G5 cannot share this panel");

    /* Name on the internal panel when it is a separate surface.
     * Rain owns the content panel, including the first frame. */
    botcorps_nav_draw();
    botcorps_rain_frame();
}

void loop(void)
{
    botcorps_rain_frame();
    M5Cardputer.update();
    delay(20);
}
