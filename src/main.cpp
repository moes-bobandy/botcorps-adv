/*
 * Boot. Hatch ENTER on MENU, then the Ready Room. On a dual build the
 * external panel keeps the Slice 0 clear while the hatch is up, and
 * the seat card while the Ready Room is up.
 *
 * Adv dual is the active BOTcorps line. This is not a CYD / COM13 port.
 * A successful flash is not a field PASS — nothing here is flashed.
 */

#include "display/botcorps_display.h"
#include "enter/botcorps_enter.h"
#include "ready/botcorps_ready.h"
#include "sd/botcorps_sd.h"

#include <memory>
#include "utility/Keyboard/KeyboardReader/TCA8418.h"

#ifndef BOTCORPS_VERSION
#define BOTCORPS_VERSION "0.0.0-slice0"
#endif

/* Wiring colors, not product chrome. Menu navy, content green, yellow
 * border on the content view rectangle. If that border sits in a
 * 240×135 corner, SCR_* is wrong. */
static const uint16_t COL_CONTENT_BG = 0x0200;
static const uint16_t COL_CONTENT_FG = 0xFFFF;
static const uint16_t COL_CONTENT_EDGE = 0xFFE0;

static void paint_label(lgfx::LGFX_Device &d, uint16_t fg, uint16_t bg,
                        const char *name, int w, int h)
{
    d.setTextWrap(false, false);
    d.setTextSize(1);
    d.setTextColor(fg, bg);
    d.setCursor(8, 8);
    d.print(name);
    d.setCursor(8, 20);
    d.print(w);
    d.print("x");
    d.print(h);
}

/* Runs under botcorps_while_content, so BotcorpsDisplay and SCR_* are
 * the content surface. fillScreen covers the panel; the yellow rect
 * is the active view and must land on the outer edge (320×240). */
static void paint_content(void)
{
    auto &d = BotcorpsDisplay;
    const int w = SCR_W;
    const int h = SCR_H;
    d.clearClipRect();
    d.fillScreen(COL_CONTENT_BG);
    if (w > 1 && h > 1) {
        d.drawRect(0, 0, w, h, COL_CONTENT_EDGE);
    }
    paint_label(d, COL_CONTENT_FG, COL_CONTENT_BG, "CONTENT", w, h);
    Serial.printf("[botcorps] content paint SCR %dx%d device %dx%d\n",
                  w, h, (int)d.width(), (int)d.height());
    if (botcorps_dual_ok() && (w != (int)d.width() || h != (int)d.height())) {
        Serial.println("[botcorps] SCR does not match the content panel");
    }
}

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
    Serial.printf("[botcorps] INT %dx%d\n",
                  (int)M5Cardputer.Display.width(),
                  (int)M5Cardputer.Display.height());

    const bool ext = botcorps_dual_begin();
    const bool sd = botcorps_sd_mount();
    /* Idempotent if the mount already resumed. Keeps GPIO5 high when
     * the panel came up and SD did not. */
    botcorps_lcd_resume_after_bus();

    Serial.printf("[botcorps] dual_flag=%d ext_ok=%d sd=%d surface=%d\n",
                  BOTCORPS_DUAL_SCREEN, ext ? 1 : 0, sd ? 1 : 0,
                  (int)botcorps_surface());
    Serial.printf("[botcorps] menu SCR %dx%d\n",
                  botcorps_view_w(), botcorps_view_h());
    Serial.println("[botcorps] pins SCK40 MOSI14 CS5 DC6 RST3 SD_CS12 SPI3");
    Serial.println("[botcorps] cap: LoRa1262 NSS/RST/BUSY G5/G3/G6 and Hydra-on-G5 cannot share this panel");

    while (true) {
        /* Hatch time: EXT stays on the Slice 0 clear. One paint, not a loop. */
        if (botcorps_dual_ok()) {
            botcorps_while_content(paint_content);
        }
        botcorps_enter_run();
        botcorps_ready_run();
    }
}

void loop(void)
{
    M5Cardputer.update();
    delay(50);
}
