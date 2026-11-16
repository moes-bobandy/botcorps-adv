/* MOCK ONLY. Clone the locked CONTROLbot Color UI v6 still (a87fe22) and swap the
 * center bot face + optional name. Gold / silver faces are a host compile of the
 * firmware painters in src/ui/botcorps_cyd_edge.inc (e_gold_face, e_silver_face,
 * e_term_marks, e_metal_face eyes) at size 110 — the same 110x110 slot as the
 * locked v6 CONTROLbot face (THEME_FACE 65,88..174,197). Blink is held off.
 * Name uses real lv_draw.h b_text / LV_GLCD. Unchanged pixels are copied
 * byte-for-byte from the base 888 still (no RGB565 round-trip of the board).
 */
#define LV_HOST 1
#define LV_HOST_BAND_ROWS 320
#include "lv_draw.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

static const int W = 240, H = 320;
static const int FACE_X = 65, FACE_Y = 88, FACE_S = 110; /* v6 THEME_FACE inclusive 65,88..174,197 */
static const int BOT_X0 = 54, BOT_Y0 = 79, BOT_X1 = 185, BOT_Y1 = 210; /* LV_CB_BOT */
static const int NAME_Y = 200; /* 5x7 sits in the black frame pad under the face (y 198..210) */

/* Documented sim RGB for the metallic palette slots in botcorps_cyd_edge.inc */
static uint8_t PAL[256][3];
static void pal_init(void)
{
    memset(PAL, 0, sizeof PAL);
    auto set = [](int i, int r, int g, int b) { PAL[i][0] = (uint8_t)r; PAL[i][1] = (uint8_t)g; PAL[i][2] = (uint8_t)b; };
    set(108, 82, 69, 24);
    set(104, 99, 73, 16);
    set(140, 148, 109, 0);
    set(177, 181, 146, 82);
    set(213, 247, 182, 74);
    set(249, 247, 211, 74);
    set(250, 255, 219, 173);
    set(41, 41, 56, 66);
    set(110, 107, 121, 132);
    set(146, 140, 154, 165);
    set(186, 198, 207, 214);
    set(223, 231, 239, 247);
    set(255, 255, 255, 255);
}

/* Minimal Gfx/cbf shim: paint into an s*s RGB888 face buffer. */
struct FaceBuf { uint8_t *px; int s; };
static FaceBuf *g_fb = nullptr;
struct Gfx {}; /* unused; firmware signature kept */
static void cbf(Gfx, int x, int y, int w, int h, uint8_t c)
{
    const int s = g_fb->s;
    uint8_t *p = g_fb->px;
    const uint8_t r = PAL[c][0], g = PAL[c][1], b = PAL[c][2];
    int x0 = x < 0 ? 0 : x, y0 = y < 0 ? 0 : y;
    int x1 = x + w > s ? s : x + w, y1 = y + h > s ? s : y + h;
    for (int yy = y0; yy < y1; ++yy)
        for (int xx = x0; xx < x1; ++xx) {
            uint8_t *q = p + (yy * s + xx) * 3;
            q[0] = r; q[1] = g; q[2] = b;
        }
}

/* ---- verbatim extract of e_gold_face / e_term_marks / e_silver_face / gold eyes
 * from /workspace/cyd-fw-3locks-wt/src/ui/botcorps_cyd_edge.inc (c1b5659 / 049ff59 line
 * of the 3locks tree). Only the host cbf target changed. Blink forced off. ---- */

static void e_gold_face(Gfx g, int x, int y, int s)
{
    static const uint8_t RIM = 108, BRZ = 104, LO = 140, BASE = 177, BRT = 213, SHN = 249, PALE = 250;
    const int bw = s >= 12 ? 2 : 1;
    const int sh0 = (s * 70) / 100, shw = s / 6 > 2 ? s / 6 : 2;
    for (int r = 0; r < s; ++r) {
        int i = 0;
        while (i < s) {
            const int d = i + r;
            uint8_t c; int run = 1;
            if (d < (s * 45) / 100) c = BRT;
            else if (d >= sh0 && d < sh0 + shw) c = (d == sh0 + shw / 2) ? PALE : SHN;
            else if (d >= (s * 165) / 100) c = BRZ;
            else if (d >= (s * 135) / 100) c = LO;
            else c = BASE;
            while (i + run < s) {
                const int d2 = i + run + r; uint8_t c2;
                if (d2 < (s * 45) / 100) c2 = BRT;
                else if (d2 >= sh0 && d2 < sh0 + shw) c2 = (d2 == sh0 + shw / 2) ? PALE : SHN;
                else if (d2 >= (s * 165) / 100) c2 = BRZ;
                else if (d2 >= (s * 135) / 100) c2 = LO;
                else c2 = BASE;
                if (c2 != c) break; ++run;
            }
            cbf(g, x + i, y + r, run, 1, c);
            i += run;
        }
    }
    for (int k = 0; k < bw; ++k) {
        const uint8_t lit = k == 0 ? PALE : BRT, dk = k == 0 ? RIM : BRZ;
        cbf(g, x + k, y + k, s - 2 * k, 1, lit);
        cbf(g, x + k, y + k, 1, s - 2 * k, lit);
        cbf(g, x + k, y + s - 1 - k, s - 2 * k, 1, dk);
        cbf(g, x + s - 1 - k, y + k, 1, s - 2 * k, dk);
    }
    const int gx = x + s / 5, gy = y + s / 5, gw = s / 4 > 2 ? s / 4 : 2;
    cbf(g, gx, gy, gw, 1, PALE);
    cbf(g, gx, gy, 1, gw / 2 > 1 ? gw / 2 : 1, PALE);
}

static int e_term_marks(int x, int y, int s, bool blink, int R[][4])
{
    int n = 0;
    const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;
    const int ey = y + c + (blink ? c - 1 : 0), eh = blink ? 1 : c;
    R[n][0] = x + m; R[n][1] = ey; R[n][2] = c; R[n][3] = eh; ++n;
    R[n][0] = x + m + 2 * c; R[n][1] = ey; R[n][2] = c; R[n][3] = eh; ++n;
    const int bw = s >= 12 ? 2 : 1, eye_bot = y + 2 * c;
    int u = s / 12 > 1 ? s / 12 : 1;
    while (u > 1 && y + s - bw - 1 - 5 * u < eye_bot + 1) --u;
    if (y + s - bw - 1 - 5 * u >= eye_bot + 1) {
        const int gw = 8 * u, gx = x + (s - gw) / 2, gy = y + s - bw - 1 - 5 * u;
        static const int8_t OFS[5] = { 0, 1, 2, 1, 0 };
        for (int r = 0; r < 5; ++r) { R[n][0] = gx + OFS[r] * u; R[n][1] = gy + r * u; R[n][2] = 2 * u; R[n][3] = u; ++n; }
        R[n][0] = gx + 5 * u; R[n][1] = gy + 4 * u; R[n][2] = 3 * u; R[n][3] = u; ++n;
    } else {
        const int gx = x + (s - 5) / 2, gy0 = y + s - bw - 1 - 3, gy = gy0 > eye_bot ? gy0 : eye_bot;
        static const int8_t OFS[3] = { 0, 1, 0 };
        for (int r = 0; r < 3; ++r) { R[n][0] = gx + OFS[r]; R[n][1] = gy + r; R[n][2] = 1; R[n][3] = 1; ++n; }
        R[n][0] = gx + 3; R[n][1] = gy + 2; R[n][2] = 2; R[n][3] = 1; ++n;
    }
    return n;
}

static void e_silver_face(Gfx g, int x, int y, int s)
{
    static const uint8_t RIM = 41, LO = 110, MID = 146, BASE = 186, HI = 223, GL = 255;
    const int bw = s >= 12 ? 2 : 1;
    const int sh0 = (s * 70) / 100, shw = s / 6 > 2 ? s / 6 : 2;
    for (int r = 0; r < s; ++r) {
        int i = 0;
        while (i < s) {
            auto col = [&](int d) -> uint8_t {
                if (d < (s * 45) / 100) return HI;
                if (d >= sh0 && d < sh0 + shw) return HI;
                if (d >= (s * 165) / 100) return LO;
                if (d >= (s * 135) / 100) return MID;
                return BASE;
            };
            const uint8_t c = col(i + r); int run = 1;
            while (i + run < s && col(i + run + r) == c) ++run;
            cbf(g, x + i, y + r, run, 1, c);
            i += run;
        }
    }
    for (int k = 0; k < bw; ++k) {
        const uint8_t lit = k == 0 ? GL : HI, dk = k == 0 ? RIM : LO;
        cbf(g, x + k, y + k, s - 2 * k, 1, lit);
        cbf(g, x + k, y + k, 1, s - 2 * k, lit);
        cbf(g, x + k, y + s - 1 - k, s - 2 * k, 1, dk);
        cbf(g, x + s - 1 - k, y + k, 1, s - 2 * k, dk);
    }
    if (s >= 12) cbf(g, x + bw + 1, y + bw, s / 4, 1, GL);
    int R[8][4]; const int n = e_term_marks(x, y, s, false, R); /* blink off for stills */
    for (int q = 0; q < n; ++q) cbf(g, R[q][0], R[q][1], R[q][2], R[q][3], 0);
}

static void e_gold_eyes(Gfx g, int x, int y, int s)
{
    const int c = s / 5 > 2 ? s / 5 : 2, m = (s - 3 * c) / 2;
    cbf(g, x + m, y + c, c, c, 0);
    cbf(g, x + m + 2 * c, y + c, c, c, 0);
}

static void paint_gold(uint8_t *px, int s) { FaceBuf fb{px, s}; g_fb = &fb; Gfx g{}; e_gold_face(g, 0, 0, s); e_gold_eyes(g, 0, 0, s); }
static void paint_silver(uint8_t *px, int s) { FaceBuf fb{px, s}; g_fb = &fb; Gfx g{}; e_silver_face(g, 0, 0, s); }

static std::vector<uint8_t> load_rgb(const char *path, size_t expect)
{
    FILE *f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "open %s\n", path); exit(1); }
    std::vector<uint8_t> b(expect);
    if (fread(b.data(), 1, expect, f) != expect) { fprintf(stderr, "short %s\n", path); exit(1); }
    fclose(f);
    return b;
}

static void nn_blit_fit(uint8_t *dst, int ds, const uint8_t *src, int sw, int sh)
{
    /* uniform nearest-neighbour, centered in ds x ds, unfilled stays 0 */
    memset(dst, 0, ds * ds * 3);
    const int sc = ds * 256 / (sw > sh ? sw : sh); /* 8.8 fixed; may leave 1px slack */
    const int dw = (sw * sc) / 256, dh = (sh * sc) / 256;
    const int ox = (ds - dw) / 2, oy = (ds - dh) / 2;
    for (int y = 0; y < dh; ++y) {
        const int sy = (int)((int64_t)y * sh / dh);
        for (int x = 0; x < dw; ++x) {
            const int sx = (int)((int64_t)x * sw / dw);
            memcpy(dst + ((oy + y) * ds + (ox + x)) * 3, src + (sy * sw + sx) * 3, 3);
        }
    }
}

static void blit_face(uint8_t *frame, const uint8_t *face, int s)
{
    for (int y = 0; y < s; ++y)
        memcpy(frame + ((FACE_Y + y) * W + FACE_X) * 3, face + y * s * 3, s * 3);
}

/* Draw name with real LV_GLCD via b_text into a 1-row-high scratch, then write only glyph pixels in 888. */
static void draw_name(uint8_t *frame, const char *name, uint8_t r, uint8_t g, uint8_t b)
{
    if (!name || !name[0]) return;
    const int n = (int)strlen(name);
    const int tw = n * 6 - 1;
    const int x = (BOT_X0 + BOT_X1 + 1 - tw) / 2;
    const uint16_t c565 = lv_rgb(r, g, b);
    std::vector<uint16_t> band((size_t)W * 16, 0);
    LvBand B{band.data(), 0, NAME_Y, W, 16};
    /* clear the name pad first (black) so leftover face-lip doesn't fight the glyphs */
    for (int yy = 198; yy <= BOT_Y1; ++yy)
        for (int xx = BOT_X0; xx <= BOT_X1; ++xx) {
            uint8_t *q = frame + (yy * W + xx) * 3;
            q[0] = q[1] = q[2] = 0;
        }
    b_text(B, x, NAME_Y, name, c565, 1);
    auto exp = [](int v, int bits) { return v * 255 / ((1 << bits) - 1); };
    for (int yy = 0; yy < 16; ++yy)
        for (int xx = 0; xx < W; ++xx) {
            uint16_t v = band[yy * W + xx];
            if (!v) continue;
            uint8_t *q = frame + ((NAME_Y + yy) * W + xx) * 3;
            q[0] = (uint8_t)exp((v >> 11) & 31, 5);
            q[1] = (uint8_t)exp((v >> 5) & 63, 6);
            q[2] = (uint8_t)exp(v & 31, 5);
        }
}

static void usage()
{
    fprintf(stderr, "usage: bigbot_compose base.rgb out.rgb gold|silver|raw [raw.rgb rawW rawH] [name]\n");
    exit(2);
}

int main(int argc, char **argv)
{
    pal_init();
    if (argc < 4) usage();
    const char *basep = argv[1], *outp = argv[2], *kind = argv[3];
    std::vector<uint8_t> frame = load_rgb(basep, (size_t)W * H * 3);
    std::vector<uint8_t> face((size_t)FACE_S * FACE_S * 3, 0);
    int ai = 4;
    if (!strcmp(kind, "gold")) {
        paint_gold(face.data(), FACE_S);
    } else if (!strcmp(kind, "silver")) {
        paint_silver(face.data(), FACE_S);
    } else if (!strcmp(kind, "raw")) {
        if (argc < 7) usage();
        int sw = atoi(argv[4]), sh = atoi(argv[5]);
        auto raw = load_rgb(argv[6], (size_t)sw * sh * 3);
        nn_blit_fit(face.data(), FACE_S, raw.data(), sw, sh);
        ai = 7;
    } else if (!strcmp(kind, "none")) {
        /* leave the locked face; optional name only */
        ai = 4;
    } else usage();
    if (strcmp(kind, "none") != 0) blit_face(frame.data(), face.data(), FACE_S);
    const char *name = (ai < argc) ? argv[ai] : "";
    if (name[0]) draw_name(frame.data(), name, 231, 239, 247);
    FILE *o = fopen(outp, "wb");
    if (!o) { fprintf(stderr, "write %s\n", outp); return 1; }
    fwrite(frame.data(), 1, frame.size(), o);
    fclose(o);
    return 0;
}
