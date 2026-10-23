/* v1019m-host CYD glue: host-only definitions for the single-panel device
 * accessors the CYD paint path calls. Builds only in the CYD host render
 * (BOTCORPS_HOST_RENDER + BOTCORPS_CYD); never part of any device build.
 * host_shim.h declares these under BOTCORPS_CYD. */
#include "host_shim.h"

static lgfx::v1::LGFX_Device s_cyd_dev;
lgfx::v1::LGFX_Device &botcorps_disp(void) { return s_cyd_dev; }
bool botcorps_cyd_run_touch_cal(void) { return true; }  /* no panel on host */
int host_touch_x = 0, host_touch_y = 0, host_touch_down = 0;   /* cyd-term-fix: injected touch */
extern int host_push_ymax;
void botcorps_band_set_push_ymax(int y) { host_push_ymax = y < 0 ? 0 : (y > 240 ? 240 : y); }   /* cyd-term-fix: sim mirrors the device clamp */
