/* hub9 (CYD only): the Defend play field on the GCHAT board, between the bot chip and the 3-line plate.
 * Engine coords (dl_xy / s_dl_rx,ry: x 116..268, y 58..134) map linearly into the field's play area.
 * Banners: CHARGING nn% while the bolt charges; ROUND OVER (score + RESTART / CLOSE, 28px buttons) stays
 * until it is answered or dismissed. Field taps echo the word into the plate prompt; a second tap runs it. */
#ifdef BOTCORPS_CYD
static bool s_cyd_dl_over = false;   /* the round ran out on GCHAT; cleared by restart / close / leaving */
enum { CDL_HDR = 12, CDL_BTN = 28 };

static void cdl_rect(int *x, int *y, int *w, int *h)
{
    if (botcorps_cyd_orient) { *x = 38; *y = 104; *w = 164; *h = 116; }   /* P: 38..202 (symmetric), above the plate pads */
    else { *x = 98; *y = 30; *w = 188; *h = 110; }                        /* L: right of the chip, above the reticle pad */
}
static void cdl_play(int *x, int *y, int *w, int *h)
{
    int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
    *x = fx + 8; *y = fy + CDL_HDR + 8; *w = fw - 16; *h = fh - CDL_HDR - 14;
}
static void cdl_map(int ex, int ey, int *sx, int *sy)
{
    int px, py, pw, ph; cdl_play(&px, &py, &pw, &ph);
    *sx = px + (ex - 116) * pw / 152;
    *sy = py + (ey - 58) * ph / 76;
}
static void cdl_unmap(int sx, int sy, int *ex, int *ey)
{
    int px, py, pw, ph; cdl_play(&px, &py, &pw, &ph);
    *ex = 116 + (sx - px) * 152 / (pw > 0 ? pw : 1);
    *ey = 58 + (sy - py) * 76 / (ph > 0 ? ph : 1);
    if (*ex < 116) *ex = 116;
    if (*ex > 268) *ex = 268;
    if (*ey < 58) *ey = 58;
    if (*ey > 134) *ey = 134;
}
static bool cdl_over(void) { return s_cyd_dl_over && !s_dl_on && s_screen == SCR_GCHAT; }
/* ROUND OVER box + its two buttons (screen coords) */
static void cdl_over_box(int *bx, int *by, int *bw, int *bh)
{
    int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
    *bw = fw - 16 < 156 ? fw - 16 : 156; *bh = 66;
    *bx = fx + (fw - *bw) / 2;
    *by = fy + CDL_HDR + (fh - CDL_HDR - *bh) / 2;
}
static void cdl_btn(int k, int *x, int *y, int *w, int *h)
{
    int bx, by, bw, bh; cdl_over_box(&bx, &by, &bw, &bh);
    const int cw = 44;
    *y = by + bh - CDL_BTN - 5; *h = CDL_BTN;
    if (k == 0) { *x = bx + 5; *w = bw - 10 - cw - 4; }   /* RESTART */
    else { *x = bx + bw - 5 - cw; *w = cw; }               /* CLOSE */
}
static bool cdl_in(int tx, int ty, int x, int y, int w, int h) { return tx >= x && tx < x + w && ty >= y && ty < y + h; }
static bool cdl_prompt_is(const char *w)
{
    const char *b = pt_buf(nullptr);
    return b && strcmp(b, w) == 0;
}
static void cdl_kind_name(int kind, char *out, size_t cap)
{
    const int bot = dl_bot();
    out[0] = 0;
    for (int k = 0; k < ZP_NAMES; ++k) {
        const int n = (k + 12) % ZP_NAMES;   /* the kind labels first, then the bot names */
        if (zp_kind(n) == kind && zp_own(bot, n)) { zp_name(n, out, cap); return; }
    }
}
static void cdl_btn_paint(Gfx g, int k, const char *lab, bool sel)
{
    int x, y, w, h; cdl_btn(k, &x, &y, &w, &h);
    g.fillRect(x, y, w, h, sel ? (uint8_t)PAL_WHITE : (k == 0 ? dl_accent() : (uint8_t)PAL_DEEP));
    g.drawRect(x, y, w, h, sel ? dl_accent() : (uint8_t)PAL_WHITE);
    OVL_BOX("CHIP", lab, x, y, w, h);
    txt(g, lab, x + w / 2, y + (h - 8) / 2, sel || k == 0 ? (uint8_t)PAL_BLACK : (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
}

static void cyd_dl_paint(Gfx g)
{
    const uint32_t t = millis();
    const bool live = dl_live();
    if (live) s_cyd_dl_over = false;
    const uint8_t bc = dl_accent();
    int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
    const int keep = s_px; use_px(g, 6);
    g.fillRect(fx, fy, fw, fh, (uint8_t)PAL_BLACK);
    g.drawRect(fx, fy, fw, fh, (uint8_t)GOLD);
    g.drawRect(fx + 1, fy + 1, fw - 2, fh - 2, bc);
    OVL_BOX("CHIP", "FIELD", fx, fy, fw, fh);
    /* header strip: time | charge bar | score */
    g.fillRect(fx + 2, fy + 2, fw - 4, CDL_HDR, (uint8_t)PAL_DEEP);
    char a[24], s[24];
    const int tl = live ? dl_time_left(t) : 0;
    snprintf(a, sizeof a, "%02ds", (tl + 999) / 1000);
    snprintf(s, sizeof s, "%d", s_dl_score);
    txt(g, a, fx + 5, fy + 4, (uint8_t)PAL_WHITE, lgfx::textdatum::top_left);
    txt(g, s, fx + fw - 5, fy + 4, (uint8_t)PAL_WHITE, lgfx::textdatum::top_right);
    {
        const int ch = live ? dl_charge_at(t) : 0;
        const int bx = fx + 5 + 3 * 6 + 6, bw = fw - 10 - 3 * 6 - 6 - 6 * 6 - 6, by = fy + 5;
        g.drawRect(bx, by, bw, 6, (uint8_t)PAL_WHITE);
        const int fillw = (bw - 2) * ch / DL_CHG;
        if (fillw > 0) g.fillRect(bx + 1, by + 1, fillw, 4, ch >= DL_CHG ? (uint8_t)PAL_OK : bc);
        for (int k = 1; k < 3; ++k) g.fillRect(bx + (bw * k) / 3, by + 1, 1, 4, (uint8_t)PAL_WHITE);   /* fire thresholds */
    }
    int px, py, pw, ph; cdl_play(&px, &py, &pw, &ph);
    /* faint grid */
    const uint8_t grid = botcorps_pal_fit(mix16(0x0000, botcorps_rgb565(bc), 22, 100));
    for (int x = px; x <= px + pw; x += pw / 6) g.fillRect(x, py - 4, 1, ph + 8, grid);
    for (int y = py; y <= py + ph; y += ph / 4) g.fillRect(px - 4, y, pw + 8, 1, grid);
    int cx, cy; cdl_map(190, 96, &cx, &cy);
    if (live) {
        const uint8_t copper = botcorps_pal_fit(mix16(0xC300, botcorps_rgb565(bc), 40, 100));
        for (int i = 0; i < DL_MAX; ++i) {
            if (!s_dl_d[i].live) continue;
            int ex, ey; dl_xy(i, &ex, &ey);
            int x, y; cdl_map(ex, ey, &x, &y);
            g.fillRect(x < cx ? x : cx, cy, (x > cx ? x - cx : cx - x) + 1, 1, copper);
            g.fillRect(x, y < cy ? y : cy, 1, (y > cy ? y - cy : cy - y) + 1, copper);
        }
    }
    if (g.raw()) botcorps_sprite_draw(*g.raw(), cx - g.ox, cy - g.oy, BOTCORPS_SHAPE_ROUND, 2, bc, false);
    if (live) {
        for (int i = 0; i < DL_MAX; ++i) {
            if (!s_dl_d[i].live) continue;
            int ex, ey; dl_xy(i, &ex, &ey);
            int x, y; cdl_map(ex, ey, &x, &y);
            gl_icon(g, s_dl_d[i].kind, x - 6, y - 6, false);
            if (s_dl_d[i].acq) g.drawRect(x - 8, y - 8, 17, 17, (uint8_t)PAL_WHITE);
            for (int k = 0; k < s_dl_d[i].hp && k < 3; ++k) g.fillRect(x - 5 + k * 4, y + 8, 3, 2, (uint8_t)PAL_OK);
        }
        int rx, ry; cdl_map(s_dl_rx, s_dl_ry, &rx, &ry);
        g.drawRect(rx - 7, ry - 7, 15, 15, bc);
        g.fillRect(rx - 2, ry, 5, 1, (uint8_t)PAL_WHITE);
        g.fillRect(rx, ry - 2, 1, 5, (uint8_t)PAL_WHITE);
        if (s_dl_bolt && (uint32_t)(t - s_dl_bolt) < 250u) g.drawLine(cx, cy, rx, ry, (uint8_t)PAL_WHITE);
        const int ch = dl_charge_at(t);
        if (ch < DL_CHG) {   /* CHARGING banner: under the header, inside the field */
            const int bx = fx + 4, by = fy + CDL_HDR + 4, bw = fw - 8, bh = 12;
            g.fillRect(bx, by, bw, bh, bc);
            g.drawRect(bx, by, bw, bh, (uint8_t)PAL_WHITE);
            OVL_BOX("CHIP", "CHARGING", bx, by, bw, bh);
            snprintf(a, sizeof a, "CHARGING %d%%", ch * 100 / DL_CHG);
            txt(g, a, bx + bw / 2, by + 2, (uint8_t)PAL_BLACK, lgfx::textdatum::top_center);
        }
    } else if (cdl_over()) {
        int bx, by, bw, bh; cdl_over_box(&bx, &by, &bw, &bh);
        g.fillRect(bx, by, bw, bh, (uint8_t)PAL_DEEP);
        g.drawRect(bx, by, bw, bh, (uint8_t)PAL_WHITE);
        g.drawRect(bx + 1, by + 1, bw - 2, bh - 2, bc);
        OVL_BOX("CHIP", "ROUND OVER", bx, by, bw, bh);
        txt(g, "ROUND OVER", bx + bw / 2, by + 5, (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
        snprintf(a, sizeof a, "SCORE %d HI %d", s_dl_score, s_dl_hi);
        txt(g, a, bx + bw / 2, by + 16, bc, lgfx::textdatum::top_center);
        cdl_btn_paint(g, 0, "RESTART", cdl_prompt_is("restart"));
        cdl_btn_paint(g, 1, "X", false);
    } else {
        const int bw = fw - 16, bh = 30, bx = fx + 8, by = fy + CDL_HDR + 8;
        g.fillRect(bx, by, bw, bh, (uint8_t)PAL_DEEP);
        g.drawRect(bx, by, bw, bh, cdl_prompt_is("restart") ? (uint8_t)PAL_WHITE : bc);
        OVL_BOX("CHIP", "IDLE", bx, by, bw, bh);
        txt(g, "NO ROUND", bx + bw / 2, by + 4, (uint8_t)PAL_WHITE, lgfx::textdatum::top_center);
        txt(g, "TAP: RESTART", bx + bw / 2, by + 17, bc, lgfx::textdatum::top_center);
    }
    s_px = keep;
}

/* restart: first tap echoes the word into the prompt, the second tap (or Enter / RUN) runs it */
static void cdl_restart_tap(void)
{
    int cap = 0; char *b = pt_buf(&cap);
    if (cdl_prompt_is("restart")) {
        b[0] = 0;
        zap_try_cmd("restart");
        s_cyd_dl_over = false;
    } else {
        snprintf(b, (size_t)cap, "restart");
        s_cyd_adv_focus = true;
    }
    s_dirty = true;
}
static bool cyd_dl_touch(int tx, int ty)
{
    if (s_screen != SCR_GCHAT) return false;
    int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
    if (!cdl_in(tx, ty, fx, fy, fw, fh)) return false;
    int cap = 0; char *b = pt_buf(&cap);
    if (dl_live()) {
        int ex, ey; cdl_unmap(tx, ty, &ex, &ey);
        int best = -1, bd = 1 << 30;
        for (int i = 0; i < DL_MAX; ++i) {
            if (!s_dl_d[i].live) continue;
            int x, y; dl_xy(i, &x, &y);
            int sx, sy; cdl_map(x, y, &sx, &sy);
            const int d = (sx - tx) * (sx - tx) + (sy - ty) * (sy - ty);
            if (d < bd) { bd = d; best = i; }
        }
        if (best >= 0 && bd <= 16 * 16) {   /* a device under the finger: aim there, echo "zap <name>" */
            int x, y; dl_xy(best, &x, &y);
            s_dl_rx = x; s_dl_ry = y;
            char nm[16], w[32];
            cdl_kind_name(s_dl_d[best].kind, nm, sizeof nm);
            snprintf(w, sizeof w, "zap %s", nm);
            if (nm[0] && cdl_prompt_is(w)) { b[0] = 0; zap_try_cmd(w); }
            else if (nm[0]) { snprintf(b, (size_t)cap, "%s", w); s_cyd_adv_focus = true; }
        } else { s_dl_rx = ex; s_dl_ry = ey; }   /* empty field: the reticle moves there */
        s_dirty = true;
        return true;
    }
    if (cdl_over()) {
        int x, y, w, h;
        cdl_btn(0, &x, &y, &w, &h);
        if (cdl_in(tx, ty, x, y, w, h)) { cdl_restart_tap(); return true; }
        cdl_btn(1, &x, &y, &w, &h);
        if (cdl_in(tx, ty, x, y, w, h)) {
            s_cyd_dl_over = false;
            if (cdl_prompt_is("restart")) b[0] = 0;
            s_dirty = true;
            return true;
        }
        return true;   /* the banner stays until it is answered */
    }
    cdl_restart_tap();   /* idle field: tap = restart (echo, then run) */
    return true;
}
/* GCHAT chip grid (hub9): verbs -> own device names -> subs, from the same tables zap_try_cmd checks.
 * Returns -1 when the line is not a Defend line (the generic command grid takes it). */
static int cyd_dl_tokens(const char *b, const char **tok, const char **sub, int max, char *head, size_t hcap)
{
    static char nm[ZP_NAMES][12];
    const int bot = dl_bot();
    if (s_screen != SCR_GCHAT || bot < 2 || bot > 12) return -1;
    while (*b == ' ') ++b;
    int n = 0;
    auto put = [&](const char *t, const char *d) { if (n < max && t && t[0]) { tok[n] = t; sub[n] = d; ++n; } };
    const char *sp = strchr(b, ' ');
    if (!sp) {   /* no verb yet: Defend verbs first, then the generic commands */
        snprintf(head, hcap, "DEFEND");
        for (int k = 0; k < 5; ++k) put(ZP_CORE[k], "defend verb");
        put(ZP_SIG[bot], "signature verb");
        return n;   /* caller appends the generic verbs */
    }
    char verb[12];
    const int vw = (int)(sp - b);
    if (vw >= (int)sizeof verb) return -1;
    memcpy(verb, b, (size_t)vw); verb[vw] = 0;
    bool core = false;
    for (int k = 0; k < 5; ++k) if (!strcmp(verb, ZP_CORE[k])) core = true;
    const bool sig = ZP_SIG[bot] && !strcmp(verb, ZP_SIG[bot]);
    if (!core && !sig) return -1;
    const char *rest = sp + 1;
    while (*rest == ' ') ++rest;
    if (!strcmp(verb, "restart")) { snprintf(head, hcap, "restart > RUN"); return 0; }
    if (!strcmp(verb, "overload")) {
        if (rest[0] && strchr(rest, ' ')) { snprintf(head, hcap, "overload > RUN"); return 0; }
        snprintf(head, hcap, "overload > sub / RUN");
        put(ZP_SUB[bot][0], "sub"); put(ZP_SUB[bot][1], "sub");
        return n;
    }
    int name = -1;
    const int l = zp_match(rest, &name);
    if (l && rest[l] == ' ') {   /* name done */
        const char *after = rest + l + 1;
        while (*after == ' ') ++after;
        if (after[0] && strchr(after, ' ')) { snprintf(head, hcap, "%s > RUN", verb); return 0; }
        snprintf(head, hcap, "%s > sub / RUN", verb);
        put(ZP_SUB[bot][0], "sub"); put(ZP_SUB[bot][1], "sub");
        return n;
    }
    snprintf(head, hcap, "%s > device", verb);
    for (int k = 0; k < ZP_NAMES; ++k) {
        const int m = (k + 12) % ZP_NAMES;
        if (!zp_own(bot, m)) continue;
        zp_name(m, nm[m], sizeof nm[m]);
        put(nm[m], m < 12 ? "bot" : "device");
    }
    return n;
}

/* Plate Enter / RUN on GCHAT: Defend lines go to the game before the generic command table. */
static bool cyd_dl_line(void)
{
    if (s_screen != SCR_GCHAT) return false;
    char *b = pt_buf(nullptr);
    if (!b || !b[0]) return false;
    if (!zap_try_cmd(b)) return false;
    b[0] = 0;
    s_pt_mode = 0;
    s_sheet = false;
    if (cyd_cl_on()) cyd_cl_close();
    if (dl_live()) s_cyd_dl_over = false;
    s_dirty = true;
    return true;
}
/* CYD tick: step the round, flag its end, repaint the live field a few times a second */
static void cyd_dl_tick(uint32_t t)
{
    static uint32_t last = 0;
    const bool was = dl_live();
    dl_step(t);
    if (was && !s_dl_on) {
        s_cyd_dl_over = true;
        char row[40];
        snprintf(row, sizeof row, "[ -- ] ROUND OVER: %d", s_dl_score);
        zp_out(row);
        s_dirty = true;
    }
    if (dl_live() && (uint32_t)(t - last) >= 200u) { last = t; s_dirty = true; }
}
#endif
