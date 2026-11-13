    /* v13 content: the DOWNLOAD folder rows (tab, icon + one big file name, big size on the right, 70x7 bar + action line), Dojo inks */
    static const char *const MO13_FN[12][3] = {
        {"ROUTER.NET", "BEACON.SIG", "WPA3.DEF"},   {"BT.RADIO", "ADVERT.SIG", "PAIR.DEF"},  {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},
        {"OPEN.NET", "WEP.SIG", "WPA3.DEF"},        {"FLOCK.CAM", "SSID.SIG", "PLATE.DEF"},  {"DRONE.UAV", "REMOTEID.SIG", "REPORT.DEF"},
        {"AIRTAG.TRK", "FINDMY.SIG", "BAGCHK.DEF"}, {"TILE.TRK", "FEED.SIG", "TAGSCAN.DEF"}, {"META.CAM", "MAKERID.SIG", "LEDCHK.DEF"},
        {"SKIMMER.CRD", "HC05.SIG", "TAPPAY.DEF"},  {"FLIPPER.RF", "NAME.SIG", "POPUP.DEF"},  {"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"}};
    static const char *const MO13_ACT[3] = {"WHAT IT IS", "HOW SPOTTED", "PROTECT"};
    char tab[16]; snprintf(tab, sizeof tab, "SCROLL %d", j + 1); b_text35(B, x + 5, y - 6, tab, pap);
    if (B.y0 == 0) mo12_aud(x + 5, y - 6, lv_w35(tab), 5, x + 1, y - 7, x + tw - 5, y - 1, "tab");
    static const int IC[3] = {0, 2, 3};   /* the INFO folder icon set (box / scope / shield-ish), drawn in ink */
    { const uint32_t ic = 0x2a2118; lvi_box(B, x + 5, y + 6, 8, 8, ic);
      if (IC[j] == 0) { lvi_box(B, x + 7, y + 8, 4, 4, ic); lvi_px(B, x + 8, y + 9, 0x00a83c); }
      if (IC[j] == 2) { lvi_box(B, x + 7, y + 8, 4, 4, ic); lvi_line(B, x + 5, y + 9, x + 12, y + 9, ic); }
      if (IC[j] == 3) { lvi_line(B, x + 6, y + 7, x + 11, y + 7, ic); lvi_line(B, x + 6, y + 7, x + 7, y + 12, ic); lvi_line(B, x + 11, y + 7, x + 9, y + 12, ic); } }
    const char *nm = MO13_FN[i][j]; b_text(B, x + 17, y + 7, nm, ink);
    const char *src = j == 0 ? MO_DEV[i].what : j == 1 ? MO_DEV[i].spot : MO_DEV[i].prot; unsigned hsh = 7; for (const char *q = nm; *q; ++q) hsh = hsh * 31 + (unsigned char)*q; const int kb10 = 6 + (int)strlen(src) % 9 + (int)(hsh % 31);   /* EX.DATA size */
    char sz[12]; snprintf(sz, sizeof sz, "%d.%dKB", kb10 / 10, kb10 % 10); const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 4 - sw;
    b_text(B, sx, y + 20, sz, g);
    b_text35(B, x + 5, y + 22, MO13_ACT[j], sub);
    const int bw = 70, fill = (bw - 4) * (tl < 0 ? 0 : tl > MO12_LOAD_MS ? MO12_LOAD_MS : tl) / MO12_LOAD_MS;
    b_frame(B, x + 4, y + 34, bw, 7, tim); if (fill > 0) mo12_rect(B, x + 6, y + 36, fill, 3, g, 256);
    char st[16]; if (tl >= MO12_LOAD_MS) snprintf(st, sizeof st, "LOADED"); else snprintf(st, sizeof st, "%d%%", tl < 0 ? 0 : 100 * tl / MO12_LOAD_MS);
    b_text35(B, x + w - 4 - lv_w35(st), y + 35, st, sub);
    if (B.y0 == 0) { mo12_aud(x + 17, y + 7, (int)strlen(nm) * 6 - 1, 7, x + 16, y + 3, x + w - 4, y + 17, "fname");
                     mo12_aud(sx, y + 20, sw, 7, x + 60, y + 18, x + w - 4, y + 30, "fsize");
                     mo12_aud(x + 5, y + 22, lv_w35(MO13_ACT[j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");
                     mo12_aud(x + w - 4 - lv_w35(st), y + 35, lv_w35(st), 5, x + bw + 6, y + 32, x + w - 3, y + h - 2, "fst"); }
