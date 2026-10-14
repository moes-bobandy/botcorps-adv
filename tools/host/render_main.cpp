#include "host_shim.h"
#include "ui/botcorps_pal.h"
#include "ui/botcorps_pal_tables.inc"

#include <stdio.h>
#include <stdint.h>

static void rgb_of(uint8_t idx, uint8_t *r, uint8_t *g, uint8_t *b)
{
    const uint16_t c = k_exact_nat[idx];
    const int r5 = (c >> 11) & 31;
    const int g6 = (c >> 5) & 63;
    const int b5 = c & 31;
    *r = (uint8_t)((r5 << 3) | (r5 >> 2));
    *g = (uint8_t)((g6 << 2) | (g6 >> 4));
    *b = (uint8_t)((b5 << 3) | (b5 >> 2));
    if (idx == 0) { *r = *g = *b = 0; }
    if (idx == 255) { *r = *g = *b = 255; }
}

static void write_raw(const char *path, const uint8_t *idx)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    for (int i = 0; i < 320 * 240; ++i) {
        uint8_t r, g, b;
        rgb_of(idx[i], &r, &g, &b);
        fputc(r, f);
        fputc(g, f);
        fputc(b, f);
    }
    fclose(f);
}

int main(void)
{
    static uint8_t term[320 * 240];
    static uint8_t hunt[320 * 240];
    static uint8_t idle[320 * 240];
    botcorps_host_terminal(term);
    botcorps_host_idle(idle);
    botcorps_host_hunt(hunt);
    write_raw("/tmp/term.rgb", term);
    write_raw("/tmp/idle.rgb", idle);
    write_raw("/tmp/hunt.rgb", hunt);
    return 0;
}
