#pragma once
/* Host harness shim: a framebuffer-backed LovyanGFX that runs the REAL rain. */
#include <stdint.h>
#include <string.h>

extern uint8_t g_fb[320*240];
extern int g_clip_x, g_clip_y, g_clip_w, g_clip_h;

namespace lgfx { inline namespace v1 {
namespace textdatum { enum textdatum_t { top_left=0 }; }
class LovyanGFX {
public:
    int cx=0, cy=0; uint8_t fg=255;
    template<class F> void setFont(const F*) {}
    void startWrite() {} void endWrite() {}
    void setClipRect(int x,int y,int w,int h){ g_clip_x=x; g_clip_y=y; g_clip_w=w; g_clip_h=h; }
    void clearClipRect(){ g_clip_x=0; g_clip_y=0; g_clip_w=320; g_clip_h=240; }
    int width() const { return 320; } int height() const { return 240; }
    void setTextSize(float){} void setTextSize(float,float){}
    void setTextWrap(bool,bool){}
    void setTextColor(uint8_t c){ fg=c; }
    void setCursor(int x,int y){ cx=x; cy=y; }
    void fillRect(int x,int y,int w,int h,uint8_t c);
    void print(const char* s);
};
class LGFX_Device : public LovyanGFX {};
}}

enum { BOTCORPS_SURFACE_MENU=0, BOTCORPS_SURFACE_CONTENT=1 };
static inline void botcorps_set_surface(int){}
lgfx::v1::LGFX_Device& botcorps_content(void);
lgfx::v1::LGFX_Device& botcorps_menu(void);
lgfx::v1::LGFX_Device& botcorps_disp(void);
bool botcorps_dual_ok(void);
bool botcorps_band_direct(void);

/* Arduino-ish globals the rain TU needs (normally from M5Cardputer.h). */
extern "C" { }
uint32_t millis(void);
struct HostPrintD { void printf(const char*,...){} };
extern HostPrintD Serial;
struct HostESPD { unsigned getFreeHeap(); unsigned getMaxAllocHeap(); };
extern HostESPD ESP;
namespace fonts { struct Font{int id;}; extern const Font Font0; }
