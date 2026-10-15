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
    int hit[17];
    for (int n = 0; n < 17; ++n) hit[n] = 0;
    int other = 0;
    for (int i = 0; i < 320 * 240; ++i) {
        int found = 0;
        for (int n = 0; n < botcorps_term_ov_n; ++n)
            if (idx[i] == botcorps_term_ov_i[n]) { hit[n]++; found = 1; break; }
        if (!found && idx[i] >= 230 && idx[i] <= 248) other++;
    }
    FILE *f = fopen("/tmp/term_idx_hist.txt", "w");
    int sum = 0;
    for (int n = 0; n < botcorps_term_ov_n && n < 17; ++n) {
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
    static uint8_t list[320 * 240];
    static uint8_t idle[320 * 240];
    static uint8_t bus[320 * 240];
    static uint8_t amber[320 * 240];
    static uint8_t bots[320 * 240];
    host_fit_calls = 0;
    botcorps_host_terminal(term);
    const int term_lut = host_term_lut;
    printf("sheet pal_fit calls %d\n", host_fit_calls);
    printf("cmds sheet swap %d\n", botcorps_host_cmds_sheet());
    printf("int override %d span %d\n", botcorps_host_int_span(), host_rgb_span);
    printf("int full %d strip %d partial %d\n", host_int_full, host_int_strip, host_int_part);
    write_raw("/tmp/term.rgb", term, term_lut);
    hist_term(term);
    botcorps_host_idle(idle);
    const int idle_lut = host_term_lut;
    write_raw("/tmp/idle.rgb", idle, idle_lut);
    botcorps_host_hunt(hunt);
    const int hunt_lut = host_term_lut;
    write_raw("/tmp/hunt.rgb", hunt, hunt_lut);
    botcorps_host_list(list);
    const int list_lut = host_term_lut;
    write_raw("/tmp/list.rgb", list, list_lut);
    botcorps_host_bus(bus);
    const int bus_lut = host_term_lut;
    write_raw("/tmp/bus.rgb", bus, bus_lut);
    botcorps_host_amber(amber);
    const int amber_lut = host_term_lut;
    write_raw("/tmp/amber.rgb", amber, amber_lut);
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
    const int bot_lut = host_term_lut;
    write_raw("/tmp/bots.rgb", bots, bot_lut);
    static uint8_t onebot[320 * 240];
    for (int b = 0; b < 12; ++b) {
        char path[64];
        snprintf(path, sizeof(path), "/tmp/bot-%02d.rgb", b);
        botcorps_host_botscan_n(onebot, b);
        write_raw(path, onebot, bot_lut);
    }
    /* Tab-open sheet rect at s_sheet_i=2: x=14..306, y=104..222.
     * Override pixels outside that rect must be 0. Frames with no
     * sheet count the whole 320x240. */
    const int frames_n = 7;
    const char *names[] = { "tab-open", "idle", "amber", "hunt", "list", "gibson", "botscan" };
    const uint8_t *frames[] = { term, idle, amber, hunt, list, bus, bots };
    const int sheet_on[] = { 1, 0, 1, 0, 0, 0, 0 };
    for (int f = 0; f < frames_n; ++f) {
        int out = 0;
        int by_code[17];
        for (int n = 0; n < 17; ++n) by_code[n] = 0;
        int minx = 320, miny = 240, maxx = 0, maxy = 0;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                const int inside = sheet_on[f] && x >= 14 && x <= 306 && y >= 104 && y <= 222;
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
    /* One out-and-back of the 7000 ms scout period, twelve frames. */
    static uint8_t seq[320 * 240];
    for (int f = 0; f < 12; ++f) {
        host_set_millis((uint32_t)f * (7000u / 12u));
        botcorps_host_bus(seq);
        int out = 0;
        for (int i = 0; i < 320 * 240; ++i) {
            for (int n = 0; n < botcorps_term_ov_n; ++n) {
                if (seq[i] == botcorps_term_ov_i[n]) { out++; break; }
            }
        }
        printf("outside-sheet gibson-f%02d %d\n", f, out);
        char path[64];
        snprintf(path, sizeof(path), "/tmp/bus-f%02d.rgb", f);
        write_raw(path, seq, 0);
    }
    host_set_millis(2500);
    /* Sheet selection through paint_ext. Stepped arrows versus a clean frame. */
    static uint8_t sheet[320 * 240];
    static uint8_t clean[320 * 240];
    static uint8_t stepped[320 * 240];
    botcorps_host_sheet(clean, 0);
    memcpy(stepped, clean, sizeof(stepped));
    const int keys[] = { 2, 2, 2, 1, 1, 1 };
    const int expect[] = { 1, 2, 3, 2, 1, 0 };
    for (int i = 0; i < 6; ++i) {
        botcorps_host_sheet_key(stepped, keys[i]);
        botcorps_host_sheet(clean, expect[i]);
        int diff = 0;
        for (int p = 0; p < 320 * 240; ++p) if (stepped[p] != clean[p]) diff++;
        printf("sheet-step %d sel %d diff %d\n", i, expect[i], diff);
        int out = 0;
        for (int y = 0; y < 240; ++y) {
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                const uint8_t c = stepped[y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n)
                    if (c == botcorps_term_ov_i[n]) { out++; break; }
            }
        }
        printf("outside-sheet step-sel%d %d\n", expect[i], out);
        if (expect[i] == 3) write_raw("/tmp/sheet-step3.rgb", stepped, 1);
    }
    write_raw("/tmp/sheet-step.rgb", stepped, 1);
    botcorps_host_sheet(sheet, 0);
    write_raw("/tmp/sheet0.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 1);
    write_raw("/tmp/sheet1.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 2);
    write_raw("/tmp/sheet2.rgb", sheet, 1);
    botcorps_host_sheet(sheet, 3);
    write_raw("/tmp/sheet3.rgb", sheet, 1);
    /* Probe the sheet the device path just painted. */
    botcorps_host_sheet(sheet, 0);
    const int probes[][2] = {
        { 14, 104 }, { 15, 104 }, { 14, 105 }, { 16, 104 }, { 14, 106 },
        { 306, 104 }, { 305, 104 }, { 306, 105 }, { 304, 104 }, { 306, 106 },
        { 14, 222 }, { 15, 222 }, { 14, 221 }, { 16, 222 }, { 14, 220 },
        { 306, 222 }, { 305, 222 }, { 306, 221 }, { 304, 222 }, { 306, 220 },
        { 62, 114 }, { 100, 114 }, { 293, 114 }, { 100, 115 }, { 100, 116 }, { 100, 117 },
        { 21, 220 }, { 150, 220 }, { 301, 220 },
        { 17, 120 }, { 18, 120 }, { 40, 120 }, { 17, 121 }, { 40, 121 },
        { 30, 110 }, { 22, 114 }, { 22, 115 }
    };
    printf("probe sel0");
    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
        printf(" %d,%d=%u", probes[i][0], probes[i][1], sheet[probes[i][1] * 320 + probes[i][0]]);
    printf("\n");
    botcorps_host_sheet(sheet, 1);
    printf("probe sel1");
    for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i)
        printf(" %d,%d=%u", probes[i][0], probes[i][1], sheet[probes[i][1] * 320 + probes[i][0]]);
    printf("\n");
    int out0 = 0;
    int out1 = 0;
    botcorps_host_sheet(sheet, 0);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out0++; break; }
        }
    botcorps_host_sheet(sheet, 1);
    for (int y = 0; y < 240; ++y)
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            for (int n = 0; n < botcorps_term_ov_n; ++n)
                if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out1++; break; }
        }
    printf("outside-sheet row0 %d row1 %d\n", out0, out1);
    botcorps_host_sheet(sheet, 0);
    static const uint8_t corn[6][6] = {
        { 235, 230, 230, 230, 230, 230 },
        { 230, 236, 236, 235, 235, 235 },
        { 230, 236, 236, 232, 232, 232 },
        { 230, 235, 232, 234, 234, 234 },
        { 230, 235, 232, 234, 236, 236 },
        { 230, 235, 232, 234, 236, 236 },
    };
    int corn_bad = 0;
    const int oxs[4] = { 14, 306, 14, 306 };
    const int oys[4] = { 104, 104, 222, 222 };
    const int sxs[4] = { 1, -1, 1, -1 };
    const int sys[4] = { 1, 1, -1, -1 };
    for (int k = 0; k < 4; ++k) {
        for (int dy = 0; dy < 6; ++dy) {
            for (int dx = 0; dx < 6; ++dx) {
                const int x = oxs[k] + sxs[k] * dx;
                const int y = oys[k] + sys[k] * dy;
                const uint8_t got = sheet[y * 320 + x];
                if (x >= 17 && x <= 49 && y >= 105 && y <= 121) continue;
                if (got != corn[dy][dx]) {
                    corn_bad++;
                    if (corn_bad <= 16) printf(" corner %d,%d got %u want %u\n", x, y, got, corn[dy][dx]);
                }
            }
        }
    }
    printf("corners-bad %d\n", corn_bad);
    int y105_236 = 0;
    int y107_234 = 0;
    for (int x = 17; x <= 49; ++x) {
        if (sheet[105 * 320 + x] == 236) y105_236++;
        if (sheet[107 * 320 + x] == 234) y107_234++;
    }
    printf("glow y105-236 %d y107-234 %d\n", y105_236, y107_234);
    printf("bottom");
    printf(" y222=%u", sheet[222 * 320 + 100]);
    printf(" y221=%u", sheet[221 * 320 + 100]);
    printf(" y220=%u", sheet[220 * 320 + 100]);
    printf(" y219=%u", sheet[219 * 320 + 100]);
    printf(" y218h=%u", sheet[218 * 320 + 100]);
    printf(" y218e=%u", sheet[218 * 320 + 18]);
    printf(" y218g=%u", sheet[218 * 320 + 300]);
    printf(" y217=%u\n", sheet[217 * 320 + 100]);
    int n127 = 0;
    for (int y = 120; y <= 132; ++y)
        for (int x = 17; x <= 302; ++x)
            if (sheet[y * 320 + x] == 127) n127++;
    printf("sel-text-127 %d\n", n127);
    printf(" prompt");
    printf(" %d,%d=%u", 9, 227, sheet[227 * 320 + 9]);
    printf(" %d,%d=%u", 8, 227, sheet[227 * 320 + 8]);
    printf(" %d,%d=%u", 9, 228, sheet[228 * 320 + 9]);
    int halo232 = 0;
    for (int y = 115; y <= 117; ++y)
        for (int x = 62; x <= 293; ++x)
            if (sheet[y * 320 + x] == 232) halo232++;
    printf(" rule-halo232 %d\n", halo232);
    static uint8_t sheetb[320 * 240];
    botcorps_host_sheet(sheetb, 1);
    int glyph_diff = 0;
    int glow236 = 0;
    int n194 = 0;
    int n5 = 0;
    for (int y = 108; y <= 118; ++y) {
        for (int x = 18; x <= 52; ++x) {
            const uint8_t a = sheet[y * 320 + x];
            const uint8_t b = sheetb[y * 320 + x];
            if ((a == 230 || b == 230) && a != b) {
                glyph_diff++;
                if (glyph_diff <= 12) printf(" glyph %d,%d %u->%u\n", x, y, a, b);
            }
            if (b == 236) glow236++;
        }
    }
    for (int y = 220; y <= 236; ++y)
        for (int x = 0; x <= 120; ++x) {
            if (sheet[y * 320 + x] == 194) n194++;
            if (sheet[y * 320 + x] == 5) n5++;
        }
    printf("core-glyph-diff %d unselected-236 %d prompt194 %d glow5 %d\n",
           glyph_diff, glow236, n194, n5);
    botcorps_host_idle(idle);
    int tile_diff = 0;
    for (int y = 69; y <= 103; ++y)
        for (int x = 7; x <= 78; ++x)
            if (sheet[y * 320 + x] != idle[y * 320 + x]) tile_diff++;
    int idle170 = 0;
    for (int x = 4; x <= 315; ++x) if (idle[167 * 320 + x] == 170) idle170++;
    printf("tile-outside-diff %d idle-170 %d\n", tile_diff, idle170);
    int tab_out = 0;
    botcorps_host_sheet(sheet, 0);
    botcorps_host_idle(idle);
    for (int y = 0; y < 240; ++y) {
        for (int x = 0; x < 320; ++x) {
            if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
            if (sheet[y * 320 + x] != idle[y * 320 + x]) {
                tab_out++;
                if (tab_out <= 8) printf(" tab-out %d,%d %u %u\n", x, y, sheet[y * 320 + x], idle[y * 320 + x]);
            }
        }
    }
    printf("tab-open-vs-closed-outside %d\n", tab_out);
    /* Signal docks. Shift rotates the existing bar triple; paint_ext still draws the scan. */
    botcorps_host_sig(0);
    host_set_millis(2500);
    botcorps_host_bus(bus);
    write_raw("/tmp/bus-sig0.rgb", bus, 0);
    const int docks[][2] = {
        { 101, 77 }, { 65, 117 }, { 47, 167 }, { 219, 77 },
        { 140, 55 }, { 160, 55 }, { 180, 55 },
        { 140, 180 }, { 160, 180 }, { 180, 180 }
    };
    printf("docks");
    for (unsigned i = 0; i < sizeof(docks) / sizeof(docks[0]); ++i) {
        const int x = docks[i][0];
        const int y = docks[i][1];
        printf(" %d,%d edge=%u", x, y, bus[y * 320 + (x - 7)]);
    }
    printf("\n");
    printf("old-edge");
    const int oldd[][2] = { { 53, 67 }, { 53, 117 }, { 53, 167 }, { 267, 67 } };
    for (unsigned i = 0; i < 4; ++i)
        printf(" %d,%d=%u", oldd[i][0], oldd[i][1], bus[oldd[i][1] * 320 + (oldd[i][0] - 7)]);
    printf("\n");
    botcorps_host_sig(1);
    botcorps_host_bus(bus);
    write_raw("/tmp/bus-sig1.rgb", bus, 0);
    printf("sig1-edge 65,67=%u 101,77=%u 47,167=%u 140,55=%u\n",
           bus[67 * 320 + (65 - 7)], bus[77 * 320 + (101 - 7)],
           bus[167 * 320 + (47 - 7)], bus[55 * 320 + (140 - 7)]);
    botcorps_host_sig(0);
    /* sel 0 (top 0), sel 20 (top 19), sel 39 (top 35). */
    {
        static const uint8_t grid[6][6] = {
            { 235, 230, 230, 230, 230, 230 },
            { 230, 236, 236, 235, 235, 235 },
            { 230, 236, 236, 232, 232, 232 },
            { 230, 235, 232, 234, 234, 234 },
            { 230, 235, 232, 234, 236, 236 },
            { 230, 235, 232, 234, 236, 236 },
        };
        const int sels[] = { 0, 20, 39 };
        const char *paths[] = { "/tmp/sheet-s0.rgb", "/tmp/sheet-s20.rgb", "/tmp/sheet-s39.rgb" };
        for (int si = 0; si < 3; ++si) {
            botcorps_host_sheet(sheet, sels[si]);
            write_raw(paths[si], sheet, 1);
            int bad = 0;
            printf("dump sel %d\n", sels[si]);
            printf(" TL");
            for (int dy = 0; dy < 6; ++dy) {
                for (int dx = 0; dx < 6; ++dx) {
                    const uint8_t g = sheet[(104 + dy) * 320 + (14 + dx)];
                    printf(" %u", g);
                    if (g != grid[dy][dx]) bad++;
                }
            }
            printf("\n grid-bad %d\n", bad);
            printf(" top");
            for (int y = 104; y <= 109; ++y) {
                printf(" y%d", y);
                for (int x = 14; x <= 25; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n");
            int hair = 0;
            for (int x = 20; x <= 299; ++x) if (sheet[218 * 320 + x] != 231) hair++;
            printf(" hair-not-231 %d y220=%u text127", hair, sheet[220 * 320 + 100]);
            int n127 = 0;
            for (int i = 0; i < 320 * 240; ++i) if (sheet[i] == 127) n127++;
            printf(" %d\n", n127);
            printf(" y216");
            for (int x = 14; x <= 19; ++x) printf(" %u", sheet[216 * 320 + x]);
            printf(" |");
            for (int x = 301; x <= 306; ++x) printf(" %u", sheet[216 * 320 + x]);
            printf("\n y217");
            for (int x = 14; x <= 22; ++x) printf(" %u", sheet[217 * 320 + x]);
            printf(" mid%u", sheet[217 * 320 + 160]);
            printf(" |");
            for (int x = 300; x <= 306; ++x) printf(" %u", sheet[217 * 320 + x]);
            printf("\n");
            printf(" up-arrow");
            for (int y = 110; y <= 117; ++y) {
                printf(" y%d", y);
                for (int x = 293; x <= 306; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n dn-arrow");
            for (int y = 208; y <= 215; ++y) {
                printf(" y%d", y);
                for (int x = 293; x <= 306; ++x) printf(",%u", sheet[y * 320 + x]);
            }
            printf("\n");
            int out = 0;
            for (int y = 0; y < 240; ++y)
                for (int x = 0; x < 320; ++x) {
                    if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                    for (int n = 0; n < botcorps_term_ov_n; ++n)
                        if (sheet[y * 320 + x] == botcorps_term_ov_i[n]) { out++; break; }
                }
            printf(" outside %d\n", out);
        }
    }
    botcorps_host_sig(0);
    botcorps_host_bus_ble(bus);
    write_raw("/tmp/bus-ble.rgb", bus, 0);
    printf("ble-edge 83,67=%u 101,77=%u 65,117=%u 47,167=%u 140,55=%u\n",
           bus[67 * 320 + (83 - 7)], bus[77 * 320 + (101 - 7)],
           bus[117 * 320 + (65 - 7)], bus[167 * 320 + (47 - 7)],
           bus[55 * 320 + (140 - 7)]);
    static uint8_t hit[320 * 240];
    botcorps_host_gchat(hit);
    write_raw("/tmp/hit-gchat.rgb", hit, 0);
    botcorps_host_gmenu(hit);
    write_raw("/tmp/hit-gmenu.rgb", hit, 0);
    botcorps_host_jackin(hit);
    write_raw("/tmp/hit-jackin.rgb", hit, 0);
    botcorps_host_scanall(hit);
    write_raw("/tmp/hit-scanall.rgb", hit, 0);
    {
        const int rings[4][3] = {
            { 4, 167, 69 }, { 5, 168, 171 }, { 6, 169, 207 }, { 7, 170, 207 }
        };
        int ring_ok = 0;
        for (int i = 0; i < 4; ++i)
            if (hit[rings[i][1] * 320 + rings[i][0]] == rings[i][2]) ring_ok++;
        const int bot = hit[223 * 320 + 4];
        int prompt_glow = 0;
        int salmon = 0;
        for (int y = 224; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                const uint8_t c = hit[y * 320 + x];
                if (c == 69 || c == 171 || c == 207) prompt_glow++;
            }
        for (int y = 167; y <= 223; ++y) {
            const uint8_t l = hit[y * 320 + 4];
            const uint8_t r = hit[y * 320 + 315];
            if (l == 205 || l == 238) salmon++;
            if (r == 205 || r == 238) salmon++;
        }
        int ov = 0;
        for (int y = 0; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                const uint8_t c = hit[y * 320 + x];
                for (int n = 0; n < botcorps_term_ov_n; ++n)
                    if (c == botcorps_term_ov_i[n]) { ov++; break; }
            }
        printf("scan-magenta rings %d/4 bottom %u prompt-glow %d salmon-border %d ov-out %d prompt194 %u\n",
               ring_ok, bot, prompt_glow, salmon, ov, hit[227 * 320 + 9]);
    }
    botcorps_host_scanall_sheet(hit);
    write_raw("/tmp/hit-scanall-tab.rgb", hit, 1);
    {
        int frame = 0;
        for (int y = 166; y < 240; ++y)
            for (int x = 0; x < 320; ++x) {
                if (x >= 14 && x <= 306 && y >= 104 && y <= 222) continue;
                const uint8_t c = hit[y * 320 + x];
                if (c == 69 || c == 171 || c == 207) frame++;
            }
        printf("tab-covers-log frame-outside %d edge %u\n", frame, hit[167 * 320 + 4]);
    }
    botcorps_host_scanall(hit);
    write_raw("/tmp/hit-scanall.rgb", hit, 0);
    botcorps_host_keyfob(hit);
    write_raw("/tmp/hit-keyfob.rgb", hit, 0);
    botcorps_host_hub(hit);
    write_raw("/tmp/hit-hub.rgb", hit, 0);
    botcorps_host_intro(hit);
    write_raw("/tmp/hit-intro.rgb", hit, 0);
    botcorps_host_sheet(hit, 0);
    printf("core-col");
    for (int y = 105; y <= 109; ++y) printf(" %u", hit[y * 320 + 30]);
    printf("\n");
    write_raw("/tmp/hit-tab0.rgb", hit, 1);
    botcorps_host_sheet(hit, 20);
    write_raw("/tmp/hit-tab20.rgb", hit, 1);
    botcorps_host_sheet(hit, 39);
    write_raw("/tmp/hit-tab39.rgb", hit, 1);
    botcorps_host_idle(hit);
    write_raw("/tmp/hit-idle.rgb", hit, 0);
    return 0;
}
