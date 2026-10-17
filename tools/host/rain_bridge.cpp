/* v1016i: host bridge so the REAL src/rain/botcorps_rain.cpp draws into host_frame.
 * Ports the proven tools/rainhost shim approach into the main render: a framebuffer-
 * backed LovyanGFX plus the real palette fit/ramp/hub helpers. */
#include "host_shim.h"
#define PROGMEM
#include "lgfx/Fonts/glcdfont.h"
#include <stdint.h>
#include <string.h>

#include "ui/botcorps_pal_tables.inc"
#include "ui/botcorps_pal_fit.inc"
#include "ui/botcorps_pal_hub.inc"
#ifndef BOTCORPS_RAIN_MIX_FIELD
#define BOTCORPS_RAIN_MIX_FIELD 0
#endif
void botcorps_rain_set_mode(int);
void botcorps_rain_set_mix(int);
bool botcorps_rain_step(void);

extern uint8_t *host_frame;

/* ---- clip + per-panel text state (rain is the only user) ---- */
static int rb_clipx = 0, rb_clipy = 0, rb_clipw = 320, rb_cliph = 240;
static int rb_cx = 0, rb_cy = 0;
static uint8_t rb_fg = 255;
static inline bool rb_in(int x, int y)
{ return x >= rb_clipx && x < rb_clipx + rb_clipw && y >= rb_clipy && y < rb_clipy + rb_cliph; }
static inline void rb_px(int x, int y, uint8_t c)
{ if (host_frame && (unsigned)x < 320 && (unsigned)y < 240 && rb_in(x, y)) host_frame[y * 320 + x] = c; }

void lgfx::v1::LovyanGFX::setClipRect(int x, int y, int w, int h)
{ rb_clipx = x; rb_clipy = y; rb_clipw = w; rb_cliph = h; }
void lgfx::v1::LovyanGFX::clearClipRect(void)
{ rb_clipx = 0; rb_clipy = 0; rb_clipw = 320; rb_cliph = 240; }
void lgfx::v1::LovyanGFX::fillRect(int x, int y, int w, int h, uint8_t c)
{ for (int j = 0; j < h; ++j) for (int i = 0; i < w; ++i) rb_px(x + i, y + j, c); }
void lgfx::v1::LovyanGFX::setTextColor(uint8_t c) { rb_fg = c; }
void lgfx::v1::LovyanGFX::setCursor(int x, int y) { rb_cx = x; rb_cy = y; }
void lgfx::v1::LovyanGFX::print(const char *s)
{
    if (!s) return;
    for (int k = 0; s[k]; ++k) {
        unsigned u = (unsigned char)s[k];
        if (u < 256) { const uint8_t *bits = font + u * 5;
            for (int col = 0; col < 5; ++col) { uint8_t line = bits[col];
                for (int row = 0; row < 8; ++row) if ((line >> row) & 1) rb_px(rb_cx + col, rb_cy + row, rb_fg); } }
        rb_cx += 6;
    }
}

/* ---- the host panel the UI's Gfx::raw() points at ---- */
static lgfx::v1::LGFX_Device s_host_panel;
lgfx::v1::LovyanGFX *host_rain_panel(void) { return &s_host_panel; }

/* ---- palette helpers (REAL, same math as tools/rainhost) ---- */
static uint16_t rb_pack332(uint8_t i)
{ int r3=(i>>5)&7,g3=(i>>2)&7,b2=i&3; int r8=(((r3<<3)+r3)<<2)+(r3>>1),g8=(((g3<<3)+g3)<<2)+(g3>>1),b8=b2*0x55;
  return (uint16_t)(((r8>>3)<<11)|((g8>>2)<<5)|(b8>>3)); }
static uint16_t rb_word(uint8_t idx){ if (k_is_res[idx]) return k_exact_nat[idx]; return rb_pack332(idx); }

uint8_t botcorps_t0_rain_col[56][8];
uint32_t botcorps_t0_row_hash[240];
uint16_t botcorps_t0_lut565[256];

uint16_t botcorps_pal_native(uint8_t idx){ return __builtin_bswap16(botcorps_t0_lut565[idx]); }
uint8_t botcorps_pal_ramp(int ink, int age)
{ if(ink<0)ink=0; if(ink>=k_ramp_n)ink=k_ramp_n-1; if(age<0)age=0; if(age>13)age=13;
  const int step=age*(PAL_RAMP_N-1)/13; return k_ramp_idx[ink][step]; }
bool botcorps_pal_ramp_for(uint16_t native, int age, uint8_t *out)
{ if(!out) return false; for(int i=0;i<k_ramp_n;++i){ if(k_ramp_src[i]!=native) continue; *out=botcorps_pal_ramp(i,age); return true; } return false; }
uint8_t botcorps_pal_fit(uint16_t native)
{ for(int n=0;n<k_own_n;++n){ const uint8_t i=k_own_idx[n]; if(k_exact_nat[i]==native) return i; }
  const unsigned r=(native>>11)&31,g=(native>>5)&63,b=native&31; const unsigned key=((r>>1)<<8)|((g>>2)<<4)|(b>>1); return k_fit4[key]; }

static bool s_hub_on = false;
void botcorps_pal_hub_set(bool on){ s_hub_on = on; }
static uint8_t hub_map(uint8_t idx){ return s_hub_on ? k_hub_fade[idx] : idx; }
void botcorps_pal_rect(lgfx::v1::LovyanGFX &d,int x,int y,int w,int h,uint8_t idx){ d.fillRect(x,y,w,h,hub_map(idx)); }
void botcorps_pal_pixel(lgfx::v1::LovyanGFX &d,int x,int y,uint8_t idx){ d.fillRect(x,y,1,1,hub_map(idx)); }
void botcorps_pal_text(lgfx::v1::LovyanGFX &d,uint8_t idx){ d.setTextColor(hub_map(idx)); }

/* bot colours (REAL, same as src/ui/botcorps_bots.cpp; bots.cpp isn't linked here) */
extern const uint16_t BOTCORPS_BOT_COLOR[12];
const uint16_t BOTCORPS_BOT_COLOR[12] = {
 0x26E8,0x269C,0x331F,0xB21F,0xF586,0xFD20,0x07FF,0xF81F,0x07E0,0xFFE0,0xFC9F,0x9CFF };

/* warm up + per-frame step, called from render_main */
void host_rain_init(int mode)
{
    for (int i = 0; i < 256; ++i) botcorps_t0_lut565[i] = __builtin_bswap16(rb_word((uint8_t)i));
    botcorps_rain_set_mode(mode);
    botcorps_rain_set_mix(BOTCORPS_RAIN_MIX_FIELD);
    botcorps_rain_set_draw(false);
    uint32_t t = 0;
    for (int w = 0; w < 48; ++w) { t += 90; host_set_millis(t); botcorps_rain_step(); }
}

