/* CYD-only host render modes. Built only by tools/host/build_cyd.sh
 * (BOTCORPS_HOST_RENDER + BOTCORPS_CYD). Returns -1 when argv is not a CYD mode. */
#include "host_shim.h"
#include <string.h>
#include <stdlib.h>

extern void write_raw(const char *path, const uint8_t *idx, int term);
extern void botcorps_host_cyd_osk(uint8_t *buf);
extern void host_rain_init(int mode);
extern void host_set_millis(uint32_t t);
extern void botcorps_host_set_color(int c);

static uint8_t s_cyd_buf[320 * 240];

int cyd_render_main(int argc, char **argv)
{
    /* rend osk <out.rgb> — Terminal with the [abc] on-screen keyboard open. */
    if (argc > 2 && strcmp(argv[1], "osk") == 0) {
        host_rain_init(0);
        host_set_millis(getenv("MS") ? (uint32_t)atoi(getenv("MS")) : 9000u);
        botcorps_host_set_color(0);
        memset(s_cyd_buf, 0, sizeof s_cyd_buf);
        botcorps_host_cyd_osk(s_cyd_buf);
        write_raw(argv[2], s_cyd_buf, 0);
        return 0;
    }
    return -1;
}
