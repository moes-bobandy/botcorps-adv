/* ==== MOCK 2026-11-13 morpheus-towers-3: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).
 * Mobandy: v12 ghost glass and v13.1 lacquer/shoji both rejected; no square pane / block grid on any face; the tower body carries
 * continuously scrolling folder/file line-items (folder icon, name, size); folders far more striking (double outline, cut corners,
 * header strip + device glyph, scanline/texture, glow on the active folder, trace/shimmer on landing). Motion untouched. EX.DATA only. */
#ifndef MO_LOOK
#define MO_LOOK 1
#endif
static const char *const MO3_FN[12][8] = {
    {"SSID.LST", "BEACON.LG", "CHAN.MAP", "WPA3.CFG", "RSSI.DAT", "OPEN.NET", "MAC.OUI", "VPN.DEF"},
    {"ADVERT.LG", "MAKER.ID", "PAIR.REQ", "RSSI.DAT", "NAME.LST", "UUID.MAP", "LE.SCAN", "OFF.DEF"},
    {"KICK.PKT", "DISAS.LOG", "FLOOD.SIG", "BSSID.LST", "CHAN.MAP", "PMF.DEF", "80211W.CF", "WPA3.DEF"},
    {"OPEN.NET", "WEP.KEY", "AP.LST", "CRACK.SIG", "SNIFF.LOG", "WPA2.DEF", "WPA3.DEF", "LOCK.CFG"},
    {"FLOCK.CAM", "PLATE.LOG", "SSID.SIG", "PNGUIN.ID", "FSEXT.BAT", "GPS.MAP", "KEEP.DAYS", "ASK.DEF"},
    {"DJI.UAV", "TELLO.NET", "REMOTE.ID", "SKYDIO.ID", "ANAFI.ID", "GPS.LOG", "PILOT.LOC", "REPORT.TX"},
    {"FINDMY.AD", "APPLE.4C", "TYPE12.ID", "OWNER.KEY", "UWB.PING", "SOUND.CMD", "LOST.MODE", "BAG.CHECK"},
    {"TILE.TRK", "FEED.ID", "FEEC.ID", "RING.CMD", "BLE.ADV", "OWNER.LOG", "SCAN.DEF", "KEYS.LST"},
    {"META.CAM", "ID.01AB", "ID.058E", "LED.CHK", "CLIP.MP4", "PHOTO.JPG", "BLE.ADV", "ASK.DEF"},
    {"HC05.SIG", "HC06.SIG", "RNBT.ID", "CARD.DMP", "PIN.LOG", "PUMP.LOC", "TAP.PAY", "COVER.KEY"},
    {"FLIPR.NAM", "SUBGHZ.RF", "NFC.DMP", "IR.REMOTE", "BADUSB.JS", "POPUP.SPM", "BLE.SPAM", "PATCH.DEF"},
    {"STAG.TRK", "FD5A.UUID", "FD59.UUID", "OWNER.ID", "UWB.PING", "SMART.THG", "SCAN.DEF", "RING.CMD"}};
static void mo3_size(const char *nm, char *out) { unsigned h = 7; for (const char *q = nm; *q; ++q) h = h * 31 + (unsigned char)*q; const int k = 6 + (int)(h % 94); snprintf(out, 12, "%d.%dKB", k / 10, k % 10); }
static inline int mo3_cl(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline uint16_t mo3_mx(uint32_t a, uint32_t b, float u) { const int k = (int)lroundf((u < 0 ? 0 : u > 1 ? 1 : u) * 64); return lv_mix(lv_hex(a), lv_hex(b), k, 64); }
/* clipped 3x5 text (rows ylo..yhi, columns <= xhi), soft fade over the window's top / bottom 4 rows */
static void mo3_t35(LvBand &B, int x, int y, const char *s, uint16_t c, int a, int ylo, int yhi, int xhi)
{
    for (; *s; ++s, x += 4) { if (x + 2 > xhi) break; int ch = (uint8_t)*s; if (ch < 0x20 || ch > 0x7e) ch = '?'; const uint16_t g = LV_F35[ch - 0x20];
        for (int k = 0; k < 15; ++k) if (g >> (14 - k) & 1) { const int yy = y + k / 3; if (yy < ylo || yy > yhi) continue;
            const int f = mo3_cl((yy - ylo) * 64, 0, 256) < mo3_cl((yhi - yy) * 64, 0, 256) ? mo3_cl((yy - ylo) * 64, 0, 256) : mo3_cl((yhi - yy) * 64, 0, 256);
            mo13_px(B, x + k % 3, yy, c, a * f / 256); } }
}
static void mo3_pxc(LvBand &B, int x, int y, uint16_t c, int a, int ylo, int yhi) { if (y < ylo || y > yhi) return;
    const int f = mo3_cl((y - ylo) * 64, 0, 256) < mo3_cl((yhi - y) * 64, 0, 256) ? mo3_cl((y - ylo) * 64, 0, 256) : mo3_cl((yhi - y) * 64, 0, 256); mo13_px(B, x, y, c, a * f / 256); }
static void mo3_fold5(LvBand &B, int x, int y, uint16_t c, int a, int ylo, int yhi)   /* 5x4 folder glyph: tab + box */
{
    static const char *const F[4] = {"##...", "#####", "#...#", "#####"};
    for (int r = 0; r < 4; ++r) for (int k = 0; k < 5; ++k) if (F[r][k] == '#') mo3_pxc(B, x + k, y + r, c, a, ylo, yhi);
}
/* the scrolling file list INSIDE the pillar body: rows rise continuously (whole-pixel steps, no blur) */
static void mo3_files(LvBand &B, int i, int x0, int x1, int ylo, int yhi, uint16_t ic, uint16_t tc, uint16_t sc, int a, float speed)
{
    if (yhi - ylo < 6 || x1 - x0 < 3) return;
    const int fw = x1 - x0 + 1, rowh = fw >= 40 ? 15 : fw >= 22 ? 8 : 4;
    const int pos = (int)(lv_now / 1000.0f * speed) + i * 37, base = pos / rowh, frac = pos % rowh;
    for (int k = 0;; ++k) { const int y = ylo + 2 - frac + k * rowh; if (y > yhi) break; const int e = (base + k) % 8; const char *nm = MO3_FN[i][e];
        if (y + rowh < ylo) continue;
        if (rowh == 15) { mo3_fold5(B, x0 + 2, y, ic, a, ylo, yhi); mo3_t35(B, x0 + 9, y, nm, tc, a, ylo, yhi, x1);
            char sz[12]; mo3_size(nm, sz); mo3_t35(B, x0 + 9, y + 7, sz, sc, a, ylo, yhi, x1);
            const int bw = x1 - (x0 + 9 + lv_w35(sz) + 3) - 1, fl = bw * (int)((7 + e * 13 + i * 5) % 10 + 1) / 10;   /* a tiny transfer bar per file */
            for (int q = 0; q < bw; ++q) mo3_pxc(B, x0 + 9 + lv_w35(sz) + 3 + q, y + 9, q < fl ? ic : sc, q < fl ? a : a / 3, ylo, yhi); }
        else if (rowh == 8) { mo3_fold5(B, x0 + 1, y, ic, a, ylo, yhi); mo3_t35(B, x0 + 8, y, nm, tc, a, ylo, yhi, x1); }
        else { mo3_pxc(B, x0, y, ic, a, ylo, yhi); mo3_pxc(B, x0 + 1, y, ic, a, ylo, yhi); mo3_pxc(B, x0, y + 1, ic, a, ylo, yhi); mo3_pxc(B, x0 + 1, y + 1, ic, a, ylo, yhi);
            const int L = mo3_cl((int)strlen(nm) * (fw - 3) / 10, 1, fw - 3); for (int q = 0; q < L; ++q) mo3_pxc(B, x0 + 3 + q, y + 1, tc, a * 3 / 4, ylo, yhi); } }
}
/* device icon with a per-look ink map; S = 8 / 16 / 32 (integer nearest-neighbour) */
static void mo3_icon(LvBand &B, int i, int x0, int y0, int S, uint32_t ck, uint32_t cb, uint32_t cc, int ak, int ab, int yhi = 9999)
{
    for (int y = 0; y < S; ++y) { if (y0 + y > yhi) break; for (int x = 0; x < S; ++x) { const int r = y * 16 / S, c = x * 16 / S; char ch;
        if (MO_ICON[i] < 0) { const char d = MO_DEAUTH[r * 12 / 16][c * 12 / 16]; ch = d == '.' ? '.' : d == 'c' ? 'c' : 'k'; } else ch = LV_ICON_ART[MO_ICON[i]][r][c];
        if (ch == '.') continue; if (ch == 'k') mo13_px(B, x0 + x, y0 + y, lv_hex(ck), ak); else if (ch == 'c') mo13_px(B, x0 + x, y0 + y, lv_hex(cc), ak); else mo13_px(B, x0 + x, y0 + y, lv_hex(cb), ab); } }
}
static void mo3_glow_icon(LvBand &B, int i, int x0, int y0, int S, uint32_t g, int a)   /* 1 px soft bloom round the icon silhouette */
{
    for (int y = -1; y <= S; ++y) for (int x = -1; x <= S; ++x) { int n = 0;
        for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) { const int yy = y + dy, xx = x + dx; if (yy < 0 || xx < 0 || yy >= S || xx >= S) continue;
            const int r = yy * 16 / S, c = xx * 16 / S; const char ch = MO_ICON[i] < 0 ? MO_DEAUTH[r * 12 / 16][c * 12 / 16] : LV_ICON_ART[MO_ICON[i]][r][c]; n += ch != '.'; }
        if (n) mo13_px(B, x0 + x, y0 + y, lv_hex(g), a * (n > 4 ? 4 : n) / 4); }
}
static void mo3_shadow(LvBand &B, int x0, int x1, int yb, uint32_t c, int a)   /* soft contact shadow on the tatami */
{
    for (int r = 1; r <= 3; ++r) { const int ext = 1 + r; const int k = a * (4 - r) / 4;
        for (int x = x0 - ext; x <= x1 + ext + 2; ++x) { const int ed = x < x0 ? x0 - x : x > x1 + 2 ? x - x1 - 2 : 0; mo13_px(B, x, yb + r, lv_hex(c), k * (ext + 1 - ed) / (ext + 1)); } }
}
static void mo11_tower_fx(LvBand &B, const LvTower &T)
{
    const int i = T.i; if (i < 0 || i >= 12) return;
    const bool open = i == itw_mo_sel || (mo_closing && i == mo_last); const float cp = open ? mo12_p(i) : 0.0f;
    const int a = open ? 256 : mo11_fade();
    const int x0 = T.x0, x1 = T.x1, yt = T.yt, yb = T.yb, fw = x1 - x0; if (fw < 2) return;
    const float kd = 1.0f + 1.0f * cp;
    const int bx0 = x0 + (int)lroundf((T.bx0 - x0) * kd), bx1 = x1 + (int)lroundf((T.bx1 - x1) * kd), byt = yt + (int)lroundf((T.byt - yt) * kd), byb = yb + (int)lroundf((T.byb - yb) * kd);
    const float t = lv_now / 1000.0f;
    const int yv = yb < 246 || !open ? yb : 246;   /* visible bottom of the body (the open panel covers 247..) */
    const int S = fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 10 ? 8 : 0;
#if MO_LOOK == 1
    /* ===== T1 MATRIX CODE GLASS: smoked green-black glass, falling code inside, hologram device in the core, glowing edges ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x1e2a20, a * 90 / 256);
    for (int r = 1; r <= 2; ++r) for (int y = yt - r; y <= yb; ++y) { mo13_px(B, x0 - r, y, lv_hex(MO11_GH), a * (r == 1 ? 70 : 28) / 256); mo13_px(B, (bx1 > x1 ? bx1 : x1) + r, y, lv_hex(MO11_GH), a * (r == 1 ? 70 : 28) / 256); }
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x0e3a22, 0x020c07, u * 0.7f + 0.3f * (y - top) / (float)(bot - top + 1)), a);
            if (s % 3 == 1) { const int hd = top + (int)fmodf(t * (22 + (x * 7) % 17) + x * 13, (float)(bot - top + 18)); for (int q = 0; q < 10; ++q) { const int y = hd - q; if (y >= top && y <= bot) mo13_px(B, x, y, lv_hex(q ? MO11_G : MO11_GH), a * (q ? 120 - q * 10 : 200) / 256); } } }
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x0a6a30), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO11_GH), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0x1a5a34), a); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_GH), a); }
    /* front glass: smooth vertical ramp + a soft vertical reflection band */
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1); const uint16_t base = mo3_mx(0x0b3320, 0x010805, v);
        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, rb = expf(-((u - 0.22f) * (u - 0.22f)) / 0.006f); mo13_px(B, x, y, base, a * 245 / 256); if (rb > 0.05f) mo13_px(B, x, y, lv_hex(0xbfffd8), (int)(a * 34 * rb / 256)); } }
    /* falling code: glyph columns (close-up) or 1 px streaks (idle), dim so the file rows stay readable */
    const bool big = fw >= 40; const int pitch = big ? 5 : 3;
    for (int cx = x0 + 2; cx <= x1 - 2; cx += pitch) { const int col = (cx - x0) / pitch + i * 7; const float sp = big ? 26 + (col * 11) % 23 : 14 + (col * 7) % 12;
        const int span = yv - yt + 30, hd = yt + (int)fmodf(t * sp + col * 41, (float)span), len = big ? 9 : 8;
        for (int q = 0; q < len; ++q) { const int gy = big ? hd - q * 6 : hd - q; if (gy < yt + 1 || gy > yv - 1) continue;
            const int al = a * (q == 0 ? (big ? 150 : 230) : (big ? 90 : 160) - q * (big ? 9 : 18)) / 256; if (al <= 0) continue; const uint16_t c = lv_hex(q == 0 ? 0xd8ffe6 : MO11_G);
            if (big) { const char gl[2] = {(char)("0123456789ZXKTRS"[(col * 5 + q * 3 + (int)(t * 3)) % 16]), 0}; mo3_t35(B, cx - 1, gy, gl, c, al, yt + 1, yv - 1, x1 - 1); }
            else mo13_px(B, cx, gy, c, al); } }
    /* hologram device in the core */
    const int iy = yt + (S >= 32 ? 6 : 3) + (int)lroundf(sinf(t * 1.6f + i) * (S >= 16 ? 1.0f : 0.0f));
    if (S) { const int ix = (x0 + x1) / 2 - S / 2; mo3_glow_icon(B, i, ix, iy, S, MO11_GH, a * 110 / 256); mo3_icon(B, i, ix, iy, S, 0xc8ffdc, MO11_G, 0xffffff, a, a * 200 / 256); }
    else if (fw >= 5) { const int gx = (x0 + x1) / 2 - 1; for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + c, yt + 3 + r, lv_hex(0xc8ffdc), a); }
    /* the file list window */
    const int ly0 = (S ? iy + S + (big ? 17 : 3) : yt + 10), ly1 = yv - (big ? 4 : 2);
    if (big) { for (int y = ly0 - 3; y <= ly1 + 1; ++y) mo13_span(B, y, x0 + 2, x1 - 2, lv_hex(0x010604), a * 170 / 256);
        mo13_line(B, x0 + 2, ly0 - 4, x1 - 2, ly0 - 4, lv_hex(MO11_GH), a); mo3_t35(B, x0 + 3, ly0 - 11, "FILES", lv_hex(MO11_GH), a, 0, 999, x1);
        { char n[8]; snprintf(n, sizeof n, "%d", 8); mo3_t35(B, x1 - 5, ly0 - 11, n, lv_hex(0x5ac080), a, 0, 999, x1); } }
    mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xd8ffe6 : 0x7dffb0), lv_hex(0x3a9a60), a, big ? 9.0f : 6.0f);
    /* glowing edges */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x7dffb0), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_GH), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xd8ffe6), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a6a30), a);
#elif MO_LOOK == 2
    /* ===== T2 CONSTRUCT WHITE LIGHT: luminous white slab, soft shadow, crisp green wire edges, floating green emblem ===== */
    if (cp < 0.3f) { mo3_shadow(B, x0, x1, yb, 0x5a5448, a * 120 / 256);
        for (int y = yt - 2; y <= yb; ++y) for (int r = 1; r <= 2; ++r) { mo13_px(B, x0 - r, y, lv_hex(0xffffff), a * (r == 1 ? 140 : 60) / 256); } }
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0xd2dad6, 0x9aa49f, u * 0.75f + 0.25f * (y - top) / (float)(bot - top + 1)), a); }
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO11_G), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO11_G), a); mo13_line(B, xf, yb, xbk, byb, lv_hex(0x5a8a6a), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xffffff), a); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_G), a); }
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);
        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw; mo13_px(B, x, y, mo3_mx(0xffffff, 0xd9e2dd, 0.55f * u * u + 0.35f * v), a); } }
    /* a soft light band rising slowly through the slab (the Construct loading light) */
    { const int span = yv - yt + 40, cy = yv + 20 - (int)fmodf(t * 18 + i * 23, (float)span);
      for (int y = cy - 10; y <= cy + 10; ++y) { if (y < yt + 1 || y > yv - 1) continue; const float d = (y - cy) / 10.0f; const int al = (int)(a * 70 * (1 - d * d) / 256); mo13_span(B, y, x0 + 1, x1 - 1, lv_hex(0xdcffe8), al); } }
    const bool big = fw >= 40;
    const int iy = yt + (S >= 32 ? 7 : 3) + (int)lroundf(sinf(t * 1.4f + i) * (S >= 16 ? 1.0f : 0.0f));
    if (S) { const int ix = (x0 + x1) / 2 - S / 2;
        if (S >= 32) { const float cx = ix + S / 2.0f - 0.5f, cy = iy + S / 2.0f - 0.5f, R = S * 0.5f + (S >= 32 ? 5 : 2);   /* thin green orbit ring under the emblem */
            for (int k = 0; k < 160; ++k) { const float th = k * 6.2832f / 160; mo13_px(B, (int)lroundf(cx + R * cosf(th)), (int)lroundf(cy + R * 0.32f * sinf(th) + S * 0.5f + 1), lv_hex(MO11_G), a * 150 / 256); } }
        mo3_glow_icon(B, i, ix, iy, S, 0x9fe8b8, a * 90 / 256); mo3_icon(B, i, ix, iy, S, 0x007a2c, 0x7fd89c, 0xffffff, a, a * 170 / 256); }
    else if (fw >= 5) { const int gx = (x0 + x1) / 2 - 1; for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + c, yt + 3 + r, lv_hex(MO11_G), a); }
    const int ly0 = (S ? iy + S + (big ? 12 : 4) : yt + 10), ly1 = yv - (big ? 4 : 2);
    if (big) mo13_line(B, x0 + 3, ly0 - 5, x1 - 3, ly0 - 5, lv_hex(MO11_G), a);
    mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_G), lv_hex(big ? 0x1e2a24 : 0x6a7a70), lv_hex(0x7a8a82), a, big ? 9.0f : 6.0f);
    mo13_line(B, x0, yt, x0, yv, lv_hex(MO11_G), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_G), a); mo13_line(B, x0, yt, x1, yt, lv_hex(MO11_GH), a); mo13_line(B, x0, yb, x1, yb, lv_hex(MO11_G), a);
#else
    /* ===== T3 DOJO SENTINEL POST: polished dark wood, brass bands, a backlit paper lantern with the device glowing through, green trim ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x2a2118, a * 110 / 256);
    const int bh = fw >= 40 ? 6 : fw >= 14 ? 3 : 2;   /* brass band height */
    const int bt0 = yt + (fw >= 40 ? 4 : 2), bb0 = yb - (fw >= 40 ? 10 : 3) - bh;   /* top band / bottom band rows */
    auto brass = [&](int k, int n) -> uint16_t { const float v = k / (float)(n > 1 ? n - 1 : 1); return v < 0.35f ? mo3_mx(0xfff0b0, 0xd8aa52, v / 0.35f) : mo3_mx(0xd8aa52, 0x6a4a18, (v - 0.35f) / 0.65f); };
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) { const float v = (y - yt) / (float)(yb - yt + 1); const int yr = (int)lroundf(yt + (y - top) * (yb - yt) / (float)(bot - top + 1)); (void)v;
                uint16_t c = mo3_mx(0x3a2414, 0x120a05, u);
                if ((yr >= bt0 && yr < bt0 + bh) || (yr >= bb0 && yr < bb0 + bh)) { const int kk = yr >= bb0 ? yr - bb0 : yr - bt0; c = lv_mix(brass(kk, bh), lv_hex(0x2a1a08), (int)(u * 40), 64); }
                mo13_px(B, x, y, c, a); } }
        mo13_line(B, xf, yt, xbk, byt, lv_hex(0xd8aa52), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xb08a3a), a); mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xfff0b0), a); }
    /* polished wood front: a soft vertical sheen near the lit edge, darker toward the right */
    for (int y = yt; y <= yv; ++y) for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, sh = expf(-((u - 0.2f) * (u - 0.2f)) / 0.012f);
        mo13_px(B, x, y, lv_mix(mo3_mx(0x3e2616, 0x170d06, u), lv_hex(0x8a5a32), (int)(sh * 40), 64), a); }
    /* brass bands with a green trim line */
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }
    /* lantern: backlit paper (smooth radial glow), dark frame, the device silhouette glowing through, ink file rows */
    const int m = fw >= 40 ? 5 : fw >= 12 ? 2 : 1, px0 = x0 + m, px1 = x1 - m, py0 = bt0 + bh + (fw >= 40 ? 4 : 2), py1 = (bb0 - (fw >= 40 ? 4 : 2) < yv ? bb0 - (fw >= 40 ? 4 : 2) : yv);
    if (px1 - px0 >= 2 && py1 - py0 >= 4) {
        const float cx = (px0 + px1) / 2.0f, cyc = py0 + (py1 - py0) * 0.3f, rx = (px1 - px0) * 0.75f + 1, ry = (py1 - py0) * 0.75f + 1;
        const float fl = 0.94f + 0.06f * sinf(t * 2.3f + i * 1.7f);   /* slow candle breathing */
        for (int y = py0; y <= py1; ++y) for (int x = px0; x <= px1; ++x) { const float dx = (x - cx) / rx, dy = (y - cyc) / ry, d = sqrtf(dx * dx + dy * dy);
            mo13_px(B, x, y, mo3_mx(0xfff4d6, 0xd89a48, d / fl), a); }
        if (fw >= 8) { mo13_line(B, px0 - 1, py0 - 1, px1 + 1, py0 - 1, lv_hex(0x140a04), a); mo13_line(B, px0 - 1, py1 + 1, px1 + 1, py1 + 1, lv_hex(0x140a04), a);
                       mo13_line(B, px0 - 1, py0, px0 - 1, py1, lv_hex(0x140a04), a); mo13_line(B, px1 + 1, py0, px1 + 1, py1, lv_hex(0x140a04), a); }
        const bool big = fw >= 40; const int Sl = big ? 32 : (px1 - px0 + 1) >= 16 ? 16 : (px1 - px0 + 1) >= 8 ? 8 : 0;
        const int iy = py0 + (big ? 4 : 2);
        if (Sl) { const int ix = (px0 + px1 + 1) / 2 - Sl / 2; mo3_icon(B, i, ix, iy, Sl, 0x2a1608, 0x6a3e18, 0xfff4d6, a * 220 / 256, a * 150 / 256, py1); }
        const int ly0 = Sl ? iy + Sl + (big ? 6 : 2) : py0 + 1, ly1 = py1 - 1;
        if (big) mo13_line(B, px0 + 2, ly0 - 3, px1 - 2, ly0 - 3, lv_hex(0x8a5a2a), a);
        mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(0x00863a), lv_hex(big ? 0x2a1608 : 0x5a3414), lv_hex(0x8a6030), a, big ? 9.0f : 6.0f);
    }
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x8a5a32), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x100804), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x100804), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a * 160 / 256); }
#endif
}
/* ---- the 3 right-hand folders, per look (DOWNLOAD structure kept: tab, device glyph, big name, big size, action line, 70x7 bar, drop stagger) ---- */
static void mo3_perim(int x, int y, int w, int h, int p, int &px, int &py)
{
    const int P = 2 * (w + h); p = ((p % P) + P) % P;
    if (p < w) { px = x + p; py = y; } else if (p < w + h) { px = x + w; py = y + (p - w); } else if (p < 2 * w + h) { px = x + w - (p - w - h); py = y + h; } else { px = x; py = y + h - (p - 2 * w - h); }
}
static void mo12_folder(LvBand &B, int i, int j, int ys, float ph, int tl)
{
    const int x = MO12_FX, w = MO12_FW, y = ys + 8, h = MO12_FH, tw = 60; const bool solid = ph >= 0.65f; const int cut = 7;
    const float t = lv_now / 1000.0f;
#if MO_LOOK == 1
    const uint32_t BODY = 0x03140b, SCAN = 0x072a17, HEAD = 0x0b3a22, OUT = 0x10d858, IN = 0x0a6a30, TAB = 0x10d858, TABT = 0x03140b, NAME = 0xd8ffe6, SIZE = 0x10d858, ACT = 0x5ac080, BARF = 0x10d858, BARB = 0x0a6a30, GLOW = 0x10d858, SHAD = 0x0a1a10;
    const uint32_t IK = 0xc8ffdc, IB = 0x10d858, IC = 0xffffff;
#elif MO_LOOK == 2
    const uint32_t BODY = 0xfafdfb, SCAN = 0xeef5f1, HEAD = 0xdcf5e6, OUT = 0x00a83c, IN = 0x9fe0b4, TAB = 0x00a83c, TABT = 0xffffff, NAME = 0x1e2a24, SIZE = 0x00a83c, ACT = 0x6a7a72, BARF = 0x00a83c, BARB = 0xcfe8d8, GLOW = 0x10d858, SHAD = 0x5a5448;
    const uint32_t IK = 0x007a2c, IB = 0x7fd89c, IC = 0xffffff;
#else
    const uint32_t BODY = 0x1c120c, SCAN = 0x24170e, HEAD = 0x3a2414, OUT = 0xd8aa52, IN = 0x00a83c, TAB = 0xd8aa52, TABT = 0x1c120c, NAME = 0xf6f0e2, SIZE = 0x10d858, ACT = 0xc8b088, BARF = 0xd8aa52, BARB = 0x3a2414, GLOW = 0xffd878, SHAD = 0x1e1a16;
    const uint32_t IK = 0xfff0c8, IB = 0xd8aa52, IC = 0xffffff;
#endif
    const uint16_t ink = lv_hex(NAME);
    /* drop shadow */
    if (solid) for (int yy = y - 4; yy < y + h + 4; ++yy) mo_hspan(B, yy, x + 4, x + w + 3, lv_hex(SHAD), MO_LOOK == 2 ? 50 : 100);
    /* active glow: the folder loading now, or a slow cycle once all are loaded */
    const bool act = (tl >= 0 && tl < MO12_LOAD_MS) || (tl >= MO12_LOAD_MS && ((int)(lv_now / 1400) % 3) == j && mo12_nland == 3);
    const int ga = act ? (int)(70 + 50 * sinf(t * 6.0f)) : 26;
    if (solid) for (int r = 3; r >= 1; --r) { const int al = ga * (4 - r) / 3; mo_hspan(B, y - 9 - r, x - r, x + tw + r, lv_hex(GLOW), al); mo_hspan(B, y - 1 - r, x + tw, x + w + r - 1, lv_hex(GLOW), al);
        mo_hspan(B, y + h + r - 1, x - r, x + w + r - 1, lv_hex(GLOW), al); for (int yy = y - 9 - r; yy < y + h + r; ++yy) { mo_bl(B, x - r, yy, lv_hex(GLOW), al); mo_bl(B, x + w - 1 + r, yy, lv_hex(GLOW), al); } }
    /* body with cut corners (T1: bottom-right; T2: top-right + bottom-left; T3: square, brass corner caps) */
    auto xr = [&](int yy) { int e = x + w - 1; if (MO_LOOK == 1 && yy > y + h - 1 - cut) e -= yy - (y + h - 1 - cut); if (MO_LOOK == 2 && yy < y + cut) e -= (y + cut) - yy; return e; };
    auto xl = [&](int yy) { int s = x; if (MO_LOOK == 2 && yy > y + h - 1 - cut) s += yy - (y + h - 1 - cut); return s; };
    for (int yy = y; yy < y + h; ++yy) { const float v = (yy - y) / (float)h; uint16_t c = MO_LOOK == 2 ? mo3_mx(0xffffff, 0xe8f0ec, v) : lv_hex(((yy - y) & 1) ? SCAN : BODY);
        if (!solid) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 200); mo_bl(B, xr(yy), yy, lv_hex(OUT), 200); continue; }
        mo_hspan(B, yy, xl(yy), xr(yy), c, MO_LOOK == 1 ? 242 : 256); }
    if (solid) for (int yy = y + 2; yy < y + 17; ++yy) mo_hspan(B, yy, xl(yy) + 3, xr(yy) - 3, lv_hex(HEAD), MO_LOOK == 2 ? 256 : 200);   /* header strip */
    /* tab (slanted end) */
    for (int r = 0; r < 9; ++r) mo_hspan(B, y - 9 + r, x, x + tw - 6 + (r < 5 ? r : 5), lv_hex(TAB), solid ? 256 : 120);
    if (!solid) return;
    /* double outline: 2 px outer + 1 px inner, following the cuts */
    for (int yy = y; yy < y + h; ++yy) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 256); mo_bl(B, xl(yy) + 1, yy, lv_hex(OUT), 256); mo_bl(B, xr(yy), yy, lv_hex(OUT), 256); mo_bl(B, xr(yy) - 1, yy, lv_hex(OUT), 256);
        if (yy >= y + 3 && yy <= y + h - 4) { mo_bl(B, xl(yy) + 3, yy, lv_hex(IN), 256); mo_bl(B, xr(yy) - 3, yy, lv_hex(IN), 256); } }
    mo_hspan(B, y, x + tw - 1, xr(y), lv_hex(OUT), 256); mo_hspan(B, y + 1, x + tw - 1, xr(y + 1), lv_hex(OUT), 256); mo_hspan(B, y + h - 1, xl(y + h - 1), xr(y + h - 1), lv_hex(OUT), 256); mo_hspan(B, y + h - 2, xl(y + h - 2), xr(y + h - 2), lv_hex(OUT), 256);
    mo_hspan(B, y + 3, xl(y + 3) + 3, xr(y + 3) - 3, lv_hex(IN), 256); mo_hspan(B, y + h - 4, xl(y + h - 4) + 3, xr(y + h - 4) - 3, lv_hex(IN), 256);
    mo_hspan(B, y - 10, x, x + tw - 6, lv_hex(OUT), 256);
#if MO_LOOK == 3
    { const uint16_t cap = lv_hex(0xfff0b0); const int L = 7;   /* brass corner caps */
      mo12_rect(B, x - 2, y - 2, L, 3, cap, 256); mo12_rect(B, x - 2, y - 2, 3, L, cap, 256); mo12_rect(B, x + w - L + 1, y - 2, L, 3, cap, 256); mo12_rect(B, x + w - 2, y - 2, 3, L, cap, 256);
      mo12_rect(B, x - 2, y + h - 1, L, 3, cap, 256); mo12_rect(B, x - 2, y + h - L + 1, 3, L, cap, 256); mo12_rect(B, x + w - L + 1, y + h - 1, L, 3, cap, 256); mo12_rect(B, x + w - 2, y + h - L + 1, 3, L, cap, 256); }
#endif
    /* content (positions = v13.1, so the text audit boxes are unchanged) */
    static const char *const MO13_FN[12][3] = {
        {"ROUTER.NET", "BEACON.SIG", "WPA3.DEF"},   {"BT.RADIO", "ADVERT.SIG", "PAIR.DEF"},  {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},
        {"OPEN.NET", "WEP.SIG", "WPA3.DEF"},        {"FLOCK.CAM", "SSID.SIG", "PLATE.DEF"},  {"DRONE.UAV", "REMOTEID.SIG", "REPORT.DEF"},
        {"AIRTAG.TRK", "FINDMY.SIG", "BAGCHK.DEF"}, {"TILE.TRK", "FEED.SIG", "TAGSCAN.DEF"}, {"META.CAM", "MAKERID.SIG", "LEDCHK.DEF"},
        {"SKIMMER.CRD", "HC05.SIG", "TAPPAY.DEF"},  {"FLIPPER.RF", "NAME.SIG", "POPUP.DEF"},  {"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"}};
    static const char *const MO13_ACT[3] = {"WHAT IT IS", "HOW SPOTTED", "PROTECT"};
    char tab[16]; snprintf(tab, sizeof tab, "SCROLL %d", j + 1); b_text35(B, x + 5, y - 6, tab, lv_hex(TABT));
    if (B.y0 == 0) mo12_aud(x + 5, y - 6, lv_w35(tab), 5, x + 1, y - 7, x + tw - 5, y - 1, "tab");
    mo3_icon(B, i, x + 6, y + 6, 8, IK, IB, IC, 256, 256);   /* device glyph in the header strip */
    const char *nm = MO13_FN[i][j]; b_text(B, x + 17, y + 7, nm, ink);
    const char *src = j == 0 ? MO_DEV[i].what : j == 1 ? MO_DEV[i].spot : MO_DEV[i].prot; unsigned hsh = 7; for (const char *q = nm; *q; ++q) hsh = hsh * 31 + (unsigned char)*q; const int kb10 = 6 + (int)strlen(src) % 9 + (int)(hsh % 31);
    char sz[12]; snprintf(sz, sizeof sz, "%d.%dKB", kb10 / 10, kb10 % 10); const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 6 - sw;
    b_text(B, sx, y + 20, sz, lv_hex(SIZE));
    b_text35(B, x + 6, y + 22, MO13_ACT[j], lv_hex(ACT));
    const int bw = 70, fill = (bw - 4) * (tl < 0 ? 0 : tl > MO12_LOAD_MS ? MO12_LOAD_MS : tl) / MO12_LOAD_MS;
    mo12_rect(B, x + 6, y + 35, bw - 4, 3, lv_hex(BARB), 256); b_frame(B, x + 4, y + 33, bw, 7, lv_hex(BARF));
    for (int q = 0; q < fill; ++q) { const uint16_t c = mo3_mx(MO_LOOK == 3 ? 0x00a83c : 0x0a8a3a, MO_LOOK == 1 ? 0xb8ffd0 : 0x10d858, q / 66.0f); mo12_rect(B, x + 6 + q, y + 35, 1, 3, c, 256); }
    char st[16]; if (tl >= MO12_LOAD_MS) snprintf(st, sizeof st, "LOADED"); else snprintf(st, sizeof st, "%d%%", tl < 0 ? 0 : 100 * tl / MO12_LOAD_MS);
    b_text35(B, x + w - 6 - lv_w35(st), y + 34, st, lv_hex(ACT));
    /* landing: a bright trace runs the outline (T1), a light sheen sweeps the face (T2), a gold glint runs the brass (T3) */
    if (tl >= -MO12_DROP_MS * 35 / 100 && tl < 700) { const float u = (tl + MO12_DROP_MS * 35 / 100) / (700.0f + MO12_DROP_MS * 35 / 100);
#if MO_LOOK == 2
        const int sx0 = x - 20 + (int)(u * (w + 40));
        for (int yy = y + 1; yy < y + h - 1; ++yy) for (int d = -4; d <= 4; ++d) { const int xx = sx0 + d - (yy - y) / 2; if (xx > xl(yy) + 1 && xx < xr(yy) - 1) mo_bl(B, xx, yy, lv_hex(0xffffff), 150 - abs(d) * 30); }
#else
        const int P = 2 * (w + h); const int hp = (int)(u * P * 1.05f);
        for (int d = 0; d < 26; ++d) { int px, py; mo3_perim(x - 1, y - 1, w + 1, h + 1, hp - d, px, py); const int al = 255 * (26 - d) / 26;
            mo_bl(B, px, py, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al); mo_bl(B, px + 1, py, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); mo_bl(B, px, py + 1, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); }
#endif
    }
    if (B.y0 == 0) { mo12_aud(x + 17, y + 7, (int)strlen(nm) * 6 - 1, 7, x + 16, y + 3, x + w - 4, y + 17, "fname");
                     mo12_aud(sx, y + 20, sw, 7, x + 60, y + 18, x + w - 4, y + 30, "fsize");
                     mo12_aud(x + 6, y + 22, lv_w35(MO13_ACT[j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");
                     mo12_aud(x + w - 6 - lv_w35(st), y + 34, lv_w35(st), 5, x + bw + 6, y + 32, x + w - 3, y + h - 2, "fst"); }
}
