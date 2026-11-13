/* ---- v13.1 HD monolith skin (every pillar; drawn over Gibson's own corners from lv_rd_place, far -> near, so it never moves anything) ---- */
static void mo13_px(LvBand &B, int x, int y, uint16_t c, int a) { if (y >= LV_RD_Y0 + 1 && y <= LV_RD_Y1) mo_bl(B, x, y, c, a); }
static void mo13_line(LvBand &B, int x, int y, int xx, int yy, uint16_t c, int a)   /* crisp Bresenham, field-clipped */
{ int dx = abs(xx - x), sx = x < xx ? 1 : -1, dy = -abs(yy - y), sy = y < yy ? 1 : -1, e = dx + dy;
  for (;;) { mo13_px(B, x, y, c, a); if (x == xx && y == yy) break; int e2 = 2 * e; if (e2 >= dy) { e += dy; x += sx; } if (e2 <= dx) { e += dx; y += sy; } } }
static void mo13_span(LvBand &B, int y, int xa, int xb, uint16_t c, int a) { if (y < LV_RD_Y0 + 1 || y > LV_RD_Y1) return; mo_hspan(B, y, xa, xb, c, a); }
static uint32_t mo13_icol(char ch)   /* LV_ICON_ART inks -> Dojo: outline ink, parchment body / shade / highlight, fixed inks Matrix green */
{ return ch == 'k' ? 0x1e1a16 : ch == 'a' ? 0xe8dec8 : ch == 'b' ? 0xb8a47c : ch == 'c' ? 0xffffff : 0x00a83c; }
static void mo13_icon(LvBand &B, int i, int x0, int y0, int S, int a)   /* integer nearest-neighbour: S = 8 (exact 2:1), 16, 32, 48 */
{
    for (int y = 0; y < S; ++y) for (int x = 0; x < S; ++x) { const int r = y * 16 / S, c = x * 16 / S; char ch;
        if (MO_ICON[i] < 0) ch = MO_DEAUTH[r * 12 / 16][c * 12 / 16] == '.' ? '.' : 'g'; else ch = LV_ICON_ART[MO_ICON[i]][r][c];
        if (ch != '.') mo13_px(B, x0 + x, y0 + y, lv_hex(mo13_icol(ch)), a); }
}
/* 3x5 kind glyphs for far pillars (too narrow for an 8 px icon): N networks, C cameras, T trackers, B BLE tools */
static const char *const MO13_GLY[4][5] = {{"#.#", "###", "###", "#.#", "#.#"}, {"###", "#..", "#..", "#..", "###"}, {"###", ".#.", ".#.", ".#.", ".#."}, {"##.", "#.#", "##.", "#.#", "##."}};
static void mo12_bigbuild(int i) { (void)i; }   /* no blurred ghost sprite */
static uint16_t mo13_mix(uint32_t a, uint32_t b, int k, int n) { return lv_mix(lv_hex(a), lv_hex(b), k < 0 ? 0 : k > n ? n : k, n); }
/* one shoji pane: 1 px lit top + left edge, two-tone inner shading (light upper half, deeper lower half), dark sill */
static void mo13_pane(LvBand &B, int xa, int ya, int xb, int yb, int kind, int a)
{
    if (xb < xa || yb < ya) return;
    const uint32_t lit[3] = {0xf2e6c8, 0x5a4a34, 0x2fbf6a}, low[3] = {0xd2c09a, 0x3a2e20, 0x158a44}, edge[3] = {0xfffaf0, 0x7a6448, 0x8affb8};
    for (int y = ya; y <= yb; ++y) mo13_span(B, y, xa, xb, lv_hex(y - ya <= (yb - ya) / 2 ? lit[kind] : low[kind]), a);
    if (yb - ya >= 2 && xb - xa >= 1) { mo13_span(B, ya, xa, xb, lv_hex(edge[kind]), a); mo13_line(B, xa, ya, xa, yb, lv_hex(edge[kind]), a); mo13_span(B, yb, xa, xb, lv_hex(0x1a140e), a); }
}
static void mo11_tower_fx(LvBand &B, const LvTower &T)
{
    const int i = T.i; if (i < 0 || i >= 12) return;
    const bool open = i == itw_mo_sel || (mo_closing && i == mo_last); const float cp = open ? mo12_p(i) : 0.0f;
    const int a = open ? 256 : mo11_fade();   /* others wash toward the parchment while open */
    const int x0 = T.x0, x1 = T.x1, yt = T.yt, yb = T.yb, fw = x1 - x0;
    /* close-up: deepen the pillar along its own receding edges (x and y extrapolated on the same rays) so the side + cap read as real 3D */
    const float k = 1.0f + 1.4f * cp;
    const int bx0 = x0 + (int)lroundf((T.bx0 - x0) * k), bx1 = x1 + (int)lroundf((T.bx1 - x1) * k), byt = yt + (int)lroundf((T.byt - yt) * k), byb = yb + (int)lroundf((T.byb - yb) * k);
    const uint16_t PAP = lv_hex(MO11_PAPER), DK = lv_hex(0x100c09), G = lv_hex(MO11_G), GH = lv_hex(MO11_GH);
    const int pitch = fw >= 40 ? 7 : fw >= 14 ? 5 : 4;   /* pane row pitch (front + side share it so the rows line up round the corner) */
    const int S = fw >= 60 ? 48 : fw >= 38 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 8 : 0, wy0 = S ? yt + 3 + S + 8 : yt + (fw >= 7 ? 10 : 3);
    /* side face: scanned per column; timber graded from the lit corner (front) to dark (back), perspective pane rows + posts */
    auto side = [&](int xf, int xbk) {
        const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n;
            const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            const uint16_t base = mo13_mix(0x7a6342, 0x241b12, s, n);   /* graded shading */
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, base, a); }
        /* perspective shoji ribs: one per pane row, a dark kumiko line + a lit line under it, both on the receding rays */
        if (n >= 3) for (int yf = wy0; yf < yb - 1; yf += 2 * pitch) { const float v = (yf - yt) / (float)(yb - yt);
            const int ya = yf, yb2 = (int)lroundf(byt + (byb - byt) * v);
            mo13_line(B, xf + dir, ya, xbk, yb2, lv_hex(0x1a140e), a); mo13_line(B, xf + dir, ya + 1, xbk, yb2 + 1, lv_hex(0xa88e62), a * 2 / 3); }
        if (n >= 8) for (int q = 1; q <= 2; ++q) { const int s = n * q / 3, x = xf + dir * s; const float u = s / (float)n;   /* posts */
            mo13_line(B, x, (int)lroundf(yt + (byt - yt) * u), x, (int)lroundf(yb + (byb - yb) * u), lv_hex(0x1e1610), a); }
        mo13_line(B, xf + dir, yt + 1, xf + dir, yb - 1, lv_hex(0xd8c6a0), a);          /* lit corner edge */
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x120e0a), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xcbb894), a); mo13_line(B, xf, yb, xbk, byb, DK, a);
    };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    /* top cap: lit timber with an inset darker panel and a bright front lip */
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xa88e62), a);
        if (yt - byt >= 4) { const float cx[4] = {(float)x0 + 3, (float)bx0 + 2, (float)bx1 - 2, (float)x1 - 3}, cy[4] = {(float)yt - 1, (float)byt + 1, (float)byt + 1, (float)yt - 1}; mo_quad(B, cx, cy, lv_hex(0x6b5638), a); }
        mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xcbb894), a); mo13_line(B, x0, yt, bx0, byt, PAP, a); mo13_line(B, x1, yt, bx1, byt, lv_hex(0xcbb894), a); }
    /* front face: lacquer with a bevel (2 px lit inner frame top / left, shaded right column blending into the side) */
    for (int y = yt; y <= yb; ++y) mo13_span(B, y, x0, x1, lv_hex(0x2e241a), a);
    if (fw >= 6) { mo13_line(B, x0 + 1, yt + 1, x0 + 1, yb - 1, lv_hex(0x7a6342), a); mo13_line(B, x0 + 1, yt + 1, x1 - 1, yt + 1, lv_hex(0x7a6342), a);
                   mo13_line(B, x1 - 1, yt + 1, x1 - 1, yb - 1, lv_hex(0x1e1610), a); if (fw >= 30) { mo13_line(B, x0 + 2, yt + 2, x0 + 2, yb - 1, lv_hex(0x4e3e2c), a); mo13_line(B, x1 - 2, yt + 2, x1 - 2, yb - 1, lv_hex(0x241c14), a); } }
    /* icon plate (integer-scaled icon) or, on far pillars, a crisp 3x5 kind glyph */
    if (S) { const int px0 = (x0 + x1) / 2 - S / 2 - 2, py0 = yt + 3;
        for (int y = py0; y < py0 + S + 4; ++y) mo13_span(B, y, px0, px0 + S + 3, DK, a);
        mo13_line(B, px0, py0 + S + 4, px0 + S + 3, py0 + S + 4, G, a); mo13_line(B, px0, py0 - 1, px0 + S + 3, py0 - 1, lv_hex(0x7a6342), a);
        mo13_icon(B, i, px0 + 2, py0 + 2, S, a); }
    else if (fw >= 7) { const int gx = (x0 + x1) / 2 - 2, gy = yt + 2;
        for (int y = gy; y < gy + 7; ++y) mo13_span(B, y, gx, gx + 4, DK, a);
        for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + 1 + c, gy + 1 + r, GH, a); }
    /* fine shoji grid: thin panes (cell ~4..8 px) with lit edges + inner shading; a few dark and green panes */
    if (fw >= 5) {
        const int ix0 = x0 + (fw >= 30 ? 4 : 2), ix1 = x1 - (fw >= 30 ? 4 : 2), iw = ix1 - ix0 + 1, cols = iw / (fw >= 40 ? 8 : 4) < 2 ? 2 : iw / (fw >= 40 ? 8 : 4);
        for (int y = wy0, row = 0; y + pitch - 1 < yb - 1; y += pitch, ++row) for (int c = 0; c < cols; ++c) {
            const int xa = ix0 + c * iw / cols, xb = ix0 + (c + 1) * iw / cols - 2;
            const int h = (c * 7 + row * 13 + i * 5 + (int)(lv_now / 1500) * (row == (i % 5))) % 11;
            mo13_pane(B, xa, y, xb, y + pitch - 2, h == 0 ? 2 : h <= 2 ? 1 : 0, a); }
    }
    /* crisp outer edges + Matrix-green seam with a moving bright pulse */
    mo13_line(B, x0, yt, x0, yb, PAP, a); mo13_line(B, x0, yt, x1, yt, PAP, a); mo13_line(B, x1, yt, x1, yb, lv_hex(0xb8a47c), a); mo13_line(B, x0, yb, x1, yb, DK, a);
    if (fw >= 8) { const int sx = x1 - (fw >= 30 ? 3 : 1); mo13_line(B, sx, yt + 2, sx, yb - 1, G, a * 3 / 4);
        const int hgt = yb - yt; if (hgt > 8) { const int py = yb - 2 - (int)(fmodf(lv_now / 1000.0f * (0.6f + 0.05f * i) + i * 0.37f, 1.0f) * (hgt - 6)); mo13_line(B, sx, py, sx, py + 3, GH, a); } }
}
