#include "host_shim.h"
#include "ui/botcorps_pal.h"
#include "ui/botcorps_pal_tables.inc"
#include "ui/botcorps_term_ov.h"

#include <stdio.h>
#include <stdint.h>

static uint16_t pack332(uint8_t i)
{
    const int r3 = (i >> 5) & 7;
    const int g3 = (i >> 2) & 7;
    const int b2 = i & 3;
    const int r8 = (((r3 << 3) + r3) << 2) + (r3 >> 1);
    const int g8 = (((g3 << 3) + g3) << 2) + (g3 >> 1);
    const int b8 = b2 * 0x55;
    const int r5 = r8 >> 3;
    const int g6 = g8 >> 2;
    const int b5 = b8 >> 3;
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static uint16_t word_of(uint8_t idx, int term)
{
    /* Same native word push_slice byte-swaps onto the EXT panel. */
    const uint16_t ov = botcorps_term_ov_native(idx, term);
    if (ov) return ov;
    if (k_is_res[idx]) return k_exact_nat[idx];
    return pack332(idx);
}

static void rgb_of(uint8_t idx, int term, uint8_t *r, uint8_t *g, uint8_t *b)
{
    const uint16_t c = word_of(idx, term);
    const int r5 = (c >> 11) & 31;
    const int g6 = (c >> 5) & 63;
    const int b5 = c & 31;
    *r = (uint8_t)((r5 << 3) | (r5 >> 2));
    *g = (uint8_t)((g6 << 2) | (g6 >> 4));
    *b = (uint8_t)((b5 << 3) | (b5 >> 2));
    if (!term && idx == 0) { *r = *g = *b = 0; }
    if (!term && idx == 255) { *r = *g = *b = 255; }
}

static void write_raw(const char *path, const uint8_t *idx, int term)
{
    FILE *f = fopen(path, "wb");
    if (!f) return;
    for (int i = 0; i < 320 * 240; ++i) {
        uint8_t r, g, b;
        rgb_of(idx[i], term, &r, &g, &b);
        fputc(r, f);
        fputc(g, f);
        fputc(b, f);
    }
    fclose(f);
}

static void hist_term(const uint8_t *idx)
{
    int hit[13];
    for (int n = 0; n < 13; ++n) hit[n] = 0;
    int other = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        int found = 0;
        for (int n = 0; n < botcorps_term_ov_n; ++n)
            if (idx[i] == botcorps_term_ov_i[n]) { hit[n]++; found = 1; break; }
        if (!found && idx[i] >= 230 && idx[i] <= 248) other++;
    }
    FILE *f = fopen("/tmp/term_idx_hist.txt", "w");
    int sum = 0;
    for (int n = 0; n < 13; ++n) {
        printf("code %u explicit %d fit 0\n", botcorps_term_ov_i[n], hit[n]);
        if (f) fprintf(f, "code %u explicit %d fit 0\n", botcorps_term_ov_i[n], hit[n]);
        sum += hit[n];
    }
    printf("other 230-248 pixels %d\n", other);
    printf("sheet-code pixels %d\n", sum);
    if (f) {
        fprintf(f, "other 230-248 pixels %d\n", other);
        fprintf(f, "sheet-code pixels %d\n", sum);
        fclose(f);
    }
}

int main(void)
{
    static uint8_t term[320 * 240];
    static uint8_t hunt[320 * 240];
    static uint8_t idle[320 * 240];
    static uint8_t bus[320 * 240];
    static uint8_t amber[320 * 240];
    static uint8_t bots[320 * 240];
    host_fit_calls = 0;
    botcorps_host_terminal(term);
    printf("sheet pal_fit calls %d\n", host_fit_calls);
    printf("cmds sheet swap %d\n", botcorps_host_cmds_sheet());
    printf("int override %d span %d\n", botcorps_host_int_span(), host_rgb_span);
    printf("int full %d strip %d partial %d\n", host_int_full, host_int_strip, host_int_part);
    write_raw("/tmp/term.rgb", term, host_term_lut);
    hist_term(term);
    botcorps_host_idle(idle);
    write_raw("/tmp/idle.rgb", idle, host_term_lut);
    botcorps_host_hunt(hunt);
    write_raw("/tmp/hunt.rgb", hunt, host_term_lut);
    botcorps_host_bus(bus);
    write_raw("/tmp/bus.rgb", bus, host_term_lut);
    botcorps_host_amber(amber);
    write_raw("/tmp/amber.rgb", amber, host_term_lut);
    int amber_237 = 0;
    int amber_123 = 0;
    int amber_213 = 0;
    int amber_229 = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        if (amber[i] == 237) amber_237++;
        if (amber[i] == 123) amber_123++;
        if (amber[i] == 213) amber_213++;
        if (amber[i] == 229) amber_229++;
    }
    printf("amber code237 %d pal_cyan123 %d pal_amber213 %d name229 %d\n",
           amber_237, amber_123, amber_213, amber_229);
    botcorps_host_botscan(bots);
    write_raw("/tmp/bots.rgb", bots, host_term_lut);
    static uint8_t onebot[320 * 240];
    for (int b = 0; b < 12; ++b) {
        char path[64];
        snprintf(path, sizeof(path), "/tmp/bot-%02d.rgb", b);
        botcorps_host_botscan_n(onebot, b);
        write_raw(path, onebot, host_term_lut);
    }
    /* Sheet body is y=104..222, x=14..306. Glow is 4px, so the sheet
     * region counted as inside is x=8..312, y=98..228. */
    const int frames_n = 6;
    const char *names[] = { "term", "idle", "amber", "hunt", "gibson", "botscan" };
    const uint8_t *frames[] = { term, idle, amber, hunt, bus, bots };
    const int sheet_on[] = { 1, 0, 1, 0, 0, 0 };
    for (int f = 0; f < frames_n; ++f) {
        int out = 0;
        int by_code[17];
        for (int n = 0; n < 17; ++n) by_code[n] = 0;
        int minx = 320, miny = 240, maxx = 0, maxy = 0;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                const int inside = sheet_on[f] && x >= 8 && x < 312 && y >= 98 && y < 228;
                if (inside) continue;
                const uint8_t c = frames[f][y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n) {
                    if (c == botcorps_term_ov_i[n]) {
                        out++;
                        by_code[n]++;
                        if (x < minx) minx = x;
                        if (y < miny) miny = y;
                        if (x > maxx) maxx = x;
                        if (y > maxy) maxy = y;
                        break;
                    }
                }
            }
        }
        printf("outside-sheet %s %d", names[f], out);
        if (out) {
            printf(" box %d,%d-%d,%d", minx, miny, maxx, maxy);
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (by_code[n]) printf(" c%u=%d", botcorps_term_ov_i[n], by_code[n]);
        }
        printf("\n");
    }
    int idle_bad = 0;
    int hunt_bad = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        if (idle[i] >= 230 && idle[i] <= 248) idle_bad++;
        if (hunt[i] >= 230 && hunt[i] <= 248) hunt_bad++;
    }
    int idle13 = 0;
    int hunt13 = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        for (int n = 0; n < 13; ++n) {
            if (idle[i] == botcorps_term_ov_i[n]) idle13++;
            if (hunt[i] == botcorps_term_ov_i[n]) hunt13++;
        }
    }
    printf("idle codes 230-248 pixels %d explicit13 %d\n", idle_bad, idle13);
    printf("hunt codes 230-248 pixels %d explicit13 %d\n", hunt_bad, hunt13);
    return 0;
}
