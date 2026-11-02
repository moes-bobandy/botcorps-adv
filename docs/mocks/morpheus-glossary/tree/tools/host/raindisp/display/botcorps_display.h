#pragma once
/* v1016i: minimal host stand-in for src/display/botcorps_display.h, so the REAL rain
 * TU compiles without M5Cardputer.h. lgfx method signatures match tools/host/host_shim.h;
 * the out-of-line bodies live in tools/host/rain_bridge.cpp (shared by both). */
#include <stdint.h>
#include <stddef.h>

namespace lgfx { inline namespace v1 {
namespace textdatum { enum textdatum_t { top_left = 0 }; }
class LovyanGFX {
public:
    template <class F> void setFont(const F *) {}
    void startWrite() {}
    void endWrite() {}
    void setClipRect(int, int, int, int);
    void clearClipRect();
    int width() const { return 320; }
    int height() const { return 240; }
    void setTextSize(float) {}
    void setTextSize(float, float) {}
    void setTextWrap(bool, bool) {}
    void setTextColor(uint8_t);
    void setCursor(int, int);
    void fillRect(int, int, int, int, uint8_t);
    void print(const char *);
};
class LGFX_Device : public LovyanGFX {};
}}

enum { BOTCORPS_SURFACE_MENU = 0, BOTCORPS_SURFACE_CONTENT = 1 };
static inline void botcorps_set_surface(int) {}
lgfx::v1::LovyanGFX &botcorps_content(void);
bool botcorps_dual_ok(void);
bool botcorps_band_direct(void);

/* time / serial / ESP that the rain TU references (normally from M5Cardputer.h) */
uint32_t host_millis(void);
static inline uint32_t millis(void) { return host_millis(); }
void host_set_millis(uint32_t t);
struct HostPrintD { void printf(const char *, ...) {} };
extern HostPrintD Serial;
struct HostESP { unsigned getFreeHeap(); unsigned getMaxAllocHeap(); };
extern HostESP ESP;
namespace fonts { struct Font { int id; }; extern const Font Font0; }
