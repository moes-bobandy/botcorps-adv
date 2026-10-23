#pragma once

#include <stdint.h>
#include <stdbool.h>

#include "ui/botcorps_frame_cfg.h"
#include "ui/botcorps_pal.h"

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* Row hashes and the RGB332 to RGB565 table. Volatile-touched so nm keeps them. */
extern uint32_t botcorps_t0_row_hash[240];
extern uint16_t botcorps_t0_lut565[256];

struct Gfx {
    lgfx::LovyanGFX *p;
    int ox;
    int oy;
    Gfx(lgfx::LovyanGFX &d);
    lgfx::LovyanGFX *raw(void) const { return p; }
    void fillRect(int x, int y, int w, int h, uint8_t c);
    /* Plain uint16_t is RGB565. A bare int or uint32_t would be read as RGB888. */
    void fillRect(int x, int y, int w, int h, uint16_t c);
    void drawRect(int x, int y, int w, int h, uint8_t c);
    void fillCircle(int x, int y, int r, uint8_t c);
    void drawCircle(int x, int y, int r, uint8_t c);
    void drawLine(int x0, int y0, int x1, int y1, uint8_t c);
    void fillTriangle(int x0, int y0, int x1, int y1, int x2, int y2, uint8_t c);
    void drawFastHLine(int x, int y, int w, uint8_t c);
    void drawFastVLine(int x, int y, int h, uint8_t c);
    void fillRoundRect(int x, int y, int w, int h, int r, uint8_t c);
    void fillScreen(uint8_t c);
    void setClipRect(int x, int y, int w, int h);
    void clearClipRect(void);
    void setTextWrap(bool x, bool y);
    template <class F>
    void setFont(const F *f) { p->setFont(f); }
    void setTextSize(float s);
    void setTextSize(float sx, float sy);
    void setTextColor(uint8_t c);
    void setTextColor(uint16_t c);
    void setTextColor(uint8_t fg, uint8_t bg);
    void setTextDatum(int dat);
    void drawString(const char *s, int x, int y);
    void setCursor(int x, int y);
    void print(const char *s);
    int getCursorX(void);
    void startWrite(void);
    void endWrite(void);
};

void botcorps_band_touch(void);
/* Install or clear the 13 sheet colours in the live lut. Full repaint only. */
void botcorps_lut_term(bool on);
/* live = false, sprite deleted, no write left open. Safe before radios. */
void botcorps_band_release(void);
/* After radios. One failure sticks for the session (direct draw, no retry loop). */
bool botcorps_band_rebuild(void);
bool botcorps_band_direct(void);
bool botcorps_band_live(void);
int botcorps_band_oy(void);
int botcorps_band_ox(void);
void botcorps_band_set_oy(int y);
void botcorps_band_set_ox(int x);
/* Full-slice redraw used when a patch targets a slice the sprite is not holding. */
void botcorps_band_set_recompose(void (*draw)(Gfx));
int botcorps_band_bytes(void);
uint32_t botcorps_band_largest(void);
/* Try a 153600-byte frame, log the free block, and release it. */
void botcorps_frame_diag(void);
/* Redraw one world rectangle into the live band and push the changed rows. */
void botcorps_band_patch(void (*draw)(Gfx), int x, int y, int w, int h);
/* v1017c: in-place dirty rect. draw MUST paint every pixel of the rect opaquely. Draws the rect
 * clipped into the sprite at its slice offset and pushes ONLY the rect - never rebuilds (recomposes)
 * the other slice, never pushes a full row. Must not cross y=120 (caller splits). */
void botcorps_band_patch_rect(void (*draw)(Gfx), int x, int y, int w, int h);
const char *botcorps_band_mode_name(void);
lgfx::LovyanGFX *botcorps_band_sprite(void);
/* Draw fn paints world coordinates. The band shifts them. Pushes changed rows. */
void botcorps_band_present(void (*draw)(Gfx));

/* Normal-order RGB565 for a named index. Swaps the pre-swapped lut word back. */
uint16_t botcorps_pal_native(uint8_t idx);
/* Mock RGB565 for an index, from the const table. Not the runtime lut. */
uint16_t botcorps_rgb565(uint8_t idx);
/* Exact reserved slot, else the 4096-entry table. Does not scan 256. */
uint8_t botcorps_pal_fit(uint16_t native);
/* 4-step ramp reused across trail ages 0..13. Ages past that are field. */
uint8_t botcorps_pal_ramp(int ink, int age);
/* True when native is a ramp source. Writes the step index for this age. */
bool botcorps_pal_ramp_for(uint16_t native, int age, uint8_t *out);
/* Per-pixel RGB invert of a world rect in the live 8-bit band sprite. */
void botcorps_band_invert_rect(int x, int y, int w, int h);
/* NEW CUT (Look A): scale each pixel of a world rect to k/100 of its RGB in the resident slice.
 * False when no band sprite is live (direct mode); the caller then fills. */
bool botcorps_band_dim_rect(int x, int y, int w, int h, int k);
/* Index into the band sprite, native RGB565 on any other surface. */
void botcorps_pal_rect(lgfx::LovyanGFX &dst, int x, int y, int w, int h, uint8_t idx);
void botcorps_pal_pixel(lgfx::LovyanGFX &dst, int x, int y, uint8_t idx);
void botcorps_pal_text(lgfx::LovyanGFX &dst, uint8_t idx);
/* Hub rain only. While set, pal_rect / pal_pixel / pal_text use the fade table. */
void botcorps_pal_hub_set(bool on);
/* Fade-table index for an ink. Does not depend on the hub flag. */
uint8_t botcorps_pal_hub_of(uint8_t idx);
