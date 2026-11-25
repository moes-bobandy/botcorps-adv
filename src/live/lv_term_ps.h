/* MOCK 2026-11-25: TERMINALbot subscreen — low-power PowerShell + Matrix area-scan readout (LV_SC_TPS).
 * The original Adv Matrix operator typewriter mock was not recovered (box data loss; one search pass of
 * /workspace, moes-bobandy GitHub repos/commits, backup/* branches, ADVbot/Adv mock folders). Type-out
 * is built from our own TERMINALbot face-rain glyph law (lv_tg_rain / LV_F35) plus the existing
 * (lv_now/500)&1 cursor blink, not cloned from that missing Adv source.
 * New screen only: no locked-scene redraw. Theme accents via lv_ink(LV_ACC) — never hard-coded purple.
 * Gold box / grey body use the locked LV_THC_* inks. Pins use the live theme sample (lv_thc_pin_*_px).
 * Entry: TERMINALbot LIST (minterm) or the `psterm` / `tps` command. Back returns to LV_SC_TERM.
 * Strip is the existing GIBSON pad pattern (cloned primitives), on this screen only. */
#pragma once
#if LV_TERM_PS

#define LV_TPS_X0 18
#define LV_TPS_Y0 28
#define LV_TPS_X1 221
#define LV_TPS_Y1 242
#define LV_TPS_TX 22
#define LV_TPS_TY 36
#define LV_TPS_ROW 8
#define LV_TPS_CW  6
#define LV_TPS_MAXL 28
#define LV_TPS_LSZ  44
#define LV_TPS_PERIOD 60   /* low-power refresh, ms */

static const uint16_t LV_TPS_PSBG  = lv_hex(0x06101C);   /* dark blue-black PowerShell */
static const uint16_t LV_TPS_PSBG2 = lv_hex(0x0A1828);
static const uint16_t LV_TPS_INK   = lv_hex(0xD8DCE0);   /* PS body text */
static const uint16_t LV_TPS_PROMPT= lv_hex(0xE8C050);   /* gold prompt, matches LV_THC_BOX_HOT */
static const uint16_t LV_TPS_MXH   = lv_hex(0x90FFA0);   /* Matrix rain head — same family as GIBSON/dojo 0x20ff60 */
static const uint16_t LV_TPS_MX    = lv_hex(0x20FF60);
static const uint16_t LV_TPS_MXD   = lv_hex(0x007830);
static const uint16_t LV_TPS_MXK   = lv_hex(0x003018);

static uint32_t lv_tps_t0 = 0;
static int lv_tps_held = 0, lv_tps_speed = 1;
static char lv_tps_line[LV_TPS_MAXL][LV_TPS_LSZ];
static uint8_t lv_tps_kind[LV_TPS_MAXL];   /* 0 sys, 1 prompt, 2 result, 3 dim/status */
static int lv_tps_n = 0, lv_tps_emit = 0, lv_tps_phase = 0;   /* 0 idle prompt, 1 scanning, 2 done prompt */

static void lv_tps_add(int kind, const char *fmt, ...)
{
    if (lv_tps_n >= LV_TPS_MAXL) {
        memmove(lv_tps_line[0], lv_tps_line[1], (LV_TPS_MAXL - 1) * LV_TPS_LSZ);
        memmove(lv_tps_kind, lv_tps_kind + 1, LV_TPS_MAXL - 1);
        --lv_tps_n;
    }
    va_list ap; va_start(ap, fmt);
    vsnprintf(lv_tps_line[lv_tps_n], LV_TPS_LSZ, fmt, ap);
    va_end(ap);
    lv_tps_kind[lv_tps_n] = (uint8_t)kind;
    ++lv_tps_n;
}

static int lv_tps_class(const LvFind &f)
{
    for (int h = 0; h < LV_NHUNT; ++h) if (lv_hunt_match(h, f)) return h;
    return -1;
}

static void lv_tps_log_find(const LvFind &f)
{
    const int h = lv_tps_class(f);
    const char *radio = f.kind == LV_K_WIFI ? "WIFI" : "BLE";
    if (h >= 0) {
        char up[16]; snprintf(up, sizeof up, "%s", LV_HUNT_NAME[h]); lv_upcase(up);
        if (f.kind == LV_K_WIFI && f.ch)
            lv_tps_add(2, "> %s %s detected %ddBm ch%d", radio, up, (int)f.rssi, (int)f.ch);
        else
            lv_tps_add(2, "> %s %s detected %ddBm", radio, up, (int)f.rssi);
    } else {
        char lab[20]; lv_label(f, lab, sizeof lab);
        if (f.kind == LV_K_WIFI && f.ch)
            lv_tps_add(2, "> %s %s %ddBm ch%d", radio, lab, (int)f.rssi, (int)f.ch);
        else
            lv_tps_add(2, "> %s %s %ddBm", radio, lab, (int)f.rssi);
    }
}

static void lv_tps_pump(void)
{
    /* Append-only: typist budget keeps running so new finds type out, they do not rewind the log. */
    if (lv_tps_phase == 0 && lv_job != LV_J_NONE) {
        if (lv_tps_n > 0 && lv_tps_kind[lv_tps_n - 1] == 1) --lv_tps_n;   /* drop idle prompt */
        lv_tps_add(1, "PS C:\\BOTcorps> Get-AreaScan");
        lv_tps_add(3, "scanning wifi/ble...");
        lv_tps_phase = 1;
    }
    while (lv_tps_emit < lv_nf) { lv_tps_log_find(lv_f[lv_tps_emit]); ++lv_tps_emit; }
    if (lv_tps_phase == 1 && lv_job == LV_J_NONE && lv_tps_emit > 0) {
        lv_tps_add(3, "scan done.  SAMPLE");
        lv_tps_add(1, "PS C:\\BOTcorps>");
        lv_tps_phase = 2;
    }
}

static int lv_tps_budget(void)
{
    /* Operator type-out: 40 ms/char. Built from our typist, not the missing Adv mock. */
    const int dt = (int)(lv_now - lv_tps_t0);
    return dt > 0 ? dt / 40 : 0;
}

static uint16_t lv_tps_rain_px(uint16_t g, int x, int y)   /* lv_tg_rain law (lv_port.h), green, edge columns, slower */
{
    static const char GL[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ#$%*+<>/|";
    const int ngl = (int)sizeof(GL) - 1, CW = 4, CH = 6, GW = 3, GH = 5;
    const int y0 = 24, y1 = PAD_Y0 - 1, h = y1 - y0 + 1;
    if (y < y0 || y > y1) return g;
    int col, cx, local_x;
    if (x < LV_TPS_X0) { local_x = x; col = local_x / CW; cx = local_x % CW; }
    else if (x > LV_TPS_X1) { local_x = x - (LV_TPS_X1 + 1); col = 8 + local_x / CW; cx = local_x % CW; }
    else return g;
    if (cx > GW - 1 || col < 0) return g;
    const uint32_t s = 0xC0DEU ^ (uint32_t)(col * 2654435761u);   /* same seed law as face rain */
    const int speed = 1;   /* low power: no 2-3x fall */
    const int trail = 9 + (int)((s >> 3) % 7);
    const int period = h + trail * CH + 24;
    const int off = (int)((s >> 16) % (uint32_t)period);
    const int phase = (int)((lv_now + (uint32_t)(lv_tps_speed - 1) * (lv_now / 2u)) / 60u);
    const int head_y = (phase * speed + off) % period;
    const int ly = y - y0;
    uint16_t out = g;
    for (int t = 0; t < trail; ++t) {
        const int gy = head_y - t * CH;
        if (gy > h - 1 || gy + GH - 1 < 0) continue;
        if (ly < gy || ly > gy + GH - 1) continue;
        const unsigned k = (unsigned)(s + (unsigned)((gy + period) / CH) * 17u + (unsigned)col * 3u);
        unsigned uc = (unsigned char)GL[k % (unsigned)ngl];
        if (uc < 32 || uc > 126) uc = 32;
        if (LV_F35[uc - 32] >> (14 - ((ly - gy) * 3 + cx)) & 1)
            out = t == 0 ? LV_TPS_MXH : (t < 3 ? LV_TPS_MX : (t < 6 ? LV_TPS_MXD : LV_TPS_MXK));
    }
    return out;
}

static void lv_tps_pins(LvBand &B)
{
    const uint16_t fill = lv_thc_pin_fill_px(), top = lv_thc_pin_top_px();
    /* Locked TERM pin columns (x 6-12 / 227-233), clipped above the strip. Power-rail colours are
     * not invented here: every pad uses the live theme sample, same path as home TERMINALbot. */
    const int y0 = 32, y1 = PAD_Y0 - 3;
    for (int y = y0; y + 3 <= y1; y += 7) {
        b_fill(B, 6, y, 7, 1, top);     b_fill(B, 6, y + 1, 7, 3, fill);
        b_fill(B, 227, y, 7, 1, top);   b_fill(B, 227, y + 1, 7, 3, fill);
    }
}

static void lv_tps_strip(LvBand &B)
{
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD);
    b_fill(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, lv_hex(0x000000));
    b_frame(B, PAD_X0, PAD_Y0, PAD_X1 - PAD_X0 + 1, PAD_Y1 - PAD_Y0 + 1, ink);
    pad_vias_frame(B, ink);
    pad_plaque(B, ink, dim);
    const char *st = lv_job != LV_J_NONE ? "SCANNING  SAMPLE" : (lv_nf ? "LAST SCAN  SAMPLE" : "IDLE  SAMPLE");
    b_text35(B, 10, 253, st, lv_hex(0xffffff));
    b_text35(B, 10 + 140, 253, "A SCAN  B BACK", dim);
    const uint16_t ac = ink, ice = lv_hex(0x30e8ff);   /* A uses live theme accent, not hard-coded grape */
    const int keep = lv_rd_pad_held; lv_rd_pad_held = lv_tps_held == 1 || lv_tps_held == 2 ? lv_tps_held : 0;
    pad_octagon(B, PAD_CX, PAD_CY, 42, 25, lv_hex(0x06040a), dim, 0);
    pad_glow_oct(B, PAD_CX, PAD_CY, 42, 25, ink, 2);
    pad_octagon(B, PAD_CX, PAD_CY, 36, 20, lv_hex(0x100818), ink, 0);
    pad_rocker(B, PAD_CX, PAD_CY, 16, 68, lv_hex(0x1a1018), ink);
    pad_disc(B, PAD_CX, PAD_CY, 3, ink, ink, 0);
    auto arcade = [&](int cx, int cy, uint16_t col, const char *lab, const char *sub, int held) {
        pad_disc(B, cx, cy, 20, lv_hex(0x08040c), col, 0);
        pad_glow(B, cx, cy, 20, held ? lv_hi(col) : col, held ? 6 : 3);
        pad_disc(B, cx, cy, 15, held ? lv_mix(lv_hex(0x100818), col, 5, 8) : lv_hex(0x120818), col, held);
        b_text35(B, cx - 1, cy - 6, lab, held ? lv_hex(0xffffff) : col);
        b_text35(B, cx - lv_w35(sub) / 2, cy + 3, sub, held ? lv_hex(0xffffff) : lv_dim(col, 6, 8));
    };
    arcade(172, PAD_CY, ice, "B", "BACK", lv_tps_held == 6);
    arcade(216, PAD_CY, ac,  "A", "SCAN", lv_tps_held == 5);
    const int ks = lv_rd_speed; lv_rd_speed = lv_tps_speed; lv_rd_pad_held = lv_tps_held == 10 ? 10 : 0;
    pad_draw_speed(B);
    lv_rd_speed = ks; lv_rd_pad_held = keep;
}

static void lv_tps_banner(LvBand &B)
{
    const uint16_t ink = lv_ink(LV_ACC), dim = lv_ink(LV_ACCD);
    b_fill(B, 0, 0, 240, 24, lv_hex(0x000000));
    b_frame(B, LV_R_BANNER_BACK.x0, LV_R_BANNER_BACK.y0,
            LV_R_BANNER_BACK.x1 - LV_R_BANNER_BACK.x0 + 1,
            LV_R_BANNER_BACK.y1 - LV_R_BANNER_BACK.y0 + 1, ink);
    b_text35(B, 4, 10, "BACK", ink);
    b_text35(B, 38, 4, "TERMINAL // AREA SCAN", lv_hex(0xffffff));
    b_text35(B, 38, 13, "LOW PWR", dim);
    /* dim backlight indicator */
    const uint16_t led = lv_hex(0x5A4A10);
    b_fill(B, 214, 6, 18, 6, lv_hex(0x1A1408));
    b_frame(B, 214, 6, 18, 6, dim);
    b_fill(B, 216, 8, 8, 2, led);
    b_text35(B, 190, 13, "DIM", led);
    b_text35(B, 190, 4, "SAMPLE", LV_TPS_MXD);
}

static void lv_tps_console(LvBand &B)
{
    const int x0 = LV_TPS_X0, y0 = LV_TPS_Y0, w = LV_TPS_X1 - LV_TPS_X0 + 1, h = LV_TPS_Y1 - LV_TPS_Y0 + 1;
    /* grey DIP lip, then gold terminal box — locked LV_THC_* inks, not invented */
    b_fill(B, x0 - 2, y0 - 2, w + 4, h + 4, LV_THC_BODY);
    b_fill(B, x0 - 2, y0 - 2, w + 4, 1, LV_THC_BODY_HI);
    b_fill(B, x0 - 2, y0 - 2, 1, h + 4, LV_THC_BODY_HI);
    b_fill(B, x0 - 2, y0 + h + 1, w + 4, 1, LV_THC_BODY_LO);
    b_fill(B, x0 + w + 1, y0 - 2, 1, h + 4, LV_THC_BODY_LO);
    b_fill(B, x0, y0, w, h, LV_THC_BOX_FILL);
    b_frame(B, x0, y0, w, h, LV_THC_BOX_INK);
    b_frame(B, x0 + 1, y0 + 1, w - 2, h - 2, LV_THC_BOX_HOT);
    b_fill(B, x0 + 3, y0 + 3, w - 6, h - 6, LV_TPS_PSBG);
    /* faint inner header bar */
    b_fill(B, x0 + 3, y0 + 3, w - 6, 7, LV_TPS_PSBG2);
    b_text35(B, x0 + 5, y0 + 4, "PS  C:\\BOTcorps", LV_TPS_PROMPT);

    lv_tps_pump();
    const int rows = (LV_TPS_Y1 - 4 - LV_TPS_TY) / LV_TPS_ROW;
    int budget = lv_tps_budget();
    int shown = 0, cur_x = LV_TPS_TX, cur_y = LV_TPS_TY, last_kind = 0;
    /* count how many lines the typist has opened, then scroll so the typing line is visible */
    int open = 0, remain = budget;
    for (int i = 0; i < lv_tps_n; ++i) {
        const int L = (int)strlen(lv_tps_line[i]);
        ++open;
        if (remain <= L) break;
        remain -= L;
    }
    const int start = open > rows ? open - rows : 0;
    remain = budget;
    for (int i = 0; i < lv_tps_n; ++i) {
        const int L = (int)strlen(lv_tps_line[i]);
        const int take = remain < L ? (remain < 0 ? 0 : remain) : L;
        remain -= L;
        if (i < start) continue;
        if (shown >= rows) break;
        const int y = LV_TPS_TY + shown * LV_TPS_ROW;
        uint16_t col = LV_TPS_INK;
        if (lv_tps_kind[i] == 1) col = LV_TPS_PROMPT;
        else if (lv_tps_kind[i] == 2) col = LV_TPS_MX;
        else if (lv_tps_kind[i] == 3) col = lv_hex(0x6A8498);
        if (take > 0) {
            char tmp[LV_TPS_LSZ]; memcpy(tmp, lv_tps_line[i], (size_t)take); tmp[take] = 0;
            b_text(B, LV_TPS_TX, y, tmp, col);
        }
        cur_x = LV_TPS_TX + take * LV_TPS_CW;
        cur_y = y;
        last_kind = lv_tps_kind[i];
        ++shown;
        if (remain < 0) break;
    }
    if ((lv_now / 500) & 1) {   /* existing cursor-blink law */
        const uint16_t cc = last_kind == 2 ? LV_TPS_MX : LV_TPS_PROMPT;
        b_fill(B, cur_x, cur_y + 6, 6, 2, cc);
    }
}

static void lv_tps_scene(LvBand &B, void *)
{
    /* field behind the gold box: black, then Matrix rain in the edge gutters */
    b_fill(B, B.x0, B.y0, B.w, B.h, lv_hex(0x000000));
    const int y0 = B.y0, y1 = B.y0 + B.h - 1;
    for (int y = y0; y <= y1; ++y) {
        if (y < 24 || y >= PAD_Y0) continue;
        for (int x = B.x0; x <= B.x0 + B.w - 1; ++x) {
            if (x >= LV_TPS_X0 && x <= LV_TPS_X1) continue;
            const uint16_t c = lv_tps_rain_px(0, x, y);
            if (c) b_fill(B, x, y, 1, 1, c);
        }
    }
    lv_tps_pins(B);
    lv_tps_banner(B);
    lv_tps_console(B);
    lv_tps_strip(B);
}

static void lv_tps_frame(void)
{
    lv_tps_held = 0;
    lv_render(lv_blit, (LvRect){0, 0, 239, 319}, lv_tps_scene, 0);
}

static void lv_tps_enter(void)
{
    lv_tps_t0 = lv_now;
    lv_tps_held = 0;
    lv_tps_n = 0;
    lv_tps_emit = 0;
    lv_tps_phase = 0;
    lv_tps_add(0, "Windows PowerShell");
    lv_tps_add(3, "Copyright (C) BOTcorps  SAMPLE");
    lv_tps_add(3, "Low-power area scan.  EX.DATA");
    lv_tps_add(0, "");
    lv_tps_add(1, "PS C:\\BOTcorps>");
}

static const char *lv_tps_tap(int x, int y)
{
    if (y >= PAD_Y0) {
        int id = 0;
        if (x >= PAD_MID_X && x < PAD_MID_X + PAD_MID_W && y >= PAD_MID_YH && y < PAD_MID_YW + PAD_MID_H) id = 10;
        else if (lv_in(PAD_HIT_L, x, y)) id = 1; else if (lv_in(PAD_HIT_R, x, y)) id = 2;
        else if (lv_in(PAD_HIT_B, x, y)) id = 6;
        else if (lv_in(PAD_HIT_A, x, y) || (x >= 196 && y >= 272)) id = 5;
        lv_tps_held = id;
        if (id == 10) { lv_tps_speed = lv_tps_speed % 3 + 1; return "tps-speed"; }
        if (id == 6) return lv_back();
        if (id == 5) {
            if (lv_job != LV_J_NONE) { lv_job_stop(); return "tps-stop"; }
            lv_job_start(LV_J_ALL, -1);
            return "tps-scan";
        }
        return id ? "tps-pad" : 0;
    }
    return 0;
}

#endif /* LV_TERM_PS */
