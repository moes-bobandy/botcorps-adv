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
static const int MO_EMB_SFIX[MO_N] = {6, 18, 10, 11, 5, 6, 9, 13, 15, 4, 14, 7, 6, 17, 5, 13, 9, 6};   /* v5e: fixed idle emblem size per tower */
static int mo_embS4(int i, int fw)
{
    if (fw >= 50) return mo_eS_last[i] = 40;
    { int S = MO_EMB_SFIX[i], lim = fw - 3; if (S > lim) S = lim < 4 ? 0 : lim; return mo_eS_last[i] = S; }   /* v5e: fixed, inset >= 1 px */
    int fit = fw - 1; if (fit > 36) fit = 36; if (fit < 6) { mo_eS_last[i] = 0; return 0; }
    int &L = mo_eS_last[i]; if (L == 40 || L == 0 || L > fit + 1 || L < fit - 3) L = fit & ~1; return L;   /* v5: shrink only when the face is 2+ px too narrow (a 1 px overhang onto the edge line is drawn over it), so a +-1 px face wobble never flips the size */
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
static const uint32_t MO_EMB_COL[MO_N] = {0x3dcccc, 0xace600, 0xccc35c, 0xff0055, 0x26ffc9, 0xb95ccc, 0x00ff6a, 0xffd400, 0x0080ff, 0x0000ff, 0xcc5c6a, 0xff4000, 0x0033cc, 0xff00ff, 0x00bfff, 0xff9500, 0xff00aa, 0x78cc5c};   /* v5: per-device emblem colour (src/emb_colors.py) */
static const uint32_t MO_EMB_DET[MO_N] = {0x1c120c, 0x1c120c, 0x1c120c, 0xffffff, 0x1c120c, 0xffffff, 0x1c120c, 0x1c120c, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0xffffff, 0x1c120c, 0x1c120c, 0xffffff, 0x1c120c};
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

#ifndef MO_BODY
#define MO_BODY 0
#endif
static inline uint32_t mo_tc(int i) { const int th = MO_THREAT[i]; return th == 0 ? 0xff385c : th == 1 ? 0xffb000 : 0x30ff90; }   /* threat colour */
static inline float mo_sig(int i) { int l = mo_db(i) + 100; if (l < 4) l = 4; if (l > 60) l = 60; return (l - 4) / 56.0f; }   /* example signal 0..1 (lv_rd_level) */
static inline uint32_t mo_mixc(uint32_t a, uint32_t b, float t) { if (t < 0) t = 0; if (t > 1) t = 1; uint32_t o = 0;
    for (int sh = 0; sh <= 16; sh += 8) { const int ca = (a >> sh) & 255, cb = (b >> sh) & 255; o |= (uint32_t)(ca + (cb - ca) * t + 0.5f) << sh; } return o; }
/* three-stop horizontal shading: lit (left) -> mid -> shadow (right), plus a gentle darkening toward the base */
static inline uint32_t mo_shade3(uint32_t lit, uint32_t mid, uint32_t dk, float u, float v) {
    const uint32_t c = u < 0.35f ? mo_mixc(lit, mid, u / 0.35f) : mo_mixc(mid, dk, (u - 0.35f) / 0.65f); return mo_mixc(c, dk, v * 0.22f); }
static inline uint32_t mo_bcol(int i) {   /* the body base colour per look */
    return MO_BODY == 4 ? 0xe0b040 : MO_BODY == 5 ? MO_EMB_COL[i] : 0xf2e6d0; }
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
    const int L = MO_BODY >= 4 ? 3 : (MO_LOOKOF[i] == 1 ? 1 : 3);   /* towers3c: all 18 take the new body; MO_LOOKOF still picks the height group */   /* v5d: the 5 HIGH towers = T1 black Matrix code glass, the other 13 = see-through wood (T3) */
    if (L == 1) {
    /* ===== T1 MATRIX CODE GLASS: smoked green-black glass, falling code inside, hologram device in the core, glowing edges ===== */
    if (cp < 0.3f) mo3_shadow(B, x0, x1, yb, 0x1e2a20, a * 90 / 256);
    /* v4: no soft outer glow (it muddied the edges) */
    auto side = [&](int xf, int xbk) { const int dir = xbk > xf ? 1 : -1, n = abs(xbk - xf); if (n < 1) return;
        for (int s = 0; s <= n; ++s) { const int x = xf + dir * s; const float u = s / (float)n; const int top = (int)lroundf(yt + (byt - yt) * u), bot = (int)lroundf(yb + (byb - yb) * u);
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, mo3_mx(0x05160d, 0x000201, u * 0.7f + 0.3f * (y - top) / (float)(bot - top + 1)), a);
            if (s % 3 == 1) { const int hd = top + (int)fmodf(t * (22 + (x * 7) % 17) + x * 13, (float)(bot - top + 18)); for (int q = 0; q < 10; ++q) { const int y = hd - q; if (y >= top && y <= bot) mo13_px(B, x, y, lv_hex(q ? MO11_G : MO11_GH), a * (q ? 120 - q * 10 : 200) / 256); } } }
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(0x0a6a30), a); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO11_GH), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(0x1a5a34), a); mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO11_GH), a); }
    /* front glass: smooth vertical ramp + a soft vertical reflection band */
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1); const uint16_t base = mo3_mx(0x03120a, 0x000201, v);
        const int hx = x0 + 1 + fw / 5; for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, x == hx ? mo3_mx(0x03120a, 0x3a8a58, 0.45f - 0.3f * v) : base, a); }   /* v4: opaque glass + one crisp highlight column */
    const bool big = fw >= 40;
    /* v5e: Matrix rain = lv_radar.h's CYD code streaks (field rain law, same constants + colours), mapped into this tower's face */
    { const int W_ = fw - 3, H_ = yv - yt - 1, ns = fw >= 40 ? 18 : (fw / 2 > 3 ? fw / 2 : 3);   /* v5f: denser */
      if (W_ > 0 && H_ > 4) for (int s_ = 0; s_ < ns; ++s_) {
        const int x = x0 + 2 + (s_ * 53 + 17 + i * 7) % W_, len = 10 + (s_ * 7) % 18, sp = 40 + (s_ * 29) % 70;
        const int head = yt + 1 + (int)(lv_now / 1000.0f * sp + s_ * 37 + i * 11) % (H_ + len);
        for (int k = 0; k < len; ++k) { const int y = head - k; if (y > yt && y < yv) mo13_px(B, x, y, lv_hex(k < 2 ? 0xd8ffe6 : k < len / 2 ? 0x10d858 : 0x00a83c), k < len / 2 ? a : a * (8 - (k - len / 2) * 7 / (len - len / 2)) / 8); } } }   /* v5f: bright head + continuous fading trail */
    /* hologram device in the core */
    const int iy = yt + (S >= 32 ? 6 : 3);   /* v4: no bob */
    if (S) { const int ix = x0 + 1 + (fw - 1 - S) / 2; mo_emb_draw(B, i, ix, iy, S, MO_EMB_COL[i], MO_EMB_DET[i], 0x010805, a, a); }
    else if (fw >= 5) { const int gx = (x0 + x1) / 2 - 1; for (int r = 0; r < 5; ++r) for (int c = 0; c < 3; ++c) if (MO13_GLY[MO_KIND[i]][r][c] == '#') mo13_px(B, gx + c, yt + 3 + r, lv_hex(0xc8ffdc), a); }
    /* the file list window */
    const int ly0 = (S ? iy + S + (big ? 17 : 3) : yt + 10), ly1 = yv - (big ? 4 : 2);
    if (big) { for (int y = ly0 - 3; y <= ly1 + 1; ++y) mo13_span(B, y, x0 + 2, x1 - 2, lv_hex(0x010604), a);
        mo13_line(B, x0 + 2, ly0 - 4, x1 - 2, ly0 - 4, lv_hex(MO11_GH), a); mo3_t35(B, x0 + 3, ly0 - 11, "FILES", lv_hex(MO11_GH), a, 0, 999, x1);
        { char n[8]; snprintf(n, sizeof n, "%d", 8); mo3_t35(B, x1 - 5, ly0 - 11, n, lv_hex(0x5ac080), a, 0, 999, x1); } }
    mo_txt_oc = 0x010805; mo3_files(B, i, x0 + 2, x1 - 2, ly0, ly1, lv_hex(MO11_GH), lv_hex(big ? 0xd8ffe6 : 0x7dffb0), lv_hex(0x5ac080), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;
    /* glowing edges */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x7dffb0), a); mo13_line(B, x1, yt, x1, yv, lv_hex(MO11_GH), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xd8ffe6), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x0a6a30), a);
    if (S) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, iy, S, MO_EMB_COL[i], MO_EMB_DET[i], 0x010805, a, a);   /* v4: emblem on top of the edges */
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
            if (x < B.x0 || x >= B.x0 + B.w || bot < B.y0 || top >= B.y0 + B.h) continue;   /* v1.2.10 speed: column outside this band (mo13_px would clip every pixel) */
            const uint16_t sc = MO_BODY == 4 ? lv_hex(mo_mixc(0xa8741c, 0x5a3a0c, u)) : MO_BODY == 5 ? lv_hex(mo_mixc(mo_mixc(MO_EMB_COL[i], 0x000000, 0.45f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.7f), u)) : MO_BODY == 6 ? lv_hex(mo_mixc(0xb01c26, 0x5a0a10, u)) : mo3_mx(0x6a4020, 0x2a160a, u);   /* v1.2.10 speed: depends on the column only (same value as before) */
            for (int y = top; y <= bot; ++y) mo13_px(B, x, y, sc, a); }   /* v5e: solid side; towers3b: shaded side */
        mo13_line(B, xbk, byt, xbk, byb, lv_hex(MO_BODY == 0 ? 0x8a5a2a : MO_BODY == 4 ? 0x3a2406 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0x000000, 0.8f) : 0x3a0408), MO_BODY ? a : a * 200 / 256); mo13_line(B, xf, yt, xbk, byt, lv_hex(MO_BODY == 0 ? 0xd8aa52 : MO_BODY == 4 ? 0xfff4c0 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0xffffff, 0.5f) : 0xff6a6a), a); };
    if (bx1 > x1) side(x1, bx1);
    if (bx0 < x0) side(x0, bx0);
    if (byt < yt) { const float X[4] = {(float)x0, (float)bx0, (float)bx1, (float)x1}, Y[4] = {(float)yt, (float)byt, (float)byt, (float)yt}; mo_quad(B, X, Y, lv_hex(MO_BODY == 4 ? 0xfff2b0 : MO_BODY == 5 ? mo_mixc(MO_EMB_COL[i], 0xffffff, 0.45f) : MO_BODY == 6 ? 0xe8404a : 0xb08a3a), a);   /* v5e: solid cap; towers3b: cap highlight */ mo13_line(B, bx0, byt, bx1, byt, lv_hex(MO_BODY == 6 ? 0xff9a9a : 0xfff0b0), a); }
    /* translucent wood front: grain at ~38% (v3; v2 was 55%) so the room / rain behind shows through, a soft sheen near the lit edge, a faint warm inner light */
#if MO_BODY == 5   /* v1.2.10 speed: mo_shade3's first stage depends on the column only; computed once per column (identical values) */
    const uint32_t e_lit = mo_mixc(MO_EMB_COL[i], 0xffffff, 0.30f), e_mid = mo_mixc(MO_EMB_COL[i], 0x000000, 0.12f), e_dk = mo_mixc(MO_EMB_COL[i], 0x000000, 0.50f);
    uint32_t e_col[240]; const int ex0 = x0 < B.x0 ? B.x0 : x0, ex1 = x1 > B.x0 + B.w - 1 ? B.x0 + B.w - 1 : x1;
    for (int x = ex0; x <= ex1; ++x) { const float u = (x - x0) / (float)fw; e_col[x - ex0] = u < 0.35f ? mo_mixc(e_lit, e_mid, u / 0.35f) : mo_mixc(e_mid, e_dk, (u - 0.35f) / 0.65f); }
    for (int y = (yt > B.y0 ? yt : B.y0); y <= yv && y < B.y0 + B.h; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);
        for (int x = ex0; x <= ex1; ++x) mo13_px(B, x, y, lv_hex(mo_mixc(e_col[x - ex0], e_dk, v * 0.22f)), a); }
    if (0)
#endif
    for (int y = yt; y <= yv; ++y) { const float v = (y - yt) / (float)(yb - yt + 1);
        for (int x = x0; x <= x1; ++x) { const float u = (x - x0) / (float)fw, sh = 0.0f;   /* v1.2.10: the unused sheen expf() removed (its result was never drawn) */
#if MO_BODY == 4   /* D UNIFORM: polished SCANbot-style gold, lit left -> shadow right */
            mo13_px(B, x, y, lv_hex(mo_shade3(0xfff4c0, 0xe0b040, 0x8a5a14, u, v)), a);
#elif MO_BODY == 5   /* E VARIED: each body enamelled in its own device colour (the emblem colour), lit left -> shadow right */
            mo13_px(B, x, y, lv_hex(mo_shade3(mo_mixc(MO_EMB_COL[i], 0xffffff, 0.30f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.12f), mo_mixc(MO_EMB_COL[i], 0x000000, 0.50f), u, v)), a);
#elif MO_BODY == 6   /* F LANTERN: white porcelain lit from inside; the warm core glow strength = the device's example signal */
            { const float g = mo_sig(i) * 0.75f * expf(-((u - 0.5f) * (u - 0.5f)) / 0.06f);
              mo13_px(B, x, y, lv_hex(mo_mixc(mo_shade3(0xfffaf0, 0xf2e6d0, 0xc0a888, u, v), 0xffb648, g)), a); }
#else
            mo13_px(B, x, y, grain(x, y, u), a);   /* v5e: SOLID wood (Mobandy 3:25 PM ET) */
#endif
            (void)sh; (void)v; } }   /* v4: blurred sheen + warm glow removed (muddy) */
    /* thin brass bands with a green trim line (the pillar's frame) */
#if MO_BODY == 0
    for (int k = 0; k < bh; ++k) { mo13_span(B, bt0 + k, x0, x1, brass(k, bh), a); mo13_span(B, bb0 + k, x0, x1, brass(k, bh), a); }
    if (fw >= 8) { mo13_span(B, bt0 + bh, x0 + 1, x1 - 1, lv_hex(MO11_G), a); mo13_span(B, bb0 - 1, x0 + 1, x1 - 1, lv_hex(MO11_G), a); }
#elif MO_BODY == 5
    /* EB: B's LED column on the right rail, in a dark recessed well. Slots run from under the emblem to the base band.
     * top slot = threat lamp (steady, threat colour); lit LEDs = the example signal level x slots; lit ones flicker at 1 + 7 x signal Hz.
     * LED size 4 / 3 / 2 px by face width (close-up / near / far); 1 px darker bevel on the lamp so it reads on any enamel. */
    { const int lw = fw >= 40 ? 4 : fw >= 20 ? 3 : 2, pitch = lw + 2, wx1 = x1 - 2, wx0 = wx1 - lw - 1;
      const int ytop = bt0 + bh + 2 * (fw >= 40 ? 4 : 2) + (S ? S + 2 : 0), ybot = bb0 - 2;
      const int ns = (ybot - ytop + 2) / pitch;
      if (fw >= 8 && ns >= 2) { const float sg = mo_sig(i); const int nl = (int)lroundf(sg * (ns - 1));
        for (int y = ytop - 1; y <= ytop + ns * pitch - 1; ++y) for (int x = wx0; x <= wx1; ++x) mo13_px(B, x, y, lv_hex(0x08090b), a);   /* the well */
        for (int k = 0; k < ns; ++k) { const int ly = ytop + k * pitch, kk = ns - 1 - k;   /* k = 0 at the top */
          uint32_t c = 0x1c2420;
          if (k == 0) c = mo_tc(i);
          else if (kk < nl) c = fmodf(t * (1.0f + 7.0f * sg) + kk * 0.37f + i * 0.11f, 1.0f) < 0.6f ? 0x5cff9c : 0x0e5a2c;
          for (int y = ly; y < ly + lw; ++y) for (int x = wx0 + 1; x < wx0 + 1 + lw; ++x) mo13_px(B, x, y, lv_hex(c), a);
          if (lw >= 3 && (k == 0 || kk < nl)) mo13_px(B, wx0 + 1, ly, lv_hex(mo_mixc(c, 0xffffff, 0.6f)), a); } } }   /* specular dot on lit LEDs */
#elif MO_BODY == 6
    /* F: red lacquer cap + base (the lantern's frame), shaded lit -> shadow, a bright lip on the cap */
    for (int y = yt; y <= bt0 + bh && y <= yv; ++y) for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, lv_hex(y == yt ? 0xff8080 : mo_shade3(0xff4a50, 0xc81e28, 0x6a0a10, (x - x0) / (float)fw, 0)), a);
    for (int y = bb0; y <= yv; ++y) for (int x = x0; x <= x1; ++x) mo13_px(B, x, y, lv_hex(mo_shade3(0xff4a50, 0xc81e28, 0x6a0a10, (x - x0) / (float)fw, 0)), a);
#endif
    /* inside the wood: the device emblem + the scrolling file line-items, seen through */
    const int m = fw >= 40 ? 5 : fw >= 12 ? 2 : 1, px0 = x0 + m, px1 = x1 - m - (MO_BODY == 5 ? (fw >= 40 ? 4 : fw >= 20 ? 3 : 2) + 3 : 0), py0 = bt0 + bh + (fw >= 40 ? 4 : 2), py1 = (bb0 - (fw >= 40 ? 4 : 2) < yv ? bb0 - (fw >= 40 ? 4 : 2) : yv);
    if (px1 - px0 >= 2 && py1 - py0 >= 4) {
        const bool big = fw >= 40; const int Sl = S;   /* v4: same big idle emblem rule as T1 / T2 */
        const int iy = py0 + (big ? 4 : 2);
        if (Sl) { const int ix = x0 + 1 + (fw - 1 - Sl) / 2; mo_emb_draw(B, i, ix, iy, Sl, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1); }   /* v5: per-device colour */
        const int ly0 = Sl ? iy + Sl + (big ? 8 : 2) : py0 + 1, ly1 = py1 - 1;
        if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a);
        const bool lt5 = ((MO_EMB_COL[i] >> 16 & 255) * 3 + (MO_EMB_COL[i] >> 8 & 255) * 6 + (MO_EMB_COL[i] & 255)) / 10 > 120;   /* E: light enamel -> dark ink, dark enamel -> white ink */
        mo_txt_oc = MO_BODY >= 4 ? -1 : 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO_BODY == 4 ? 0x5a3a08 : MO_BODY == 6 ? 0xa8141e : MO_BODY == 5 ? (lt5 ? 0x101010 : 0xffffff) : MO11_GH), lv_hex(MO_BODY == 4 ? 0x2a1604 : MO_BODY == 6 ? 0x2a1a10 : MO_BODY == 5 ? (lt5 ? 0x101010 : 0xffffff) : 0xfff0c8), lv_hex(MO_BODY == 4 ? 0x4a2a04 : MO_BODY == 6 ? 0x5a3a20 : MO_BODY == 5 ? (lt5 ? 0x202020 : 0xf0f0f0) : 0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: cream text, dark outline, opaque */
    }
    /* edges: lit brass left / top, dark right, green inner rail */
#if MO_BODY == 0
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }
#elif MO_BODY == 4
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xfffbe8), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x4a2e08), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xffffff), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2406), a);
#elif MO_BODY == 5   /* E: crisp light rim (lit edge) whose brightness = the device's example signal, dark rim on the shadow side */
    { const uint32_t rim = mo_mixc(MO_EMB_COL[i], 0xffffff, 0.25f + 0.65f * mo_sig(i));
      mo13_line(B, x0, yt, x0, yv, lv_hex(rim), a); mo13_line(B, x0, yt, x1, yt, lv_hex(rim), a); if (fw >= 16) { mo13_line(B, x0 + 1, yt, x0 + 1, yv, lv_hex(rim), a); mo13_line(B, x0, yt + 1, x1, yt + 1, lv_hex(rim), a); }
      mo13_line(B, x1, yt, x1, yv, lv_hex(mo_mixc(MO_EMB_COL[i], 0x000000, 0.75f)), a); mo13_line(B, x0, yb, x1, yb, lv_hex(mo_mixc(MO_EMB_COL[i], 0x000000, 0.75f)), a); }
#else
    mo13_line(B, x0, yt, x0, yv, lv_hex(0x8a1018), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x4a0408), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x4a0408), a);
#endif
#if MO_BODY == 4 || MO_BODY == 5
    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) { const int ex = x0 + 1 + (fw - 1 - S) / 2, ey = py0 + (fw >= 40 ? 4 : 2);   /* the emblem's plate: 1 px around the emblem */
        for (int y = ey - 1; y <= ey + S && y <= py1; ++y) mo13_span(B, y, ex - 1, ex + S, lv_hex(y == ey + S ? (MO_BODY == 4 ? 0xfff4c0 : 0x5a5a64) : 0x14100c), a); }
#endif
    if (S && px1 - px0 >= 2 && py1 - py0 >= 4) mo_emb_draw(B, i, x0 + 1 + (fw - 1 - S) / 2, py0 + (fw >= 40 ? 4 : 2), S, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1);   /* v4: emblem on top of the edges; v5 per-device colour */
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
    const bool act = false;   /* v5f: no decorative glow cycle */
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
    /* v5f (Mobandy 3:24 / 3:48 PM ET): digital instruments, every element = a real value (folders18 + EX.DATA rssi + threat table) */
    auto seg7 = [&](int sx_, int sy_, char ch, uint16_t on, uint16_t off) {   /* 5x9 seven-segment digit, unlit segments dim */
        static const char *const D7 = "0123456789-ABCDEF"; static const uint8_t M7[17] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x40,0x77,0x7c,0x39,0x5e,0x79,0x71};
        const char *p_ = strchr(D7, ch); const uint8_t m = p_ ? M7[p_ - D7] : 0;
        auto sg = [&](int k, int rx, int ry, int rw, int rh) { mo12_rect(B, sx_ + rx, sy_ + ry, rw, rh, (m >> k) & 1 ? on : off, 256); };
        sg(0, 1, 0, 3, 1); sg(1, 4, 1, 1, 3); sg(2, 4, 5, 1, 3); sg(3, 1, 8, 3, 1); sg(4, 0, 5, 1, 3); sg(5, 0, 1, 1, 3); sg(6, 1, 4, 3, 1); };
    auto seg7s = [&](int sx_, int sy_, const char *s_, uint16_t on, uint16_t off) { for (int k = 0; s_[k]; ++k) seg7(sx_ + k * 7, sy_, s_[k], on, off); return sx_ + (int)strlen(s_) * 7; };
    const uint16_t LIT = lv_hex(MO_LOOK == 2 ? 0x00a83c : 0x30ff90), DIM = lv_hex(MO_LOOK == 2 ? 0xcfe8d8 : MO_LOOK == 3 ? 0x3a2414 : 0x0a3a20), LBL = lv_hex(ACT);
    const int th = MO_THREAT[i]; const uint16_t TC = lv_hex(th == 0 ? 0xff385c : th == 1 ? 0xffb000 : 0x30c060);
    b_text35(B, x + 6, y + 19, MO_FOLD_ACT[i][j], lv_hex(ACT));   /* the folder's real fact line */
    const int iy0 = y + 27, iy = y + 34;   /* instrument labels / instruments */
    if (j == 0) {   /* WHAT IT IS: signal (EX.DATA dBm, 7-seg + 10-LED meter) + threat level (3 segments + warning icon) */
        char db[8]; snprintf(db, sizeof db, "%d", mo_db(i)); b_text35(B, x + 6, iy0, "DBM", LBL); seg7s(x + 6, iy, db, LIT, DIM);
        int lev = mo_db(i) + 100; if (lev < 4) lev = 4; if (lev > 60) lev = 60; const int nl = (lev * 10 + 30) / 60;   /* lv_rd_level -> 10 LEDs */
        b_text35(B, x + 40, iy0, "SIGNAL", LBL);
        for (int k = 0; k < 10; ++k) { const int bh_ = 2 + k * 7 / 9; mo12_rect(B, x + 40 + k * 5, iy + 9 - bh_, 4, bh_, k < nl ? LIT : DIM, 256); }
        b_text35(B, x + 96, iy0, "THREAT", LBL);
        for (int r = 0; r < 8; ++r) mo_hspan(B, iy + r, x + 100 - r / 2, x + 100 + r / 2, TC, 256);   /* warning triangle */
        mo12_rect(B, x + 100, iy + 2, 1, 3, lv_hex(0x000000), 256); mo12_rect(B, x + 100, iy + 6, 1, 1, lv_hex(0x000000), 256);
        for (int k = 0; k < 3; ++k) mo12_rect(B, x + 107 + k * 9, iy + 2, 7, 5, k < 3 - th ? TC : DIM, 256);
    } else if (j == 1) {   /* HOW SPOTTED: radio badge (lit), band badge from the BAND / GHZ item, numeric items as 7-seg digits */
        auto badge = [&](int bx, const char *s_) { const int bw_ = (int)strlen(s_) * 4 + 5; mo12_rect(B, bx, iy - 1, bw_, 9, LIT, 256); b_text35(B, bx + 3, iy + 1, s_, lv_hex(MO_LOOK == 2 ? 0xffffff : 0x03140b)); return bx + bw_ + 3; };
        b_text35(B, x + 6, iy0, "RADIO", LBL); int bx = badge(x + 6, MO_FOLD_TAG[i][1]);
        for (int e = 0; e < 8; ++e) { const char *v = MO_ITEM_V[i][e]; const char *g_ = strstr(v, "GHZ"); if (g_) { char bb[8]; snprintf(bb, sizeof bb, "%.*sG", (int)(g_ - v > 4 ? 4 : g_ - v), v); for (char *q = bb; *q; ++q) if (*q == ' ') memmove(q, q + 1, strlen(q)); bx = badge(bx, bb); break; } }
        int nx = x + w - 6, cnt = 0;
        for (int e = 7; e >= 0 && cnt < 2; --e) { const char *v = MO_ITEM_V[i][e]; const int L_ = (int)strlen(v); bool num = L_ > 0 && L_ <= 3; for (int k = 0; k < L_; ++k) if (v[k] < '0' || v[k] > '9') num = false;
            if (!num) continue; const char *kk = MO_ITEM_K[i][e]; char k7[8]; snprintf(k7, sizeof k7, "%.7s", kk); const int ww = L_ * 7 > lv_w35(k7) ? L_ * 7 : lv_w35(k7);
            if (nx - ww < bx + 2) break; nx -= ww; b_text35(B, nx, iy0, k7, LBL); seg7s(nx, iy, v, LIT, DIM); nx -= 6; ++cnt; }
    } else {   /* PROTECT: shield + check icon + DO THIS: marks the header line as the action to take */
        for (int r = 0; r < 10; ++r) { const int hw = r < 6 ? 4 : 4 - (r - 5); mo_hspan(B, iy + r - 1, x + 10 - hw, x + 10 + hw, LIT, 256); }
        const uint16_t dk = lv_hex(MO_LOOK == 2 ? 0xffffff : 0x03140b); mo12_rect(B, x + 8, iy + 4, 1, 1, dk, 256); mo12_rect(B, x + 9, iy + 5, 1, 1, dk, 256); for (int k = 0; k < 3; ++k) mo12_rect(B, x + 10 + k, iy + 4 - k, 1, 1, dk, 256);
        b_text(B, x + 18, iy + 1, "DO THIS", lv_hex(NAME));   /* the action itself is the header line */
    }
    (void)tl; (void)BARF; (void)BARB; (void)SIZE;
    if (B.y0 == 0) { mo12_aud(x + 17, y + 7, (int)strlen(nm) * 6 - 1, 7, x + 16, y + 3, x + w - 4, y + 17, "fname");
                     mo12_aud(x + 6, y + 19, lv_w35(MO_FOLD_ACT[i][j]), 5, x + 2, y + 18, x + w - 4, y + 25, "fact"); }
}
