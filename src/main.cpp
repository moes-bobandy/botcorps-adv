/*
 * BOTcorps boot. CYD single-screen or Adv dual.
 */

#include "display/botcorps_display.h"
#include "probe/botcorps_probe.h"
#include "ui/botcorps_band.h"
#include "ui/botcorps_ui.h"

#ifndef BOTCORPS_VERSION
#define BOTCORPS_VERSION "0.0.0"
#endif

#ifdef BOTCORPS_CYD

void setup(void)
{
    Serial.begin(115200);
    delay(100);
    Serial.printf("\n[botcorps] %s CYD\n", BOTCORPS_VERSION);

    const bool ok = botcorps_dual_begin();
    Serial.printf("[botcorps] display_ok=%d\n", ok ? 1 : 0);
    if (!ok) {
        Serial.println("[botcorps] display fail");
        while (1) delay(1000);
    }

    botcorps_band_touch();
    botcorps_frame_diag();
    botcorps_band_release();
    botcorps_probe_run();
    botcorps_band_rebuild();
    botcorps_id_strip_alloc();
    botcorps_ui_mark_dirty();

    botcorps_ui_begin();
    botcorps_probe_screen("-", "rain");
    botcorps_ui_draw();
}

void loop(void)
{
    botcorps_ui_input();
    botcorps_ui_draw();
    delay(10);
}

#else

#include <memory>
#include "utility/Keyboard/KeyboardReader/TCA8418.h"

static void park_ir_led(void)
{
    pinMode(44, OUTPUT);
    digitalWrite(44, HIGH);
}

static void park_sd_cs(void)
{
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
    M5Cardputer.Keyboard.begin(std::make_unique<TCA8418KeyboardReader>());
    park_ir_led();

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

    botcorps_band_touch();
    botcorps_frame_diag();
    botcorps_band_release();
    botcorps_probe_run();
    botcorps_band_rebuild();
    botcorps_id_strip_alloc();
    botcorps_ui_mark_dirty();

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

#endif
