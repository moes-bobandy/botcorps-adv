/* ==== MOCK 2026-11-13 morpheus-mix (from morpheus-towers-3 mo_looks.inc): the three pillar looks MIXED per device by threat level
 * (MO_LOOKOF[i]: HIGH -> 1 T1 code glass, MEDIUM -> 3 T3 sentinel lantern, LOW -> 2 T2 construct white light), 18 devices (WIFI + BLUETOOTH removed 2026-11-14), and the device
 * icon art replaced by crisp anti-aliased vector emblems (mo_emblem.h). The look code below is towers-3's, only switched at run time.
 * towers-3 header: three NEW pillar looks + matching folders (MO_LOOK 1 code glass, 2 construct light, 3 sentinel post).
 * Mobandy: v12 ghost glass and v13.1 lacquer/shoji both rejected; no square pane / block grid on any face; the tower body carries
 * continuously scrolling folder/file line-items (folder icon, name, size); folders far more striking (double outline, cut corners,
 * header strip + device glyph, scanline/texture, glow on the active folder, trace/shimmer on landing). Motion untouched. EX.DATA only. */
static const char *const MO3_FN[MO_N][8] = {   /* 2026-11-14 spread: WIFI + BLUETOOTH rows removed (18 devices) */
    {"KICK.PKT", "DISAS.LOG", "FLOOD.SIG", "BSSID.LST", "CHAN.MAP", "PMF.DEF", "80211W.CF", "WPA3.DEF"},
    {"OPEN.NET", "WEP.KEY", "AP.LST", "CRACK.SIG", "SNIFF.LOG", "WPA2.DEF", "WPA3.DEF", "LOCK.CFG"},
    {"FLOCK.CAM", "PLATE.LOG", "SSID.SIG", "PNGUIN.ID", "FSEXT.BAT", "GPS.MAP", "KEEP.DAYS", "ASK.DEF"},
    {"DJI.UAV", "TELLO.NET", "REMOTE.ID", "SKYDIO.ID", "ANAFI.ID", "GPS.LOG", "PILOT.LOC", "REPORT.TX"},
    {"FINDMY.AD", "APPLE.4C", "TYPE12.ID", "OWNER.KEY", "UWB.PING", "SOUND.CMD", "LOST.MODE", "BAG.CHECK"},
    {"TILE.TRK", "FEED.ID", "FEEC.ID", "RING.CMD", "BLE.ADV", "OWNER.LOG", "SCAN.DEF", "KEYS.LST"},
    {"META.CAM", "ID.01AB", "ID.058E", "LED.CHK", "CLIP.MP4", "PHOTO.JPG", "BLE.ADV", "ASK.DEF"},
    {"HC05.SIG", "HC06.SIG", "RNBT.ID", "CARD.DMP", "PIN.LOG", "PUMP.LOC", "TAP.PAY", "COVER.KEY"},
    {"FLIPR.NAM", "SUBGHZ.RF", "NFC.DMP", "IR.REMOTE", "BADUSB.JS", "POPUP.SPM", "BLE.SPAM", "PATCH.DEF"},
    {"STAG.TRK", "FD5A.UUID", "FD59.UUID", "OWNER.ID", "UWB.PING", "SMART.THG", "SCAN.DEF", "RING.CMD"},
    {"AXON.CAM", "AB3.SSID", "TASER.ID", "OUI.LST", "BODYCAM.V", "LE.GEAR", "RSSI.DAT", "ASK.DEF"},
    {"RAVEN.SNS", "UUID.3100", "SHOT.LOG", "POLE.LOC", "MIC.ARY", "ALERT.TX", "UUID.3500", "ASK.DEF"},
    {"ALPR.CAM", "MOTO.OUI", "GENTC.OUI", "PLATE.LOG", "HOTLST.DB", "GPS.MAP", "KEEP.DAYS", "CITY.DEF"},
    {"IPCAM.NET", "WYZE.OUI", "VRKDA.OUI", "TUYA.OUI", "RTSP.STM", "AXIS.OUI", "ROOM.CHK", "COVER.DEF"},
    {"RING.CAM", "RING.OUI", "AMZN.OUI", "PORCH.MP4", "MOTION.LG", "NEIGH.APP", "SHARE.REQ", "ASK.DEF"},
    {"FMDN.TAG", "FEAA.UUID", "CHIPL.ID", "PBBEE.ID", "MOTO.TAG", "OWNER.KEY", "SCAN.DEF", "RING.CMD"},
    {"TWIN.SSID", "BSSID.A", "BSSID.B", "ENC.DIFF", "BEACON.LG", "CAPTV.PRT", "KARMA.SIG", "LOCK.DEF"},
    {"IBCN.ADV", "4C000215", "UUID.MAJ", "MINOR.ID", "STORE.LOC", "SHOP.TRK", "PUSH.ADS", "BLE.OFF"}};
/* vector emblem blit: 1 px halo (contrast ink), body, detail; coverage alpha into RGB565 (mo13_px blends, field-clipped) */
static int mo_ex[MO_N], mo_ey[MO_N], mo_eS[MO_N];   /* this frame's drawn emblem rect per pillar (dump / gate) */
/* v4: opaque-UI audit. Every emblem / on-tower folder / file-line pixel goes through mo_ui_px; a pixel drawn below the pillar's own
 * alpha (fade-out while another pillar is open) would be a translucent UI pixel. Counts are printed at exit (MOUI) for the gate. */
static long mo_ui_tot, mo_ui_part; static int mo_ui_a = 256, mo_ui_cnt = 1;   /* mo_ui_cnt = 0 while the T1 code rain (not UI) borrows the 3x5 text routine */
static inline void mo_ui_px(LvBand &B, int x, int y, uint16_t c, int al) { if (mo_ui_cnt) { ++mo_ui_tot; if (al < mo_ui_a) ++mo_ui_part; } mo13_px(B, x, y, c, al); }
struct MoUiRep { ~MoUiRep() { printf("MOUI tot=%ld part=%ld\n", mo_ui_tot, mo_ui_part); fflush(stdout); } };
static MoUiRep mo_ui_rep;
static int mo_txt_oc = -1;   /* v4: 1 px opaque outline colour behind on-tower text (-1 none) */
static void mo_emb_draw(LvBand &B, int i, int x0, int y0, int S, uint32_t bc, uint32_t dc, uint32_t hc, int a, int ha, int yhi = 9999)
{
    const MoEmb *E = mo_emb_get(i, S); if (!E) return; S = E->S; const int H = S + 2;
    if (ha > 0) { mo_ex[i] = x0; mo_ey[i] = y0; mo_eS[i] = S; }   /* only the pillar emblems (with halo) are recorded; chip / folder glyphs are not */
    const uint16_t BC = lv_hex(bc), DC = lv_hex(dc), HC = lv_hex(hc);
    if (ha > 0) for (int y = 0; y < H; ++y) { if (y0 - 1 + y > yhi) break; for (int x = 0; x < H; ++x) { const int k = E->halo[y * H + x]; if (k) mo_ui_px(B, x0 - 1 + x, y0 - 1 + y, HC, a); } }   /* v4: opaque outline */
    for (int y = 0; y < S; ++y) { if (y0 + y > yhi) break; for (int x = 0; x < S; ++x) { const int kb = E->body[y * S + x], kd = E->det[y * S + x];
        if (kb) mo_ui_px(B, x0 + x, y0 + y, BC, a); if (kd) mo_ui_px(B, x0 + x, y0 + y, DC, a); } }   /* v4: binary masks, opaque */
}
static inline int mo_embS(int fw) { return fw >= 50 ? 40 : fw >= 44 ? 32 : fw >= 20 ? 16 : fw >= 12 ? 10 : fw >= 8 ? 8 : 0; }   /* v3 steps (kept for reference) */
/* v4: idle emblem as large as the face allows (face interior = fw - 1 px), in 2 px steps with hysteresis so a drifting pillar does not
 * flip sizes frame to frame; close-up 40 px. Returns 0 when the face is too narrow (< 7 px). */
static int mo_eS_last[MO_N];
static int mo_embS4(int i, int fw)
{
    if (fw >= 50) return mo_eS_last[i] = 40;
    int fit = fw - 1; if (fit > 36) fit = 36; if (fit < 6) { mo_eS_last[i] = 0; return 0; }
    int &L = mo_eS_last[i]; if (L == 40 || L == 0 || L > fit || L < fit - 3) L = fit & ~1; if (L > fit) L = fit; return L;
}
/* GENERATED by src/folders18.py (2026-11-14): real per-device folder + line-item text. Do not edit by hand. */
static const char *const MO_FOLD_NAME[MO_N][3] = {
    {"DEAUTH FLOOD", "MGMT FRAME BURST", "USE WPA3 / PMF"},   /* DEAUTH */
    {"WEAK WI-FI AP", "BEACON SAYS OPEN/WEP", "STAY OFF / USE VPN"},   /* HACK */
    {"FLOCK SAFETY ALPR", "SSID FLOCK- / BLE ID", "ASK RETENTION RULES"},   /* FLOCK */
    {"REMOTE ID DRONE", "RID: BLE FFFA, WIFI", "NOTE TIME + PLACE"},   /* DRONE */
    {"APPLE AIRTAG", "APPLE 004C, TYPE 12", "SCAN, FIND, DISABLE"},   /* AIR TAG */
    {"TILE KEYRING TAG", "BLE SVC FEED / FEEC", "TILE SCAN AND SECURE"},   /* KEYFOB */
    {"RAY-BAN META GLASSES", "BLE SVC FD5F", "WATCH CAPTURE LED"},   /* META */
    {"BT CARD SKIMMER", "HC-05/06, RN42 NAMES", "TAP PAY, COVER PIN"},   /* SKIMMER */
    {"FLIPPER ZERO", "BLE SVC 3081-3083", "IGNORE POPUPS"},   /* FLIPPER */
    {"GALAXY SMARTTAG", "BLE SVC FD5A", "SMARTTHINGS TAG SCAN"},   /* SAMSUNG */
    {"AXON BODY CAMERA", "SSID AB2- AB3- AB4-", "KNOW YOUR RIGHTS"},   /* AXON */
    {"RAVEN SHOT SENSOR", "BLE SVC 3100-3500", "ASK AUDIO POLICY"},   /* RAVEN */
    {"PLATE READER CAMERA", "WIFI MAKER OUI", "ASK WHO GETS DATA"},   /* ALPR */
    {"WI-FI IP CAMERA", "CAMERA MAKER OUI", "CHECK ROOM FOR CAMS"},   /* CAMERA */
    {"RING DOORBELL / CAM", "RING + AMAZON OUI", "ASSUME AUDIO + VIDEO"},   /* RING */
    {"FIND MY DEVICE TAG", "BLE SVC FEAA (FMDN)", "ANDROID TRACKER SCAN"},   /* GOOGLE TAG */
    {"EVIL TWIN HOTSPOT", "1 SSID, 2 BSSIDS", "FORGET OPEN COPIES"},   /* EVIL TWIN */
    {"APPLE IBEACON", "HEADER 4C 00 02 15", "LIMIT APP BT ACCESS"},   /* IBEACON */
};
static const char *const MO_FOLD_ACT[MO_N][3] = {
    {"FAKE 802.11 KICK FRAMES", "DEAUTH + DISASSOC RATE", "RISK: KICK + KEY CAPTURE"},
    {"OPEN OR WEP NETWORK", "SECURITY FIELD OF BEACON", "RISK: TRAFFIC READABLE"},
    {"PLATE CAMERA ON A POLE", "49 OUI, CO ID 09C8", "RISK: CAR MOVES LOGGED"},
    {"UAS BROADCASTING ITS ID", "ASTM F3411 MSG DECODED", "RISK: AERIAL FILMING"},
    {"FIND MY BLE TRACKER", "FIND MY OFFLINE ADVERT", "RISK: STALKER TAG IN BAG"},
    {"LIFE360 BLE TRACKER", "TILE SERVICE UUIDS", "RISK: FOLLOWS YOUR MOVES"},
    {"CAMERA GLASSES, SNAP TOO", "META/LUXOTTICA/SNAP IDS", "RISK: SILENT PHOTO/VIDEO"},
    {"HIDDEN IN PUMPS + ATMS", "SPP UUID 1101 + 3 OUI", "RISK: CARD + PIN STOLEN"},
    {"MULTI-RADIO HACK TOOL", "CO ID 0E29, OUI 0CFA22", "RISK: BLE SPAM, RF CLONE"},
    {"SAMSUNG BLE TRACKER", "SMARTTAG ADVERT UUID", "RISK: GALAXY NET LOCATES"},
    {"POLICE CAMS, TASERS", "AXON- SSID + 3 WIFI OUI", "RISK: YOU MAY BE FILMED"},
    {"POLE AUDIO DETECTOR", "ANY UUID IN THAT RANGE", "RISK: LISTENS FOR SOUNDS"},
    {"MOTOROLA OR GENETEC", "7 OUI: 5 MOTO, 2 GENETEC", "RISK: TRIPS LOGGED"},
    {"WYZE TUYA VERKADA AXIS", "17 KNOWN CAM OUI ON WIFI", "RISK: HIDDEN RECORDING"},
    {"AMAZON HOME CAMERA", "15 WIFI OUI PREFIXES", "RISK: PORCH IS RECORDED"},
    {"CHIPOLO PEBBLEBEE MOTO", "GOOGLE FIND MY NETWORK", "RISK: ANDROIDS RELAY IT"},
    {"FAKE COPY OF A REAL AP", "ENCRYPTION DISAGREES", "RISK: PASSWORD PHISHING"},
    {"SHOP PROXIMITY BEACON", "UUID + MAJOR + MINOR", "RISK: IN-STORE TRACKING"},
};
static const char *const MO_FOLD_TAG[MO_N][3] = {
    {"HIGH", "WIFI", "DO"},
    {"HIGH", "WIFI", "DO"},
    {"LOW", "WI+BT", "DO"},
    {"LOW", "WI+BT", "DO"},
    {"MED", "BLE", "DO"},
    {"MED", "BLE", "DO"},
    {"MED", "BLE", "DO"},
    {"HIGH", "BT", "DO"},
    {"HIGH", "BLE", "DO"},
    {"MED", "BLE", "DO"},
    {"MED", "WIFI", "DO"},
    {"LOW", "BLE", "DO"},
    {"LOW", "WIFI", "DO"},
    {"MED", "WIFI", "DO"},
    {"LOW", "WIFI", "DO"},
    {"MED", "BLE", "DO"},
    {"HIGH", "WIFI", "DO"},
    {"LOW", "BLE", "DO"},
};
static const char *const MO_ITEM_K[MO_N][8] = {
    {"TYPE", "FRAME", "SUBTYPE", "DISASOC", "SIGN", "BAND", "FIX", "STD"},
    {"TYPE", "LOCK", "OR", "WEP", "SNIFF", "SAFE", "ALSO OK", "DO"},
    {"MAKER", "TYPE", "SSID", "BLE CO", "BLE NAME", "OUIS", "RISK", "DO"},
    {"TYPE", "STD", "BLE SVC", "WIFI", "SENDS", "ALSO", "RISK", "DO"},
    {"MAKER", "NET", "CO ID", "TYPE", "HAS", "ALERTS", "RISK", "DO"},
    {"MAKER", "OWNER", "SVC", "SVC 2", "RADIO", "RISK", "CHECK", "DO"},
    {"MAKER", "BRAND", "ALSO", "SVC", "CAM", "LED", "RISK", "DO"},
    {"TYPE", "MOD 1", "MOD 2", "MOD 3", "MOD 4", "SPP", "RISK", "DO"},
    {"RADIO", "ALSO", "RFID", "SVC", "CO ID", "OUI", "RISK", "DO"},
    {"MAKER", "NET", "SVC", "ALSO", "PLUS", "RISK", "APP", "DO"},
    {"MAKER", "USER", "SSID", "ALSO", "OR", "OUIS", "RISK", "DO"},
    {"MAKER", "TYPE", "MOUNT", "SVC LO", "SVC HI", "RADIO", "RISK", "DO"},
    {"TYPE", "MAKER", "ALSO", "OUIS", "MOTO", "GENETEC", "RISK", "DO"},
    {"TYPE", "OUIS", "MAKER", "MAKER 2", "MAKER 3", "RADIO", "RISK", "DO"},
    {"MAKER", "OWNER", "TYPE", "OUIS", "RADIO", "REC", "RISK", "DO"},
    {"NET", "SVC", "PROTO", "MAKER", "MAKER 2", "ALSO", "RISK", "DO"},
    {"TYPE", "SSID", "BSSIDS", "LOCK", "LURE", "RISK", "USE", "DO"},
    {"FORMAT", "HEADER", "ID", "PLUS", "USE", "RISK", "DEFAULT", "DO"},
};
static const char *const MO_ITEM_V[MO_N][8] = {
    {"ATTACK", "DEAUTH", "12", "10", "BURST", "2.4 GHZ", "PMF", "802.11W"},
    {"AP", "OPEN", "WEP", "BROKEN", "EASY", "WPA3", "WPA2", "VPN"},
    {"FLOCK", "ALPR CAM", "FLOCK-", "09C8", "PENGUIN", "49", "TRACKED", "ASK CITY"},
    {"UAS", "F3411", "FFFA", "VEND IE", "SERIAL", "OP LOC", "FILMING", "REPORT"},
    {"APPLE", "FIND MY", "004C", "12", "UWB+NFC", "IOS+AND", "STALKING", "PULL BAT"},
    {"TILE", "LIFE360", "FEED", "FEEC", "BLE", "TRACKING", "TILE APP", "SCAN"},
    {"META", "RAY-BAN", "SNAP", "FD5F", "PHO+VID", "CAPTURE", "FILMING", "ASK"},
    {"SKIMMER", "HC-05", "HC-06", "RN42", "BT04-A", "1101", "CARD", "TAP PAY"},
    {"SUB-GHZ", "NFC+IR", "125KHZ", "3081", "0E29", "0CFA22", "CLONING", "UPDATE"},
    {"SAMSUNG", "ST FIND", "FD5A", "FD59", "UWB", "TRACKING", "SMARTTH.", "SCAN"},
    {"AXON", "POLICE", "AB3-", "AB4-", "AXON-", "3", "FILMED", "RIGHTS"},
    {"FLOCK", "GUNSHOT", "POLE", "3100", "3500", "BLE", "AUDIO", "ASK"},
    {"ALPR", "MOTOROLA", "GENETEC", "7", "5", "2", "LOGGED", "ASK"},
    {"IP CAM", "17", "WYZE", "TUYA", "VERKADA", "WIFI", "HIDDEN", "LOOK"},
    {"RING", "AMAZON", "DOORBELL", "15", "WIFI", "AUD+VID", "RECORDED", "ASSUME"},
    {"GOOGLE", "FEAA", "FMDN", "CHIPOLO", "PEBBLEBE", "MOTO TAG", "TRACKING", "SCAN"},
    {"ROGUE AP", "SAME", "TWO", "DIFFERS", "PORTAL", "PHISHING", "VPN", "FORGET"},
    {"APPLE", "4C000215", "UUID", "MAJ+MIN", "RETAIL", "LOCATION", "OFF", "APP PERM"},
};
static void mo3_size(const char *nm, char *out) { unsigned h = 7; for (const char *q = nm; *q; ++q) h = h * 31 + (unsigned char)*q; const int k = 6 + (int)(h % 94); snprintf(out, 12, "%d.%dKB", k / 10, k % 10); }
static inline int mo3_cl(int v, int lo, int hi) { return v < lo ? lo : v > hi ? hi : v; }
static inline uint16_t mo3_mx(uint32_t a, uint32_t b, float u) { const int k = (int)lroundf((u < 0 ? 0 : u > 1 ? 1 : u) * 64); return lv_mix(lv_hex(a), lv_hex(b), k, 64); }
/* clipped 3x5 text (rows ylo..yhi, columns <= xhi), soft fade over the window's top / bottom 4 rows */
static void mo3_t35(LvBand &B, int x, int y, const char *s, uint16_t c, int a, int ylo, int yhi, int xhi)
{
    for (; *s; ++s, x += 4) { if (x + 2 > xhi) break; int ch = (uint8_t)*s; if (ch < 0x20 || ch > 0x7e) ch = '?'; const uint16_t g = LV_F35[ch - 0x20];
        for (int k = 0; k < 15; ++k) if (g >> (14 - k) & 1) { const int yy = y + k / 3; if (yy < ylo || yy > yhi) continue;
            if (mo_txt_oc >= 0) for (int dy = -1; dy <= 1; ++dy) for (int dx = -1; dx <= 1; ++dx) { const int oy = yy + dy; if (oy < ylo || oy > yhi) continue;
                const int r2 = k / 3 + dy, c2 = k % 3 + dx; if (r2 >= 0 && r2 < 5 && c2 >= 0 && c2 < 3 && (g >> (14 - (r2 * 3 + c2)) & 1)) continue; mo_ui_px(B, x + c2, oy, lv_hex((uint32_t)mo_txt_oc), a); }
            mo_ui_px(B, x + k % 3, yy, c, a); } }   /* v4: hard clip, opaque (was a 4-row alpha fade) */
}
static void mo3_pxc(LvBand &B, int x, int y, uint16_t c, int a, int ylo, int yhi) { if (y < ylo || y > yhi) return; mo_ui_px(B, x, y, c, a); }   /* v4: hard clip, opaque */
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
    for (int k = 0;; ++k) { const int y = ylo + 2 - frac + k * rowh; if (y > yhi) break; const int e = (base + k) % 8; const char *nm = MO_ITEM_K[i][e];   /* real KEY (was a made-up file name) */
        if (y + rowh < ylo) continue;
        if (rowh == 15) { mo3_fold5(B, x0 + 2, y, ic, a, ylo, yhi); mo3_t35(B, x0 + 9, y, nm, tc, a, ylo, yhi, x1);
            char sz[12]; snprintf(sz, sizeof sz, "%s", MO_ITEM_V[i][e]); mo3_t35(B, x0 + 9, y + 7, sz, sc, a, ylo, yhi, x1);   /* real VALUE (was a fake KB size) */
            const int bw = x1 - (x0 + 9 + lv_w35(sz) + 3) - 1, fl = bw * (int)((7 + e * 13 + i * 5) % 10 + 1) / 10;   /* a tiny transfer bar per file */
            for (int q = 0; q < bw; ++q) if (q < fl || (q & 1) == 0) mo3_pxc(B, x0 + 9 + lv_w35(sz) + 3 + q, y + 9, q < fl ? ic : sc, a, ylo, yhi); }   /* v4: opaque (unfilled = dotted, was 1/3 alpha) */
        else if (rowh == 8) { mo3_fold5(B, x0 + 1, y, ic, a, ylo, yhi); mo3_t35(B, x0 + 8, y, MO_ITEM_V[i][e], tc, a, ylo, yhi, x1); }   /* one-line rows: the VALUE (the fact) */
        else { mo3_pxc(B, x0, y, ic, a, ylo, yhi); mo3_pxc(B, x0 + 1, y, ic, a, ylo, yhi); mo3_pxc(B, x0, y + 1, ic, a, ylo, yhi); mo3_pxc(B, x0 + 1, y + 1, ic, a, ylo, yhi);
            const int L = mo3_cl((int)strlen(nm) * (fw - 3) / 10, 1, fw - 3); for (int q = 0; q < L; ++q) mo3_pxc(B, x0 + 3 + q, y + 1, tc, a, ylo, yhi); } }
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
    const int i = T.i; if (i < 0 || i >= MO_N) return;
    const bool open = i == itw_mo_sel || (mo_closing && i == mo_last); const float cp = open ? mo12_p(i) : 0.0f;
    const int a = open ? 256 : mo11_fade();
    const int x0 = T.x0, x1 = T.x1, yt = T.yt, yb = T.yb, fw = x1 - x0; if (fw < 2) return;
    const float kd = 1.0f + 1.0f * cp;
    const int bx0 = x0 + (int)lroundf((T.bx0 - x0) * kd), bx1 = x1 + (int)lroundf((T.bx1 - x1) * kd), byt = yt + (int)lroundf((T.byt - yt) * kd), byb = yb + (int)lroundf((T.byb - yb) * kd);
    const float t = lv_now / 1000.0f;
    const int yv = yb < 246 || !open ? yb : 246;   /* visible bottom of the body (the open panel covers 247..) */
    const int S = mo_embS4(i, fw); mo_ui_a = a;
    const int L = MO_LOOKOF[i];
    if (L == 1) {
    /* ===== T1 MATRIX CODE GLASS: smoked green-black glass, falling code inside, hologram device in the core, glowing edges ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x1e2a20, a * 90 / 256);
    /* v4: no soft outer glow (it muddied the edges) */
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
        const int hx = x0 + 1 + fw / 5; for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, x == hx ? mo3_mx(0x0b3320, 0x5ac080, 0.45f - 0.3f * v) : base, a); }   /* v4: opaque glass + one crisp highlight column */
    /* falling code: glyph columns (close-up) or 1 px streaks (idle), dim so the file rows stay readable */
    const bool big = fw >= 40; const int pitch = big ? 5 : 3;
    for (int cx = x0 + 2; cx <= x1 - 2; cx += pitch) { const int col = (cx - x0) / pitch + i * 7; const float sp = big ? 26 + (col * 11) % 23 : 14 + (col * 7) % 12;
        const int span = yv - yt + 30, hd = yt + (int)fmodf(t * sp + col * 41, (float)span), len = big ? 9 : 8;
        for (int q = 0; q < len; ++q) { const int gy = big ? hd - q * 6 : hd - q; if (gy < yt + 1 || gy > yv - 1) continue;
            const int al = a * (q == 0 ? (big ? 150 : 230) : (big ? 90 : 160) - q * (big ? 9 : 18)) / 256; if (al <= 0) continue; const uint16_t c = lv_hex(q == 0 ? 0xd8ffe6 : MO11_G);
            if (big) { const char gl[2] = {(char)("0123456789ZXKTRS"[(col * 5 + q * 3 + (int)(t * 3)) % 16]), 0}; mo_ui_cnt = 0; mo3_t35(B, cx - 1, gy, gl, c, al, yt + 1, yv - 1, x1 - 1); mo_ui_cnt = 1; }   /* code rain: decoration, not audited UI */
            else mo13_px(B, cx, gy, c, al); } }
    /* hologram device in the core */
    const int iy = yt + (S >= 32 ? 6 : 3);   /* v4: no bob */
    if (S) { const int ix = x0 + 1 + (fw - 1 - S) / 2; mo_emb_draw(B, i, ix, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a); }
    else if (fw >= 5) { const int gx = (x0 + x1) / 2 - 1; for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + c, yt + 3 + r, lv_hex(0xc8ffdc), a); }
    /* the file list window */
    const int ly0 = (S ? iy + S + (big ? 17 : 3) : yt + 10), ly1 = yv - (big ? 4 : 2);
    if (big) { for (int y = ly0 - 3; y <= ly1 + 1; ++y) mo13_span(B, y, x0 + 2, x1 - 2, lv_hex(0x010604), a);
        mo13_line(B, x0 + 2, ly0 - 4, x1 - 2, ly0 - 4, lv_hex(MO11_GH), a); mo3_t35(B, x0 + 3, ly0 - 11, "FILES", lv_hex(MO11_GH), a, 0, 999, x1);
        { char n[8]; snprintf(n, sizeof n, "%d", 8); mo3_t35(B, x1 - 5, ly0 - 11, n, lv_hex(0x5ac080), a, 0, 999, x1); } }
    mo_txt_oc = 0x010805; mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xd8ffe6 : 0x7dffb0), lv_hex(0x5ac080), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;
    /* glowing edges */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x7dffb0), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_GH), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xd8ffe6), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a6a30), a);
    if (S) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, iy, S, 0xd8ffe6, MO11_GH, 0x010805, a, a);   /* v4: emblem on top of the edges */
    } else if (L == 2) {
    /* ===== T2 CONSTRUCT WHITE LIGHT: luminous white slab, soft shadow, crisp green wire edges, floating green emblem ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x5a5448, a * 120 / 256);   /* v4: no soft white outer glow */
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, lv_hex(0xc4ccc8), a * 150 / 256); }   /* v4: see-through side */
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO11_G), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO11_G), a); mo13_line(B, xf, yb, xbk, byb, lv_hex(0x5a8a6a), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xffffff), a * 170 / 256); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_G), a); }
    for (int y = yt; y <= yv; ++y) mo13_span(B, y, x0, x1, lv_hex(0xf4f8f6), a * 120 / 256);   /* v4: SEE-THROUGH white light front (~47%), flat = crisp; the room / pillars behind show through */
    /* v4: the soft rising light band removed (alpha shimmer) */
    const bool big = fw >= 40;
    const int iy = yt + (S >= 32 ? 7 : 3);   /* v4: no bob */
    if (S) { const int ix = x0 + 1 + (fw - 1 - S) / 2;
        if (S >= 32) { const float cx = ix + S / 2.0f - 0.5f, cy = iy + S / 2.0f - 0.5f, R = S * 0.5f + (S >= 32 ? 5 : 2);   /* thin green orbit ring under the emblem */
            for (int k = 0; k < 160; ++k) { const float th = k * 6.2832f / 160; mo13_px(B, (int)lroundf(cx + R * cosf(th)), (int)lroundf(cy + R * 0.32f * sinf(th) + S * 0.5f + 1), lv_hex(MO11_G), a * 150 / 256); } }
        mo_emb_draw(B, i, ix, iy, S, 0x005a20, 0x00b843, 0xffffff, a, a); }
    else if (fw >= 5) { const int gx = (x0 + x1) / 2 - 1; for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + c, yt + 3 + r, lv_hex(MO11_G), a); }
    const int ly0 = (S ? iy + S + (big ? 12 : 4) : yt + 10), ly1 = yv - (big ? 4 : 2);
    if (big) mo13_line(B, x0 + 3, ly0 - 5, x1 - 3, ly0 - 5, lv_hex(MO11_G), a);
    mo_txt_oc = 0xffffff; mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(0x008a32), lv_hex(0x1e2a24), lv_hex(0x2e4a3a), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: dark text, white outline, opaque */
    mo13_line(B, x0, yt, x0, yv, lv_hex(MO11_G), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_G), a); mo13_line(B, x0, yt, x1, yt, lv_hex(MO11_GH), a); mo13_line(B, x0, yb, x1, yb, lv_hex(MO11_G), a);
    if (S) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, iy, S, 0x005a20, 0x00b843, 0xffffff, a, a);   /* v4: emblem on top of the edges */
    } else {
    /* ===== T3 SEE-THROUGH WOOD (2026-11-14): translucent wood glass (grain + warm tone, the room shows through), thin brass bands,
     * the device emblem and the scrolling file line-items inside the pillar, green trim. Geometry / emblem sizes / file list as before. ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x2a2118, a * 80 / 256);
    const int bh = fw >= 40 ? 4 : fw >= 14 ? 2 : 1;   /* thin brass band height */
    const int bt0 = yt + (fw >= 40 ? 4 : 2), bb0 = yb - (fw >= 40 ? 10 : 3) - bh;
    auto brass = [&](int k, int n) -> uint16_t { const float v = k / (float)(n > 1 ? n - 1 : 1); return v < 0.35f ? mo3_mx(0xfff0b0, 0xd8aa52, v / 0.35f) : mo3_mx(0xd8aa52, 0x6a4a18, (v - 0.35f) / 0.65f); };
    auto grain = [&](int x, int y, float u) -> uint16_t {   /* wood grain: wavy vertical figure, light / dark streaks */
        const float g = sinf((x - x0) * (fw >= 40 ? 0.55f : 1.3f) + 1.6f * sinf(y * 0.11f + i * 1.3f) + i);
        (void)u; return lv_hex(g > 0.15f ? 0x8a5428 : 0x4a2a12); };   /* v4: two-tone grain (crisp streaks, no ramp) */
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x6a4020, 0x2a160a, u), a * 105 / 256); }
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x8a5a2a), a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(0xd8aa52), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0xb08a3a), a * 150 / 256); mo13_line(B, bx0, byt, bx1, byt, lv_hex(0xfff0b0), a); }
    /* translucent wood front: grain at ~38% (v3; v2 was 55%) so the room / rain behind shows through, a soft sheen near the lit edge, a faint warm inner light */
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);
        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, sh = expf(-((u - 0.2f) * (u - 0.2f)) / 0.012f);
            mo13_px(B, x, y, grain(x, y, u), a * 96 / 256);   /* v3: ~38% wood so the room shows through at idle too */
            (void)sh; (void)v; } }   /* v4: blurred sheen + warm glow removed (muddy) */
    /* thin brass bands with a green trim line (the pillar's frame) */
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }
    /* inside the wood: the device emblem + the scrolling file line-items, seen through */
    const int m = fw >= 40 ? 5 : fw >= 12 ? 2 : 1, px0 = x0 + m, px1 = x1 - m, py0 = bt0 + bh + (fw >= 40 ? 4 : 2), py1 = (bb0 - (fw >= 40 ? 4 : 2) < yv ? bb0 - (fw >= 40 ? 4 : 2) : yv);
    if (px1 - px0 >= 2 && py1 - py0 >= 4) {
        const bool big = fw >= 40; const int Sl = S;   /* v4: same big idle emblem rule as T1 / T2 */
        const int iy = py0 + (big ? 4 : 2);
        if (Sl) { const int ix = x0 + 1 + (fw - 1 - Sl) / 2; mo_emb_draw(B, i, ix, iy, Sl, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1); }
        const int ly0 = Sl ? iy + Sl + (big ? 8 : 2) : py0 + 1, ly1 = py1 - 1;
        if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a);
        mo_txt_oc = 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(0xfff0c8), lv_hex(0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: cream text, dark outline, opaque */
    }
    /* edges: lit brass left / top, dark right, green inner rail */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }
    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, py0 + (fw >= 40 ? 4 : 2), S, 0xfff0c8, MO11_GH, 0x1c120c, a, a, py1);   /* v4: emblem on top of the edges */
    }
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
const int MO_LOOK = MO_LOOKOF[i];   /* the opened pillar's own look */
    uint32_t BODY, SCAN, HEAD, OUT, IN, TAB, TABT, NAME, SIZE, ACT, BARF, BARB, GLOW, SHAD, IK, IB, IC;
    if (MO_LOOK == 1) { BODY = 0x03140b; SCAN = 0x072a17; HEAD = 0x0b3a22; OUT = 0x10d858; IN = 0x0a6a30; TAB = 0x10d858; TABT = 0x03140b; NAME = 0xd8ffe6; SIZE = 0x10d858; ACT = 0x5ac080; BARF = 0x10d858; BARB = 0x0a6a30; GLOW = 0x10d858; SHAD = 0x0a1a10;
        IK = 0xc8ffdc; IB = 0x10d858; IC = 0xffffff; }
    else if (MO_LOOK == 2) { BODY = 0xfafdfb; SCAN = 0xeef5f1; HEAD = 0xdcf5e6; OUT = 0x00a83c; IN = 0x9fe0b4; TAB = 0x00a83c; TABT = 0xffffff; NAME = 0x1e2a24; SIZE = 0x00a83c; ACT = 0x6a7a72; BARF = 0x00a83c; BARB = 0xcfe8d8; GLOW = 0x10d858; SHAD = 0x5a5448;
        IK = 0x006a26; IB = 0x00b843; IC = 0xffffff; }
    else { BODY = 0x1c120c; SCAN = 0x24170e; HEAD = 0x3a2414; OUT = 0xd8aa52; IN = 0x00a83c; TAB = 0xd8aa52; TABT = 0x1c120c; NAME = 0xf6f0e2; SIZE = 0x10d858; ACT = 0xc8b088; BARF = 0xd8aa52; BARB = 0x3a2414; GLOW = 0xffd878; SHAD = 0x1e1a16;
        IK = 0xfff0c8; IB = 0xd8aa52; IC = 0xffffff; }
    (void)IC;
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
        if (!solid) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 256); mo_bl(B, xr(yy), yy, lv_hex(OUT), 256); continue; }
        mo_hspan(B, yy, xl(yy), xr(yy), c, 256); }   /* v4: opaque */
    if (solid) for (int yy = y + 2; yy < y + 17; ++yy) mo_hspan(B, yy, xl(yy) + 3, xr(yy) - 3, lv_hex(HEAD), 256);   /* header strip */
    /* tab (slanted end) */
    for (int r = 0; r < 9; ++r) mo_hspan(B, y - 9 + r, x, x + tw - 6 + (r < 5 ? r : 5), lv_hex(TAB), 256);
    if (!solid) return;
    /* double outline: 2 px outer + 1 px inner, following the cuts */
    for (int yy = y; yy < y + h; ++yy) { mo_bl(B, xl(yy), yy, lv_hex(OUT), 256); mo_bl(B, xl(yy) + 1, yy, lv_hex(OUT), 256); mo_bl(B, xr(yy), yy, lv_hex(OUT), 256); mo_bl(B, xr(yy) - 1, yy, lv_hex(OUT), 256);
        if (yy >= y + 3 && yy <= y + h - 4) { mo_bl(B, xl(yy) + 3, yy, lv_hex(IN), 256); mo_bl(B, xr(yy) - 3, yy, lv_hex(IN), 256); } }
    mo_hspan(B, y, x + tw - 1, xr(y), lv_hex(OUT), 256); mo_hspan(B, y + 1, x + tw - 1, xr(y + 1), lv_hex(OUT), 256); mo_hspan(B, y + h - 1, xl(y + h - 1), xr(y + h - 1), lv_hex(OUT), 256); mo_hspan(B, y + h - 2, xl(y + h - 2), xr(y + h - 2), lv_hex(OUT), 256);
    mo_hspan(B, y + 3, xl(y + 3) + 3, xr(y + 3) - 3, lv_hex(IN), 256); mo_hspan(B, y + h - 4, xl(y + h - 4) + 3, xr(y + h - 4) - 3, lv_hex(IN), 256);
    mo_hspan(B, y - 10, x, x + tw - 6, lv_hex(OUT), 256);
    if (MO_LOOK == 3) { const uint16_t cap = lv_hex(0xfff0b0); const int L = 7;   /* brass corner caps */
      mo12_rect(B, x - 2, y - 2, L, 3, cap, 256); mo12_rect(B, x - 2, y - 2, 3, L, cap, 256); mo12_rect(B, x + w - L + 1, y - 2, L, 3, cap, 256); mo12_rect(B, x + w - 2, y - 2, 3, L, cap, 256);
      mo12_rect(B, x - 2, y + h - 1, L, 3, cap, 256); mo12_rect(B, x - 2, y + h - L + 1, 3, L, cap, 256); mo12_rect(B, x + w - L + 1, y + h - 1, L, 3, cap, 256); mo12_rect(B, x + w - 2, y + h - L + 1, 3, L, cap, 256); }
    /* content (positions = v13.1, so the text audit boxes are unchanged) */
    static const char *const MO13_FN[MO_N][3] = {   /* 2026-11-14 spread: WIFI + BLUETOOTH removed */
        {"DEAUTH.PKT", "FLOOD.SIG", "PMF.DEF"},
        {"OPEN.NET", "WEP.SIG", "WPA3.DEF"},        {"FLOCK.CAM", "SSID.SIG", "PLATE.DEF"},  {"DRONE.UAV", "REMOTEID.SIG", "REPORT.DEF"},
        {"AIRTAG.TRK", "FINDMY.SIG", "BAGCHK.DEF"}, {"TILE.TRK", "FEED.SIG", "TAGSCAN.DEF"}, {"META.CAM", "MAKERID.SIG", "LEDCHK.DEF"},
        {"SKIMMER.CRD", "HC05.SIG", "TAPPAY.DEF"},  {"FLIPPER.RF", "NAME.SIG", "POPUP.DEF"},  {"SMARTTAG.TRK", "FD5A.SIG", "TAGSCAN.DEF"},
        {"AXON.CAM", "AB3SSID.SIG", "CAMCHK.DEF"},  {"RAVEN.SNS", "UUID31.SIG", "ASK.DEF"},   {"ALPR.CAM", "MOTOOUI.SIG", "PLATE.DEF"},
        {"IPCAM.NET", "CAMOUI.SIG", "ROOMCHK.DEF"}, {"RING.CAM", "RINGOUI.SIG", "PORCH.DEF"}, {"FMDN.TRK", "FEAA.SIG", "TAGSCAN.DEF"},
        {"TWIN.AP", "2BSSID.SIG", "LOCKCHK.DEF"},   {"IBEACON.ADV", "4C0215.SIG", "BTOFF.DEF"}};
    static const char *const MO13_ACT[3] = {"WHAT IT IS", "HOW SPOTTED", "PROTECT"};
    char tab[16]; snprintf(tab, sizeof tab, "SCROLL %d", j + 1); b_text35(B, x + 5, y - 6, tab, lv_hex(TABT));
    if (B.y0 == 0) mo12_aud(x + 5, y - 6, lv_w35(tab), 5, x + 1, y - 7, x + tw - 5, y - 1, "tab");
    mo_emb_draw(B, i, x + 5, y + 5, 10, IK, IB, 0, 256, 0);   /* device emblem (vector, 10 px) in the header strip */
    const char *nm = MO_FOLD_NAME[i][j]; b_text(B, x + 17, y + 7, nm, ink);   /* real folder name: ID / DETECT / ACTION */
    const char *sz = MO_FOLD_TAG[i][j]; const int sw = (int)strlen(sz) * 6 - 1, sx = x + w - 6 - sw;   /* right tag: threat / radio / DO (was a fake KB size) */
    b_text(B, sx, y + 20, sz, lv_hex(SIZE));
    b_text35(B, x + 6, y + 22, MO_FOLD_ACT[i][j], lv_hex(ACT));
    const int bw = 70, fill = (bw - 4) * (tl < 0 ? 0 : tl > MO12_LOAD_MS ? MO12_LOAD_MS : tl) / MO12_LOAD_MS;
    mo12_rect(B, x + 6, y + 35, bw - 4, 3, lv_hex(BARB), 256); b_frame(B, x + 4, y + 33, bw, 7, lv_hex(BARF));
    for (int q = 0; q < fill; ++q) { const uint16_t c = mo3_mx(MO_LOOK == 3 ? 0x00a83c : 0x0a8a3a, MO_LOOK == 1 ? 0xb8ffd0 : 0x10d858, q / 66.0f); mo12_rect(B, x + 6 + q, y + 35, 1, 3, c, 256); }
    char st[16]; if (tl >= MO12_LOAD_MS) snprintf(st, sizeof st, "LOADED"); else snprintf(st, sizeof st, "%d%%", tl < 0 ? 0 : 100 * tl / MO12_LOAD_MS);
    b_text35(B, x + w - 6 - lv_w35(st), y + 34, st, lv_hex(ACT));
    /* landing: a bright trace runs the outline (T1), a light sheen sweeps the face (T2), a gold glint runs the brass (T3) */
    if (tl >= -MO12_DROP_MS * 35 / 100 && tl < 700) { const float u = (tl + MO12_DROP_MS * 35 / 100) / (700.0f + MO12_DROP_MS * 35 / 100);
        if (MO_LOOK == 2) {
        const int sx0 = x - 20 + (int)(u * (w + 40));
        for (int yy = y + 1; yy < y + h - 1; ++yy) for (int d = -4; d <= 4; ++d) { const int xx = sx0 + d - (yy - y) / 2; if (xx > xl(yy) + 1 && xx < xr(yy) - 1) mo_bl(B, xx, yy, lv_hex(0xffffff), 150 - abs(d) * 30); }
        } else {
        const int P = 2 * (w + h); const int hp = (int)(u * P * 1.05f);
        for (int d = 0; d < 26; ++d) { int px, py; mo3_perim(x - 1, y - 1, w + 1, h + 1, hp - d, px, py); const int al = 255 * (26 - d) / 26;
            mo_bl(B, px, py, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al); mo_bl(B, px + 1, py, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); mo_bl(B, px, py + 1, lv_hex(MO_LOOK == 1 ? 0xeafff1 : 0xfff8d8), al / 2); }
        }
    }
    if (B.y0 == 0) { mo12_aud(x + 17, y + 7, (int)strlen(nm) * 6 - 1, 7, x + 16, y + 3, x + w - 4, y + 17, "fname");
                     mo12_aud(sx, y + 20, sw, 7, x + 60, y + 18, x + w - 4, y + 30, "fsize");
                     mo12_aud(x + 6, y + 22, lv_w35(MO_FOLD_ACT[i][j]), 5, x + 2, y + 18, sx - 3, y + 30, "fact");
                     mo12_aud(x + w - 6 - lv_w35(st), y + 34, lv_w35(st), 5, x + bw + 6, y + 32, x + w - 3, y + h - 2, "fst"); }
}
