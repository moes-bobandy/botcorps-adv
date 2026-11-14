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
    const int L = MO_LOOKOF[i] == 1 ? 1 : 3;   /* v5d: the 5 HIGH towers = T1 black Matrix code glass, the other 13 = see-through wood (T3) */
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
        if (Sl) { const int ix = x0 + 1 + (fw - 1 - Sl) / 2; mo_emb_draw(B, i, ix, iy, Sl, MO_EMB_COL[i], MO_EMB_DET[i], 0x1c120c, a, a, py1); }   /* v5: per-device colour */
        const int ly0 = Sl ? iy + Sl + (big ? 8 : 2) : py0 + 1, ly1 = py1 - 1;
        if (big) mo13_line(B, px0 + 2, ly0 - 4, px1 - 2, ly0 - 4, lv_hex(0xd8aa52), a);
        mo_txt_oc = 0x1c120c; mo3_files(B, i, px0 + 1, px1 - 1, ly0, ly1, lv_hex(MO11_GH), lv_hex(0xfff0c8), lv_hex(0xffd878), a, big ? 9.0f : 6.0f); mo_txt_oc = -1;   /* v4: cream text, dark outline, opaque */
    }
    /* edges: lit brass left / top, dark right, green inner rail */
    mo13_line(B, x0, yt, x0, yv, lv_hex(0xd8aa52), a); mo13_line(B, x1, yt, x1, yv, lv_hex(0x3a2010), a); mo13_line(B, x0, yt, x1, yt, lv_hex(0xfff0b0), a); mo13_line(B, x0, yb, x1, yb, lv_hex(0x3a2010), a);
    if (fw >= 10) { mo13_line(B, x0 + 1, bt0 + bh + 1, x0 + 1, bb0 - 2, lv_hex(MO11_G), a); }
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
    /* v5d (Mobandy 2:36-2:38 PM ET): the tapped-open folder IS HACKbot DOWNLOAD's folder (lv_scanbot_download.h lv_dl_drop: the same
     * lvi_* outline lines, slanted tab, drop shadow, 4-px scanline body, neon inks, FOLDER n.m tab text, lvi_icon, 85x6 bar + status),
     * body 40 px tall (HACKbot 32) so one more text row fits. Every field is real device data from folders18 (MO_FOLD_NAME / MO_FOLD_TAG /
     * MO_FOLD_ACT), the device's threat level (MO_LOOKOF) and its EXAMPLE signal (MO11_RSSI, EX.DATA) through Gibson's own
     * lv_rd_level 11-step strength formula. Opaque (lvi_rect / lvi_line / lvi_txt write solid pixels). */
    static const unsigned neon[4] = {0x30e8ff, 0xff2ad4, 0xffb000, 0x30ff90};
    const int x = MO12_FX, y = ys, H = MO12_FH; const bool got = tl >= MO12_LOAD_MS; const unsigned c = got ? 0x30ff90 : neon[j];
    lvi_line(B, 84, y + 12, 89, y + 12, c);
    if (ph >= 0.65f) lvi_rect(B, x + 3, y + 2, 145, H, 0x061321);
    lvi_line(B, x + 145, y + 2, x + 148, y + 5, c); lvi_line(B, x + 148, y + 5, x + 148, y + H, c); lvi_line(B, x + 3, y + H + 1, x + 148, y + H + 1, c);
    if (ph >= 0.65f) { lvi_rect(B, x, y, 145, H, got ? 0x06200f : 0x050c16); for (int yy = y + 2; yy < y + H - 1; yy += 4) lvi_rect(B, x + 1, yy, 143, 1, 0x081422); }
    if (ph >= 0.65f) lvi_rect(B, x, y - 7, 88, 8, 0x050c16);
    lvi_line(B, x, y - 8, x + 83, y - 8, c); lvi_line(B, x + 83, y - 8, x + 88, y - 3, c); lvi_line(B, x + 88, y - 3, x + 88, y, c); lvi_line(B, x, y - 8, x, y + H - 1, c);
    lvi_line(B, x + 88, y, x + 144, y, c); lvi_line(B, x + 144, y, x + 144, y + H - 1, c); lvi_line(B, x, y + H - 1, x + 144, y + H - 1, c);
    if (ph < 0.65f) return;
    char t[48];
    snprintf(t, sizeof t, "FOLDER %d.%d", i + 1, j + 1); lvi_txt(B, x + 4, y - 6, t, c);
    /* row 1: detection icon (lvi_icon: 0 ID card, 2 radio, 3 shield; 1 = alert triangle on a HIGH device's ID) + real folder name + risk chip */
    const int lk = MO_LOOKOF[i]; const char *rk = lk == 1 ? "HIGH" : lk == 3 ? "MED" : "LOW"; const unsigned rc = lk == 1 ? 0xff385c : lk == 3 ? 0xffb000 : 0x30ff90;
    lvi_icon(B, x + 3, y + 3, j == 0 ? (lk == 1 ? 1 : 0) : j == 1 ? 2 : 3, c);
    const char *nm = MO_FOLD_NAME[i][j]; lvi_txt(B, x + 14, y + 5, nm, c);
    const int cw = lv_w35(rk) + 6, cx = x + 141 - cw; lvi_rect(B, cx, y + 3, cw, 9, rc); lvi_txt(B, cx + 3, y + 5, rk, 0x050c16);
    /* row 2: the real tag (threat / radio / DO) + the EX.DATA signal: dBm and Gibson's 11-step strength meter (lvi_band inks) */
    const char *sz = MO_FOLD_TAG[i][j]; lvi_txt(B, x + 4, y + 14, sz, 0x9fb8c8);
    const int lev = MO11_RSSI[i] + 100 < 4 ? 4 : MO11_RSSI[i] + 100 > 60 ? 60 : MO11_RSSI[i] + 100, sl = (lev - 4) * 11 / 56 + 1;
    char db[16]; snprintf(db, sizeof db, "%dDBM EX", MO11_RSSI[i]); const int mx = x + 141 - 11 * 3, dx = mx - 3 - lv_w35(db);
    lvi_txt(B, dx, y + 14, db, 0x6a8090);
    for (int k = 0; k < 11; ++k) { const unsigned bc = k < sl ? (k < 6 ? 0x30ff90 : k < 9 ? 0xffb000 : 0xff385c) : 0x12313a; lvi_rect(B, mx + k * 3, y + 19 - (k * 5) / 10, 2, 1 + (k * 5) / 10, bc); }
    /* row 3: the real detail / action line */
    lvi_txt(B, x + 4, y + 23, MO_FOLD_ACT[i][j], c == 0x30ff90 ? 0xb8ffd0 : 0xd8e8f0);
    /* row 4: HACKbot's 85x6 pull bar = this folder's load, status, and a mock-clock timestamp (seconds since the tap) */
    const int fill = 81 * (tl < 0 ? 0 : tl > MO12_LOAD_MS ? MO12_LOAD_MS : tl) / MO12_LOAD_MS;
    lvi_box(B, x + 4, y + 31, 85, 6, c); lvi_rect(B, x + 6, y + 33, fill, 2, c);
    if (got) snprintf(t, sizeof t, "GOT"); else snprintf(t, sizeof t, "%d%%", 100 * (tl < 0 ? 0 : tl) / MO12_LOAD_MS);   /* HACKbot GOT / PULL n% (short form so the timestamp fits) */
    lvi_txt(B, x + 94, y + 32, t, got ? c : 0x6a8090);
    { const int ms = (int)(lv_now - itw_mo_t0); char ts[12]; snprintf(ts, sizeof ts, "T+%d.%dS", ms / 1000, (ms / 100) % 10); lvi_txt(B, x + 141 - lv_w35(ts), y + 32, ts, 0x6a8090); }
    if (B.y0 == 0) { mo12_aud(x + 14, y + 5, lv_w35(nm), 5, x + 13, y + 2, cx - 2, y + 12, "fname");
                     mo12_aud(x + 4, y + 14, lv_w35(sz), 5, x + 2, y + 13, dx - 2, y + 20, "fsize");
                     mo12_aud(x + 4, y + 23, lv_w35(MO_FOLD_ACT[i][j]), 5, x + 2, y + 22, x + 142, y + 29, "fact");
                     mo12_aud(x + 94, y + 32, lv_w35(t), 5, x + 90, y + 30, x + 120, y + 38, "fst"); }
}
static int mo12_drop0(void)   /* v5d: the folders drop only after the search is done: lock 1 s, search bar types the device, the 11-band sweep, +100 ms */
{
    const int L = itw_mo_sel >= 0 && itw_mo_sel < MO_N ? (int)strlen(MO_DEV[itw_mo_sel].name) : 6;
    return MO12_BAR0 + L * MO12_TYPE_MS + 11 * MO12_SWEEP_FR * 100 / 3 + 100;
}
