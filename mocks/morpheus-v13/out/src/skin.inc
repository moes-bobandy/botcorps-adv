/* ---- v13 HD monolith skin (every pillar; drawn over Gibson's own corners from lv_rd_place, far -> near, so it never moves anything) ---- */
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
static void mo12_bigbuild(int i) { (void)i; }   /* v13: no blurred ghost sprite */
static void mo11_tower_fx(LvBand &B, const LvTower &T)
{
    const int i = T.i; if (i < 0 || i >= 12) return;
    const int a = (i == itw_mo_sel || (mo_closing && i == mo_last)) ? 256 : mo11_fade();   /* others wash toward the parchment while open */
    const int x0 = T.x0, x1 = T.x1, yt = T.yt, yb = T.yb, fw = x1 - x0;
    const uint16_t LAQ = lv_hex(0x2e241a), LAQL = lv_hex(0x45372a), SIDE = lv_hex(0x3d2f20), TOP = lv_hex(0x9c845c), RIB = lv_hex(0xb8a47c),
                   PAP = lv_hex(MO11_PAPER), DK = lv_hex(0x100c09), G = lv_hex(MO11_G), GH = lv_hex(MO11_GH), TIM = lv_hex(0x9c845c);
    /* side face (the visible one: toward the vanishing point) with perspective ribs that follow the receding top / bottom edges */
    auto side = [&](int xf, int xbk) {
        const float X[4] = {(float)xf, (float)xbk, (float)xbk, (float)xf}, Y[4] = {(float)yt, (float)T.byt, (float)T.byb, (float)yb};
        mo_quad(B, X, Y, SIDE, a);
        const int pitch = fw >= 30 ? 13 : 8;
        if (abs(xbk - xf) >= 2) for (int y = yt + 6 + (fw >= 12 ? 8 + (fw >= 42 ? 32 : fw >= 20 ? 16 : 8) : 0); y < yb - 2; y += pitch) {
            const float u = (y - yt) / (float)(yb - yt); const int by = (int)lroundf(T.byt + (T.byb - T.byt) * u);
            mo13_line(B, xf, y, xbk, by, TIM, a); }
        if (abs(xbk - xf) >= 6) { const int xm = (xf + xbk) / 2; const float u = (xm - xf) / (float)(xbk - xf);   /* mid post, in perspective */
            mo13_line(B, xm, (int)lroundf(yt + (T.byt - yt) * u) + 1, xm, (int)lroundf(yb + (T.byb - yb) * u) - 1, lv_hex(0x6b5638), a); }
        mo13_line(B, xbk, T.byt, xbk, T.byb, TIM, a); mo13_line(B, xf, yt, xbk, T.byt, RIB, a); mo13_line(B, xf + (xbk > xf ? 1 : -1), yt + 1, xf + (xbk > xf ? 1 : -1), yb - 1, lv_hex(0x120e0a), a);
    };
    if (T.bx1 > x1) side(x1, T.bx1);
    if (T.bx0 < x0) side(x0, T.bx0);
    /* top face (lit timber cap) */
    if (T.byt < yt) { const float X[4] = {(float)x0, (float)T.bx0, (float)T.bx1, (float)x1}, Y[4] = {(float)yt, (float)T.byt, (float)T.byt, (float)yt}; mo_quad(B, X, Y, TOP, a);
        mo13_line(B, T.bx0, T.byt, T.bx1, T.byt, RIB, a); }
    /* front face: opaque lacquer, a lit band down the left third (light from the upper left), no gradient = no banding */
    for (int y = yt; y <= yb; ++y) { mo13_span(B, y, x0, x1, LAQ, a); if (fw >= 6) mo13_span(B, y, x0 + 1, x0 + fw / 3, LAQL, a); }
    /* recessed icon plate + integer-scaled device icon */
    const int S = fw >= 54 ? 48 : fw >= 38 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 8 : 0; int wy = yt + 3;
    if (S) { const int px0 = (x0 + x1) / 2 - S / 2 - 2, py0 = yt + 3;
        for (int y = py0; y < py0 + S + 4; ++y) mo13_span(B, y, px0, px0 + S + 3, DK, a);
        mo13_line(B, px0, py0 + S + 4, px0 + S + 3, py0 + S + 4, G, a);   /* green sill under the plate */
        mo13_icon(B, i, px0 + 2, py0 + 2, S, a); wy = py0 + S + 8; }
    /* shoji window cassettes: lit paper / dark / Matrix-green panes, 1 px kumiko, lit lintel */
    if (fw >= 6) {
        const int cols = fw >= 30 ? 3 : 2, pitch = fw >= 30 ? 13 : fw >= 12 ? 8 : 6, ph = pitch - 3, ix0 = x0 + 2, iw = fw - 3;
        for (int y = wy, row = 0; y + ph < yb - 1; y += pitch, ++row) for (int c = 0; c < cols; ++c) {
            const int xa = ix0 + c * iw / cols + 1, xb = ix0 + (c + 1) * iw / cols - 1; if (xb < xa) continue;
            const int h = (c * 7 + row * 13 + i * 5 + (int)(lv_now / 1500) * (row == (i % 5))) % 9;
            const uint16_t pc = h == 0 ? lv_hex(0x1f9a52) : h <= 2 ? lv_hex(0x4a3c2a) : lv_hex(0xe6d8b6);
            for (int yy = y; yy < y + ph; ++yy) mo13_span(B, yy, xa, xb, pc, a);
            if (xb - xa >= 3) mo13_span(B, y, xa, xb, h <= 2 && h ? lv_hex(0x6b5638) : PAP, a);    /* lit lintel */
            if (xb - xa >= 8) mo13_line(B, (xa + xb) / 2, y + 1, (xa + xb) / 2, y + ph - 1, h <= 2 ? DK : TIM, a);   /* kumiko */
            if (ph >= 6) mo13_span(B, y + ph / 2, xa, xb, h <= 2 ? DK : TIM, a); }
    }
    /* crisp edges: parchment lit left + top, timber right, dark base; Matrix-green seam with a moving bright pulse */
    mo13_line(B, x0, yt, x0, yb, PAP, a); mo13_line(B, x0, yt, x1, yt, PAP, a); mo13_line(B, x1, yt, x1, yb, RIB, a); mo13_line(B, x0, yb, x1, yb, DK, a);
    if (fw >= 8) { const int sx = x1 - 1; mo13_line(B, sx, yt + 1, sx, yb - 1, G, a * 3 / 4);
        const int hgt = yb - yt; if (hgt > 8) { const int py = yb - 2 - (int)(fmodf(lv_now / 1000.0f * (0.6f + 0.05f * i) + i * 0.37f, 1.0f) * (hgt - 6)); mo13_line(B, sx, py, sx, py + 3, GH, a); } }
}
