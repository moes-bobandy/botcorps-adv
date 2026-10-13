#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ui/botcorps_frame_cfg.h"

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* Row hashes and the RGB332 to RGB565 table. Volatile-touched so nm keeps them. */
extern uint32_t botcorps_t0_row_hash[240];
extern uint16_t botcorps_t0_lut565[256];

struct Gfx {
    lgfx::LovyanGFX *p;
    int oy;
    Gfx(lgfx::LovyanGFX &d);
    lgfx::LovyanGFX *raw(void) const { return p; }
    void fillRect(int x, int y, int w, int h, uint16_t c);
    void drawRect(int x, int y, int w, int h, uint16_t c);
    void fillCircle(int x, int y, int r, uint16_t c);
    void drawCircle(int x, int y, int r, uint16_t c);
    void drawLine(int x0, int y0, int x1, int y1, uint16_t c);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint16_t c);
    void drawFastHLine(int x, int y, int w, uint16_t c);
    void drawFastVLine(int x, int y, int h, uint16_t c);
    void fillRoundRect(int x, int y, int w, int h, int r, uint16_t c);
    void fillScreen(uint16_t c);
    void setClipRect(int x, int y, int w, int h);
    void clearClipRect(void);
    void setTextWrap(bool x, bool y);
    template <class F>
    void setFont(const F *f) { p->setFont(f); }
    void setTextSize(float s);
    void setTextSize(float sx, float sy);
    void setTextColor(uint16_t c);
    void setTextColor(uint16_t fg, uint16_t bg);
    void setTextDatum(int dat);
    void drawString(const char *s, int x, int y);
    void setCursor(int x, int y);
    void print(const char *s);
    int getCursorX(void);
    void startWrite(void);
    void endWrite(void);
};

void botcorps_band_touch(void);
/* live = false, sprite deleted, no write left open. Safe before radios. */
void botcorps_band_release(void);
/* After radios. One failure sticks for the session (direct draw, no retry loop). */
bool botcorps_band_rebuild(void);
bool botcorps_band_direct(void);
bool botcorps_band_live(void);
int botcorps_band_oy(void);
void botcorps_band_set_oy(int y);
int botcorps_band_bytes(void);
uint32_t botcorps_band_largest(void);
const char *botcorps_band_mode_name(void);
lgfx::LovyanGFX *botcorps_band_sprite(void);
/* Draw fn paints world coordinates. The band shifts them. Pushes changed rows. */
void botcorps_band_present(void (*draw)(Gfx));
