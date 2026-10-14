#include "host_shim.h"
#include "ui/botcorps_pal.h"
#include "ui/botcorps_pal_tables.inc"

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

static const uint8_t OV_I[13] = {
    230, 231, 232, 234, 235, 236, 237, 239, 242, 243, 246, 247, 248
};
static const uint16_t OV_C[13] = {
    0xF95A, 0x48A9, 0x3887, 0x3086, 0x58CA, 0x2064, 0x079F,
    0x026A, 0x01E8, 0x01A7, 0x02EC, 0x0105, 0xE6FA
};

static uint16_t word_of(uint8_t idx, int term)
{
    if (term) {
        for (int n = 0; n < 13; ++n) if (OV_I[n] == idx) return OV_C[n];
    }
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
        for (int n = 0; n < 13; ++n) if (idx[i] == OV_I[n]) { hit[n]++; found = 1; break; }
        if (!found && idx[i] >= 230 && idx[i] <= 248) other++;
    }
    FILE *f = fopen("/tmp/term_idx_hist.txt", "w");
    int sum = 0;
    for (int n = 0; n < 13; ++n) {
        printf("code %u pixels %d\n", OV_I[n], hit[n]);
        if (f) fprintf(f, "code %u pixels %d\n", OV_I[n], hit[n]);
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
    botcorps_host_terminal(term);
    write_raw("/tmp/term.rgb", term, host_term_lut);
    hist_term(term);
    botcorps_host_idle(idle);
    write_raw("/tmp/idle.rgb", idle, host_term_lut);
    botcorps_host_hunt(hunt);
    write_raw("/tmp/hunt.rgb", hunt, host_term_lut);
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
            if (idle[i] == OV_I[n]) idle13++;
            if (hunt[i] == OV_I[n]) hunt13++;
        }
    }
    printf("idle codes 230-248 pixels %d explicit13 %d\n", idle_bad, idle13);
    printf("hunt codes 230-248 pixels %d explicit13 %d\n", hunt_bad, hunt13);
    return 0;
}
