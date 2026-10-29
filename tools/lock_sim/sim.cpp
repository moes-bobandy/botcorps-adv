/* Host sim for src/lock: same lock_core/lock_flow code as the firmware,
 * zlib raw inflate in place of ROM tinfl, RGB565 palette -> RGB888 frames.
 *   sim seg <s> <out.rgb>        every frame of segment s (+1 after loop-back)
 *   sim flow <script> <outdir>   lines: "tick N" | "tap X Y" | "dump NAME"  */
#include <zlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include "../../src/lock/lock_flow.h"

static uint8_t fb[320][240];
static const uint16_t *g_pal;
static std::vector<uint8_t> blob;
static uint8_t ibuf[LK_INFL_CAP];

static int infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    z_stream z; memset(&z, 0, sizeof z);
    inflateInit2(&z, -15);
    z.next_in = (Bytef *)src; z.avail_in = len; z.next_out = dst; z.avail_out = cap;
    int r = inflate(&z, Z_FINISH); int out = (int)z.total_out; inflateEnd(&z);
    return r == Z_STREAM_END ? out : -1;
}
static void draw(int y, int x, int n, const uint8_t *idx) { memcpy(&fb[y][x], idx, n); }
static void dump(FILE *f)
{
    static uint8_t row[240 * 3];
    for (int y = 0; y < 320; ++y) {
        for (int x = 0; x < 240; ++x) {
            uint16_t v = g_pal[fb[y][x]];
            int r = (v >> 11) & 31, g = (v >> 5) & 63, b = v & 31;
            row[x * 3] = (r << 3) | (r >> 2); row[x * 3 + 1] = (g << 2) | (g >> 4); row[x * 3 + 2] = (b << 3) | (b >> 2);
        }
        fwrite(row, 1, sizeof row, f);
    }
}
int main(int argc, char **argv)
{
    FILE *bf = fopen("src/lock/lock_blob.bin", "rb");
    if (!bf) { perror("blob"); return 1; }
    blob.resize(LK_BLOB_SIZE);
    if (fread(blob.data(), 1, LK_BLOB_SIZE, bf) != LK_BLOB_SIZE) return 1;
    LkIo io{blob.data(), infl, draw, ibuf};
    if (argc == 4 && !strcmp(argv[1], "seg")) {
        int s = atoi(argv[2]); g_pal = LK_CLIP[lk_clip_of_seg(s)].pal;
        FILE *o = fopen(argv[3], "wb");
        LkPlay p; p.start(s);
        int n = LK_SEG[s].n + (LK_SEG[s].loop ? 1 : 0);
        for (int i = 0; i < n; ++i) { p.step(io); dump(o); }
        fclose(o); return 0;
    }
    if (argc == 4 && !strcmp(argv[1], "flow")) {
        FILE *sc = fopen(argv[2], "r"); char cmd[32]; LkFlow fl; fl.begin(0);
        int total = 0;
        while (fscanf(sc, "%31s", cmd) == 1) {
            if (!strcmp(cmd, "tick")) { int n; fscanf(sc, "%d", &n); for (int i = 0; i < n; ++i) { g_pal = LK_CLIP[lk_clip_of_seg(fl.p.seg)].pal; fl.tick(io); ++total; } }
            else if (!strcmp(cmd, "tap")) { int x, y; fscanf(sc, "%d %d", &x, &y); int r = fl.tap(x, y); printf("tap %d,%d -> scr=%d seg=%d game=%d theme=%d act=%d\n", x, y, fl.scr, fl.p.seg, fl.game, fl.theme, r); }
            else if (!strcmp(cmd, "dump")) {
                char nm[64]; fscanf(sc, "%63s", nm); char path[512]; snprintf(path, sizeof path, "%s/%s.rgb", argv[3], nm);
                FILE *o = fopen(path, "wb"); dump(o); fclose(o);
                printf("dump %s scr=%d seg=%d frame=%d clipframe=%d\n", nm, fl.scr, fl.p.seg, fl.p.shown, LK_SEG[fl.p.seg].f0 + fl.p.shown);
            }
        }
        return 0;
    }
    fprintf(stderr, "usage\n"); return 2;
}
