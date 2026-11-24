/* ---------- HACKbot game (MOCK 2026-11-12 re-mock; was SCANbot "DOWNLOAD"). Mobandy 2026-11-12: "its hard to grasp whats going on.
 * lets try where at the beginning it prompts in the terminal what to look for and you have to go to each tower individually and
 * download the right file." ----------
 * A NEW screen (LV_SC_DL = 12), behind LV_DL_GAME (needs LV_HC_GAME + LV_RD_INFO; 0 = none). No existing button routes to it.
 *  INTRO  - a full-screen terminal types the mission: the ONE file to find (name + size), how many towers, the trace rule. Tap = start.
 *  TOWERS - the GIBSON radar scene 1:1 (lv_scene_radar via the Hot/Cold world swap lv_hc_swap: sky, skyline, 3D grid, rain, neon
 *           towers, lock brackets, D4 current) with 5 FAKE towers T1..T5. Tap a tower: it locks, the radar camera (lv_rd_camx)
 *           eases 400 ms until that tower stands at the left, then its 3 folders drop in (INFO Joey folder frames, ~0.6 s).
 *           Each folder holds one file (name + size). Tap a file = DOWNLOAD (1.2 s bar). Right file (name AND size match the
 *           mission) = DOWNLOAD COMPLETE -> win card. Wrong file = ACCESS DENIED flash + reason, trace unchanged. "< TOWERS" closes the tower
 *           (camera eases back); checked towers are dimmed with a green tick.
 *  TRACE  - TIMER ONLY (Mobandy 2026-11-12): 90 s to 100% on every level. Wrong files do not move it.
 *           100% = TRACE COMPLETE -> TRACED card -> RETRY (same level, files reshuffled, its intro again).
 *  SIZES  - LOCKED: one more file a tower every 2 levels (L1-2 3, L3-4 4, L5-6 5, L7+ 6 = cap); the folders shrink to share the same
 *           drop area (no scrolling). 7 a tower is drawable (sim feasibility only) but is not in the progression.
 *  LEVELS - Mobandy 2026-11-12 "increase in levels to where you have to find more files to download as the levels get higher".
 *           DEFAULTS he can change: level L needs min(L, 5) target files, each in a different tower; the intro lists them all;
 *           the bottom terminal shows FIND got/n + every target with a green tick when downloaded; all of them = LEVEL L CLEAR
 *           -> NEXT LEVEL (that level's intro) or BACK.
 *  SLOTS  - mock 2026-11-20: the HACKbot intro itself is the 3-slot picker (Mobandy). U/D moves between slots, A opens,
 *           B goes back. Slot 1..3 keep full progress (level, score, towers hacked, files downloaded). Empty = NEW GAME.
 *           Saves go to SD /BOTcorps/hackbot/slotN.sav (small 16-byte HB1 record). NVS fallback only if there is no SD.
 *  PAUSE  - during play, B on the field (no tower open) opens a pause menu; RESUME / SAVE / EXIT (U/D, A selects).
 *           SAVE writes the slot in use. EXIT returns home without saving. B / RESUME returns to play.
 * The mission line ("FIND <file> <size>") is the bottom terminal's prompt line on every play frame. No bot is drawn in the scene.
 * While this screen is up it borrows the Hot/Cold game's world buffer lv_hc_w. All files / towers are FAKE game data. */
#if LV_DL_GAME
#if !LV_HC_GAME || !LV_RD_INFO
#error "LV_DL_GAME needs LV_HC_GAME and LV_RD_INFO"
#endif
#define LV_DL_NT 5
#define LV_DL_NF 7             /* most folders the layout can draw (7 = feasibility only); level L has lv_dl_nf_for(L) */
#define LV_DL_CAM_MS 400      /* camera ease to / from a tower */
#define LV_DL_DROP0_MS 300    /* folders start dropping this long after the tap (camera mostly there) */
#define LV_DL_DROP_GAP 110    /* stagger between folders */
#define LV_DL_DROP_MS 380     /* one folder's fall: all 3 are down 300+220+380 = 900 ms after the tap */
#define LV_DL_DL_MS 1200      /* download bar */
#define LV_DL_DENY_MS 900     /* ACCESS DENIED flash */
enum { LV_DL_INTRO, LV_DL_PLAY, LV_DL_DONE, LV_DL_TRACED, LV_DL_WIN, LV_DL_OVER, LV_DL_SLOTS, LV_DL_PAUSE, LV_DL_SAVEOK };  /* SLOTS/PAUSE/SAVEOK appended so existing phase numbers stay */
enum { LV_DL_S_IDLE, LV_DL_S_DL, LV_DL_S_GOT, LV_DL_S_DENY };
#define LV_DL_MAXT 5          /* most target files in one level (DEFAULT) */
struct LvDlFile { uint8_t name, st; uint8_t mb10; int8_t tgt; bool la; };   /* size = mb10 / 10 MB; tgt = target index or -1; la = look-alike */
static LvDlFile lv_dl_fo[LV_DL_NT][LV_DL_NF];
static const char *const LV_DL_NAMES[] = {"GARBAGE.DAT", "GARBAGE.BAK", "GARBAGE.TXT", "PAYROLL.DB", "EMAIL.LOG", "KERNEL.SYS", "PLAGUE.EXE",
                                          "TANKER.NAV", "BALLAST.CFG", "MEMO.TXT", "LEDGER.DAT", "CRASH.DMP", "BACKUP.ZIP", "SYSLOG.OLD",
                                          "DAVINCI.KEY", "ELLINGSON.HR", "GIBSON.LOG", "WORM.SRC"};   /* 14.. = target names only */
#define LV_DL_NNAMES 18
#define LV_DL_NPLAIN 14   /* other files are drawn from 3..13 */
static const uint8_t LV_DL_TNAME[LV_DL_MAXT] = {0, 14, 15, 16, 17};   /* GARBAGE.DAT, DAVINCI.KEY, ELLINGSON.HR, GIBSON.LOG, WORM.SRC */
static int lv_dl_round = 1, lv_dl_phase = LV_DL_INTRO, lv_dl_tw = -1, lv_dl_cam_tw = -1, lv_dl_cam_mode = 0;   /* mode 0 drift, 1 to tower, 2 back */
static int lv_dl_need = 1, lv_dl_got = 0, lv_dl_ttw[LV_DL_MAXT], lv_dl_tf[LV_DL_MAXT], lv_dl_tmb[LV_DL_MAXT], lv_dl_tnm[LV_DL_MAXT]; static bool lv_dl_tgot[LV_DL_MAXT];
static int lv_dl_need_for(int lv) { return lv < 1 ? 1 : lv < LV_DL_MAXT ? lv : LV_DL_MAXT; }   /* DEFAULT: L1 = 1 file, +1 a level, max 5 */
static int lv_dl_pf = -1, lv_dl_wrong = 0, lv_dl_dls = 0, lv_dl_trace = 0, lv_dl_score = 0, lv_dl_best = -1;
static uint32_t lv_dl_t0 = 0, lv_dl_tend = 0, lv_dl_pt = 0, lv_dl_open_t = 0, lv_dl_cam_t = 0, lv_dl_deny_t = 0, lv_dl_intro_t = 0;
static float lv_dl_cam_from = 0, lv_dl_cam_at = 0, lv_dl_pull_p0 = 0;
static char lv_dl_why[40], lv_dl_note[40]; static bool lv_dl_found = false; static uint32_t lv_dl_found_t = 0; static int lv_dl_try = 0;   /* note: a right file that is not the last one */   /* RETRY count: a retry reshuffles the files (same towers) */
struct LvDlTag { int16_t x0, y0, x1, y1, i; }; static LvDlTag lv_dl_tags[LV_DL_NT]; static int lv_dl_ntag = 0, lv_dl_tag_tw = -1;   /* tower tags drawn last frame (sim gate) */   /* why the last download was wrong (shown in the panel until the next download / tower) */
static bool lv_dl_seen[LV_DL_NT], lv_dl_deny = false, lv_dl_typed_all = false, lv_dl_intro_done = false;   /* done = the last drawn intro frame showed every line */
static const LvRect LV_DL_BTN_T = {170, 247, 239, 285};   /* "< TOWERS" finger target (drawn 64x34 at 173,249), 70x39 */
/* Mobandy 2026-11-12 "They get smaller every time to fit within the same area": n folders always share the 3-folder drop area
 * (tab tops from y 42, last body ends at y 228). n = 3: pitch 66, body 46 (as before). n > 3: pitch (186 + 4) / n, 4 px gap, 8 px tab. No scrolling. */
static int lv_dl_nf = 3;
#define LV_DL_NF_CAP 6         /* LOCKED default (Mobandy 2026-11-12 "one extra every couple levels", "max out at six or seven"): cap 6 */
static int lv_dl_nf_force = 0;   /* SIM feasibility only (sim "dlnf N"): 0 = the progression */
static int lv_dl_nf_for(int lv) { const int n = 3 + ((lv < 1 ? 1 : lv) - 1) / 2; return n > LV_DL_NF_CAP ? LV_DL_NF_CAP : n; }   /* L1-2 3, L3-4 4, L5-6 5, L7+ 6 */
static const int LV_DL_FX = 92, LV_DL_FW = 143, LV_DL_FY0 = 42;
static int lv_dl_fdy(void) { return lv_dl_nf <= 3 ? 66 : (186 + 4) / lv_dl_nf; }        /* row pitch: 66 / 47 / 38 / 31 / 27 for 3..7 */
static int lv_dl_fbody(void) { return lv_dl_nf <= 3 ? 46 : lv_dl_fdy() - 4 - 8; }     /* body height: 46 / 35 / 26 / 19 / 15 */
#ifndef LV_DL_SELFDESTRUCT
#define LV_DL_SELFDESTRUCT 1   /* mock/v1220 item 6 (Mobandy 3:49 PM ET 11-24 "a count down once in the tower to self destruct and then you lose if the folder is in the tower"); 0 = v1.2.19 */
#endif
#if LV_DL_SELFDESTRUCT
#if LV_DL_SPEED
#define LV_DL_SD_SPD lv_dl_speed
#else
#define LV_DL_SD_SPD 1
#endif
#define LV_DL_SD_MS 12000      /* tower self-destruct fuse in game ms: 12 s at 1X, 6 s at 2X, 4 s at 3X (the SPEED clock) */
#define LV_DL_SD_URGENT 3000   /* real ms left: the readout flashes (banner flash law) */
static int32_t lv_dl_sd = 0; static uint32_t lv_dl_sd_last = 0;   /* game ms left on the open tower's fuse */
static bool lv_dl_blown = false; static int lv_dl_blown_tw = -1;   /* lost: the tower blew with the target still inside */
static uint8_t lv_dl_sdk = 0; static uint32_t lv_dl_sdk_t = 0; static int lv_dl_sdk_tw = -1; static int32_t lv_dl_sdk_left = 0;   /* 1 close call (out with <3 s), 2 empty tower blew */
static int lv_dl_sd_real(int32_t g) { return g <= 0 ? 0 : (int)((g + LV_DL_SD_SPD - 1) / LV_DL_SD_SPD); }   /* game ms -> real ms at this SPEED */
#endif
static int lv_dl_gap(void) { return lv_dl_nf <= 3 ? LV_DL_DROP_GAP : 2 * LV_DL_DROP_GAP / (lv_dl_nf - 1); }   /* same total stagger (220 ms) */
#ifndef LV_DL_DECK
#define LV_DL_DECK 1   /* v1.2.14: 1 = the d-pad game deck (real firmware); 0 = the old panel + terminal (kept) */
#endif
static int lv_dl_fsel = 0;   /* v1.2.14: the deck's folder cursor (was the MOCK's lv_mock_dpad_sel) */
/* ---- 3 save slots (mock 2026-11-20). File format: 16-byte little-endian record "HB1\0" + ver,level,score,towers,got,need,pad.
 * Path: /BOTcorps/hackbot/slot1.sav .. slot3.sav on the CYD SD card. If the card is missing or the write fails, the same
 * 16 bytes go in NVS namespace "botcorps", keys hbsl1..hbsl3. The host sim never touches SD or NVS: it shows canned
 * EXAMPLE DATA (slot 1 filled, slots 2 and 3 NEW GAME) and stamps EX.DATA on the slot picker. */
#define LV_DL_NSLOT 3
#define LV_DL_SAV_DIR "/BOTcorps/hackbot"
#define LV_DL_SAV_NAME "/BOTcorps/hackbot/slot%d.sav"
struct LvDlSave { bool used; uint8_t level, towers, files_got, files_need; int16_t score; };
static LvDlSave lv_dl_slot[LV_DL_NSLOT];
static int lv_dl_slot_i = 0;     /* 0..2: selected slot on the intro, and the slot in use once play starts */
static int lv_dl_menu = 0;       /* pause items: 0 RESUME, 1 SAVE, 2 EXIT */
#define LV_DL_PMENU 3
static bool lv_dl_sd_ok = true;  /* sim: SD present (example). Firmware: mount result. */
static bool lv_dl_exdata = true; /* mock stamp: numbers are example data, not live saves */
static void lv_dl_slot_path(int i, char *o, int n) { snprintf(o, n, LV_DL_SAV_NAME, i + 1); }
static void lv_dl_slots_example(void)   /* host-sim canned slots, clearly example data */
{
    memset(lv_dl_slot, 0, sizeof lv_dl_slot);
    lv_dl_sd_ok = true; lv_dl_exdata = true;
    lv_dl_slot[0].used = true; lv_dl_slot[0].level = 3; lv_dl_slot[0].score = 860;
    lv_dl_slot[0].towers = 3; lv_dl_slot[0].files_got = 2; lv_dl_slot[0].files_need = 3;
}
static void lv_dl_pack_save(const LvDlSave &s, uint8_t b[16])
{
    memset(b, 0, 16); b[0] = 'H'; b[1] = 'B'; b[2] = '1'; b[3] = 0; b[4] = 1;
    b[5] = s.used ? 1 : 0; b[6] = s.level; b[7] = (uint8_t)(s.score & 0xff); b[8] = (uint8_t)((s.score >> 8) & 0xff);
    b[9] = s.towers; b[10] = s.files_got; b[11] = s.files_need;
}
static bool lv_dl_unpack_save(const uint8_t b[16], LvDlSave &s)
{
    if (b[0] != 'H' || b[1] != 'B' || b[2] != '1' || b[3] != 0 || b[4] != 1) return false;
    memset(&s, 0, sizeof s); s.used = b[5] != 0; s.level = b[6]; s.score = (int16_t)(b[7] | (b[8] << 8));
    s.towers = b[9]; s.files_got = b[10]; s.files_need = b[11]; return true;
}
/* v1.2.17 firmware save I/O (Mobandy "build v1.2.17 ... HACKbot saves", 2026-11-20). The firmware glue (lv_ui.cpp) sets
 * these two hooks; the host sim leaves them null and keeps the canned EXAMPLE DATA (EX.DATA stamp).
 *   load: fills raw[i] / have[i] for slots 1..3. Returns 1 = read from SD /BOTcorps/hackbot/slotN.sav, 0 = no SD (NVS
 *         "botcorps" hbsl1..hbsl3), -1 = neither.
 *   save: SD first (mkdir /BOTcorps, /BOTcorps/hackbot, write 16 bytes, read back). If the card is absent or the write
 *         fails: NVS hbslN, same 16 bytes. Returns 1 = SD, 0 = NVS, -1 = both failed. */
static int (*lv_dl_io_load_hook)(uint8_t raw[LV_DL_NSLOT][16], bool have[LV_DL_NSLOT]) = nullptr;
static int (*lv_dl_io_save_hook)(int slot, const uint8_t raw[16]) = nullptr;
static int lv_dl_io_last = 1;    /* last save result (1 SD, 0 NVS, -1 failed); serial / sim dlstate only */
static void lv_dl_slots_load(void)
{
    if (!lv_dl_io_load_hook) { lv_dl_slots_example(); return; }   /* host sim: example data, stamped EX.DATA */
    memset(lv_dl_slot, 0, sizeof lv_dl_slot); lv_dl_exdata = false;   /* firmware: real saves, no EX.DATA stamp */
    uint8_t raw[LV_DL_NSLOT][16]; bool have[LV_DL_NSLOT] = {};
    memset(raw, 0, sizeof raw);
    const int src = lv_dl_io_load_hook(raw, have);
    lv_dl_sd_ok = src == 1;
    for (int i = 0; i < LV_DL_NSLOT; ++i) {
        LvDlSave s{};
        if (have[i] && lv_dl_unpack_save(raw[i], s) && s.used) { if (s.level < 1) s.level = 1; lv_dl_slot[i] = s; }
    }
}
static void lv_dl_slot_write(int i)
{
    if (!lv_dl_io_save_hook || i < 0 || i >= LV_DL_NSLOT) return;   /* host sim: RAM only */
    uint8_t raw[16]; lv_dl_pack_save(lv_dl_slot[i], raw);
    lv_dl_io_last = lv_dl_io_save_hook(i, raw);
    lv_dl_sd_ok = lv_dl_io_last == 1;
}
static void lv_dl_capture_slot(int i)
{
    if (i < 0 || i >= LV_DL_NSLOT) return;
    int ns = 0; for (int t = 0; t < LV_DL_NT; ++t) ns += lv_dl_seen[t];
    lv_dl_slot[i].used = true; lv_dl_slot[i].level = (uint8_t)(lv_dl_round < 1 ? 1 : lv_dl_round);
    lv_dl_slot[i].score = (int16_t)(lv_dl_score < 0 ? 0 : lv_dl_score > 9999 ? 9999 : lv_dl_score);
    lv_dl_slot[i].towers = (uint8_t)ns; lv_dl_slot[i].files_got = (uint8_t)lv_dl_got;
    lv_dl_slot[i].files_need = (uint8_t)lv_dl_need;
}
static LvRect lv_dl_slot_rect(int i) { return {(int16_t)22, (int16_t)(100 + i * 44), (int16_t)217, (int16_t)(100 + i * 44 + 41)}; }   /* 196 x 42, >=40 px tall */
static LvRect lv_dl_pause_rect(int i) { return {(int16_t)22, (int16_t)(128 + i * 36), (int16_t)217, (int16_t)(128 + i * 36 + 35)}; }   /* 196 x 36: RESUME / SAVE / EXIT, last row ends y 235, above the deck */
static LvRect lv_dl_ftap(int i)   /* 3: 152 x 59 (as before); n > 3: one full pitch each, edge to edge: 152 x 47 / 38 / 31 / 27 */
{
    const int y = LV_DL_FY0 + i * lv_dl_fdy();
    if (lv_dl_nf <= 3) return {(int16_t)(LV_DL_FX - 4), (int16_t)(y - 2), 239, (int16_t)(y + 8 + 46 + 2)};
    return {(int16_t)(LV_DL_FX - 4), (int16_t)(y - 2), 239, (int16_t)(y + lv_dl_fdy() - 3)};
}
static int lv_dl_tms(void) { return 90000; }   /* DEFAULT: 90 s on every level, timer only */
static int lv_dl_secs10(void) { return (int)(((lv_dl_phase == LV_DL_PLAY ? lv_now : lv_dl_tend) - lv_dl_t0) / 100); }
static void lv_dl_trace_upd(void)
{
    if (lv_dl_phase != LV_DL_PLAY) return;
    const int t = (int)((int64_t)(lv_now - lv_dl_t0) * 1000 / lv_dl_tms()); lv_dl_trace = t > 1000 ? 1000 : t;
}
static const char *lv_dl_name(const LvDlFile &f) { return LV_DL_NAMES[f.name % LV_DL_NNAMES]; }
static void lv_dl_size(int mb10, char *o, int n) { snprintf(o, n, "%d.%dMB", mb10 / 10, mb10 % 10); }
static void lv_dl_new_round(void)
{
    LvHcWorld &w = lv_hc_w; memset(&w, 0, sizeof w);
    int n = 0;   /* 5 towers in screen slots 40 px apart (x -80..80 from centre): 40 px finger zones, labels never overlap */
    for (int slot = 0; slot < LV_DL_NT; ++slot) for (int k = 0; k < 60000; ++k) {
        const uint8_t mac[6] = {0xD1, 0x0A, (uint8_t)lv_dl_round, (uint8_t)(k >> 8), (uint8_t)k, (uint8_t)(slot + 1)};
        const uint32_t h = lv_rd_hash(mac);
        const float Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f, X = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * Z, x = 120 * X / Z;
        if (Z < 2.0f || Z > 4.2f || fabsf(x - (-80 + 40 * slot)) > 3) continue;
        LvFind &f = w.f[n]; snprintf(f.name, sizeof f.name, "TOWER %d", n + 1); memcpy(f.mac, mac, 6);
        f.rssi = (int8_t)(-70 + (int)((h >> 16) % 26)); f.kind = LV_K_WIFI; f.mfg = 0xFFFF; ++n; break;
    }
    w.nf = n; w.sel = -1; w.t0 = lv_now; w.miss = LV_RD_MISS_GRACE; w.tgt.rssi = -99; w.tgt_have = true;
    /* the files: ONE target (GARBAGE.DAT + this round's size); 2 same-name wrong-size fakes, 2 look-alike names with the right
     * size, the rest other files. Never two identical name+size pairs. The target tower is never T1 or the first tower a player
     * would try twice: it is picked per round, and every tower has 3 files. */
    const uint32_t s = lv_rd_ch((uint32_t)lv_dl_round, 0x4acbu ^ (uint32_t)lv_dl_try * 0x9e37u);
    lv_dl_need = lv_dl_need_for(lv_dl_round); lv_dl_got = 0; lv_dl_nf = lv_dl_nf_force ? lv_dl_nf_force : lv_dl_nf_for(lv_dl_round);
    int ord[LV_DL_NT]; ord[0] = 1 + (int)((s >> 8) % (LV_DL_NT - 1));   /* target towers: the first as before, the rest shuffled = all different */
    for (int i = 1, t = 0; i < LV_DL_NT; ++t) if (t != ord[0]) ord[i++] = t;
    for (int i = LV_DL_NT - 1; i > 1; --i) { const int j = 1 + (int)(lv_rd_ch(s, 400u + (uint32_t)i) % (uint32_t)i); const int q = ord[i]; ord[i] = ord[j]; ord[j] = q; }
    for (int k = 0; k < lv_dl_need; ++k) {
        lv_dl_ttw[k] = ord[k]; lv_dl_tf[k] = k ? (int)(lv_rd_ch(s, 800u + (uint32_t)k) % (uint32_t)lv_dl_nf) : (int)((s >> 12) % (uint32_t)lv_dl_nf);
        lv_dl_tmb[k] = 12 + (int)((k ? lv_rd_ch(s, 700u + (uint32_t)k) : s) % 70); lv_dl_tnm[k] = LV_DL_TNAME[(k + lv_dl_round - 1) % LV_DL_MAXT]; lv_dl_tgot[k] = false;
    }
    int nm = 3 + (int)((s >> 16) % (LV_DL_NPLAIN - 3));
    for (int t = 0; t < LV_DL_NT; ++t) for (int k = 0; k < lv_dl_nf; ++k) {
        LvDlFile &f = lv_dl_fo[t][k]; f.st = LV_DL_S_IDLE; f.tgt = -1; f.la = false;
        f.name = (uint8_t)(3 + (nm++ - 3) % (LV_DL_NPLAIN - 3)); f.mb10 = (uint8_t)(5 + lv_rd_ch(s, (uint32_t)(t * 8 + k)) % 90);
    }
    for (int k = 0; k < lv_dl_need; ++k) { LvDlFile &T = lv_dl_fo[lv_dl_ttw[k]][lv_dl_tf[k]]; T.name = (uint8_t)lv_dl_tnm[k]; T.mb10 = (uint8_t)lv_dl_tmb[k]; T.tgt = (int8_t)k; }
    if (lv_dl_need == 1) {   /* 1 file (as before): the look-alikes go in other towers (one per tower where possible), never in the target's slot */
    const int lv_dl_tgt_tw = lv_dl_ttw[0], lv_dl_tgt_mb10 = lv_dl_tmb[0];
    const int kinds[4][2] = {{0, -1}, {0, +1}, {1, 0}, {2, 0}};   /* name index, size delta sign (0 = same size) */
    for (int q = 0; q < 4; ++q) {
        const int t = (lv_dl_tgt_tw + 1 + q) % LV_DL_NT; const int k = (int)(lv_rd_ch(s, 500u + (uint32_t)q) % (uint32_t)lv_dl_nf);
        LvDlFile &f = lv_dl_fo[t][k]; if (f.tgt >= 0) continue;
        f.name = (uint8_t)kinds[q][0]; f.la = true;
        int mb = lv_dl_tgt_mb10; if (kinds[q][1]) { mb += kinds[q][1] * (6 + (int)(lv_rd_ch(s, 600u + (uint32_t)q) % 20)); if (mb < 3) mb = lv_dl_tgt_mb10 + 13; }
        f.mb10 = (uint8_t)mb;
    }
    } else for (int k = 0; k < lv_dl_need; ++k) {   /* 2+ files: per target, one same-name WRONG-SIZE fake in another tower */
        for (int j = 0; j < 64; ++j) {
            const uint32_t r = lv_rd_ch(s, 900u + (uint32_t)(k * 64 + j)); const int t = (int)(r % LV_DL_NT), q = (int)((r >> 8) % (uint32_t)lv_dl_nf);
            LvDlFile &f = lv_dl_fo[t][q]; if (f.tgt >= 0 || f.la || t == lv_dl_ttw[k]) continue;
            int mb = lv_dl_tmb[k] + ((k + (int)(s >> 20)) & 1 ? 1 : -1) * (6 + (int)(lv_rd_ch(s, 600u + (uint32_t)k) % 20)); if (mb < 3) mb = lv_dl_tmb[k] + 13;
            f.name = (uint8_t)lv_dl_tnm[k]; f.mb10 = (uint8_t)mb; f.la = true; break;
        }
    }
    lv_dl_tw = -1; lv_dl_cam_tw = -1; lv_dl_cam_mode = 0; lv_dl_pf = -1; lv_dl_wrong = lv_dl_dls = lv_dl_trace = 0; lv_dl_deny = lv_dl_found = false; lv_dl_why[0] = lv_dl_note[0] = 0;
#if LV_DL_SELFDESTRUCT
    lv_dl_sd = 0; lv_dl_blown = false; lv_dl_blown_tw = -1; lv_dl_sdk = 0;
#endif
    for (int t = 0; t < LV_DL_NT; ++t) lv_dl_seen[t] = false;
    lv_dl_t0 = lv_dl_tend = lv_now;
}
static void lv_dl_lock(int i, bool on)
{
    LvHcWorld &w = lv_hc_w;
    w.lock = on; w.lock_t = lv_now; if (on) memcpy(w.lockmac, w.f[i].mac, 6); w.sel = -1; w.holding = false; w.hunting = false; w.miss = LV_RD_MISS_GRACE;
}
static void lv_dl_play(void)
{
    lv_dl_new_round(); lv_dl_phase = LV_DL_PLAY; lv_dl_t0 = lv_now;
    lv_hc_log("LINK UP. %d TOWERS. TRACE RUNNING.", LV_DL_NT); lv_hc_log("TAP A TOWER TO OPEN IT");
}
static LvSelClk lv_dl_clk;   /* v1.2.11: lv_select.h (LOCKED shared selection) clock for the open-tower pull-up + drop */
static void lv_dl_enter(void) { lv_sel_clk_reset(lv_dl_clk, lv_now); lv_dl_new_round(); lv_dl_phase = LV_DL_INTRO; lv_dl_intro_t = lv_now; lv_dl_typed_all = lv_dl_intro_done = false; }   /* NEXT LEVEL / RETRY / sim dllevel: the locked level typist, not the slot intro */
static void lv_dl_leave(void) { lv_dl_phase = LV_DL_INTRO; lv_dl_tw = -1; lv_dl_cam_mode = 0; lv_rd_cam_hold = false; }   /* v1.2.10: leaving with a tower open left lv_rd_cam_hold set -> GIBSON lost its camera sway */
static void lv_dl_boot(void)   /* first entry from HOME / pager: the slot-picker intro */
{
    lv_sel_clk_reset(lv_dl_clk, lv_now); lv_dl_new_round();   /* rain + towers sit behind the card; the 90s trace is not running */
    lv_dl_slots_load(); lv_dl_phase = LV_DL_SLOTS; lv_dl_slot_i = 0; lv_dl_menu = 0; lv_dl_tw = -1;
}
static void lv_dl_apply_slot(int i)
{
    if (i < 0 || i >= LV_DL_NSLOT) return;
    lv_dl_slot_i = i;
    const LvDlSave s = lv_dl_slot[i];
    if (!s.used) { lv_dl_round = 1; lv_dl_try = 0; lv_dl_score = 0; lv_dl_enter(); return; }   /* NEW GAME -> locked level-1 typist */
    lv_dl_round = s.level < 1 ? 1 : s.level; lv_dl_try = 0;
    lv_sel_clk_reset(lv_dl_clk, lv_now);
    lv_dl_play();   /* one new_round at this level, then PLAY with the saved progress applied below */
    lv_dl_score = s.score; lv_dl_best = s.score;
    int want = s.towers > LV_DL_NT ? LV_DL_NT : s.towers;
    for (int t = 0; t < LV_DL_NT; ++t) lv_dl_seen[t] = t < want;
    int g = s.files_got; if (g > lv_dl_need) g = lv_dl_need; lv_dl_got = g;
    for (int k = 0; k < lv_dl_need; ++k) {
        lv_dl_tgot[k] = k < g;
        if (k < g) { LvDlFile &f = lv_dl_fo[lv_dl_ttw[k]][lv_dl_tf[k]]; f.st = LV_DL_S_GOT; }
    }
}
static void lv_dl_open_pause(void)
{
    if (lv_dl_phase != LV_DL_PLAY || lv_dl_tw >= 0) return;   /* B on an open tower still closes it (locked) */
    lv_dl_tend = lv_now; lv_dl_phase = LV_DL_PAUSE; lv_dl_menu = 0;   /* freeze the 90s timer at tend; rain keeps moving */
}
static void lv_dl_resume_play(void)
{
    if (lv_dl_phase != LV_DL_PAUSE && lv_dl_phase != LV_DL_SAVEOK) return;
    const uint32_t dt = (uint32_t)(lv_now - lv_dl_tend);
    lv_dl_t0 += dt; lv_dl_pt += dt; lv_dl_deny_t += dt; lv_dl_found_t += dt;   /* freeze was tend; shift the 90s / banner clocks. lv_sel_clk kept stepping, leave it. */
    lv_dl_phase = LV_DL_PLAY;
}
static void lv_dl_do_save(void)
{
    lv_dl_capture_slot(lv_dl_slot_i);
    lv_dl_slot_write(lv_dl_slot_i);   /* v1.2.17: SD slotN.sav, NVS hbslN if no SD (firmware only) */
    lv_dl_phase = LV_DL_SAVEOK;
}
static void lv_dl_open_tower(int i)
{
    const uint32_t vt = lv_sel_clk_now(lv_dl_clk, lv_now);
    lv_dl_why[0] = lv_dl_note[0] = 0; lv_dl_tw = i; lv_dl_open_t = vt; lv_dl_seen[i] = true; lv_dl_lock(i, true);
    lv_dl_cam_mode = 1; lv_dl_cam_tw = i; lv_dl_cam_t = vt; lv_dl_cam_from = lv_dl_cam_at; lv_dl_fsel = 0;
    lv_hc_log("TOWER %d OPEN. %d FILES.", i + 1, lv_dl_nf);
#if LV_DL_SELFDESTRUCT
    lv_dl_sd = LV_DL_SD_MS; lv_dl_sd_last = lv_now; lv_dl_sdk = 0; lv_hc_log("SELF-DESTRUCT ARMED: %dS", lv_dl_sd_real(LV_DL_SD_MS) / 1000);
#endif
}
static void lv_dl_close_tower(void)
{
    if (lv_dl_tw < 0) return;
    lv_hc_log("TOWER %d CHECKED", lv_dl_tw + 1);
#if LV_DL_SELFDESTRUCT
    if (lv_dl_phase == LV_DL_PLAY && lv_dl_sd > 0 && lv_dl_sd_real(lv_dl_sd) <= LV_DL_SD_URGENT) { lv_dl_sdk = 1; lv_dl_sdk_t = lv_now; lv_dl_sdk_tw = lv_dl_tw; lv_dl_sdk_left = lv_dl_sd_real(lv_dl_sd); }
    lv_dl_sd = 0;
#endif
    lv_dl_cam_mode = 2; lv_dl_cam_t = lv_sel_clk_now(lv_dl_clk, lv_now); lv_dl_tw = -1; lv_dl_lock(0, false);
    lv_dl_cam_from = lv_dl_cam_at; lv_dl_pull_p0 = lv_rd_pull_scr == LV_SC_DL ? lv_rd_pull_p : 0.0f;   /* v1.2.13: fly back out from here */
}
static void lv_dl_resolve(void)
{
    LvDlFile &f = lv_dl_fo[lv_dl_tw][lv_dl_pf]; char sz[12]; lv_dl_size(f.mb10, sz, sizeof sz);
    if (f.tgt >= 0) {
        f.st = LV_DL_S_GOT; lv_dl_tgot[f.tgt] = true; ++lv_dl_got;
        if (lv_dl_got >= lv_dl_need) {   /* the last one on the list: level clear */
            lv_dl_trace_upd(); lv_dl_phase = LV_DL_DONE; lv_dl_tend = lv_dl_pt = lv_now;
            lv_dl_score = 1000 - lv_dl_trace / 10 * 10; if (lv_dl_score > lv_dl_best) lv_dl_best = lv_dl_score;
            lv_hc_log("%s %s  DOWNLOAD COMPLETE", lv_dl_name(f), sz);
        } else {   /* one of several: FILE FOUND, keep going */
            lv_dl_found = true; lv_dl_found_t = lv_now; snprintf(lv_dl_note, sizeof lv_dl_note, "GOT %s %s", lv_dl_name(f), sz);
            lv_hc_log("%s %s  GOT %d OF %d", lv_dl_name(f), sz, lv_dl_got, lv_dl_need);
        }
    } else {
        int k = -1; for (int j = 0; j < lv_dl_need; ++j) if (lv_dl_tnm[j] == f.name) k = j;
        if (k < 0) snprintf(lv_dl_why, sizeof lv_dl_why, "WRONG NAME: %s", lv_dl_name(f));
        else { char need[12]; lv_dl_size(lv_dl_tmb[k], need, sizeof need); snprintf(lv_dl_why, sizeof lv_dl_why, "WRONG SIZE: %s, NEED %s", sz, need); }
        f.st = LV_DL_S_DENY; ++lv_dl_wrong; lv_dl_deny = true; lv_dl_deny_t = lv_now;
        lv_hc_log("%s %s  ACCESS DENIED", lv_dl_name(f), sz);
    }
    lv_dl_pf = -1;
}
static inline float lv_dl_ease(float u) { return lv_sel_ease(u); }   /* v1.2.11: the shared lv_select.h ease (same formula) */
static float lv_dl_cam_for(int i)   /* camera x that puts tower i's front face centre at screen x 50; called INSIDE lv_hc_swap (lv_f = game world) */
{
    const uint32_t h = lv_rd_hash(lv_f[i].mac);
    const float Z = 1.6f + (((h >> 8) & 0xff) / 255.0f) * 3.2f, X = (((h & 0xff) / 255.0f) * 2.0f - 1.0f) * 0.72f * Z;
    return X - (50 - 120) * Z / LV_RD_F;
}
static void lv_dl_tick(void)
{
#if LV_DL_SPEED
    { uint32_t dt = lv_now - lv_dl_slast; lv_dl_slast = lv_now; if (dt > 250) dt = 250; lv_dl_soff += dt * (uint32_t)(lv_dl_speed - 1); }   /* GIBSON's soff law */
#endif
    { const int oel = (int)(lv_dl_clk.vt - lv_dl_open_t), cel = (int)(lv_dl_clk.vt - lv_dl_cam_t);   /* lv_select.h clock: cap only while the pull-up / drop plays */
      lv_sel_clk_step(lv_dl_clk, lv_now, (lv_dl_cam_mode && cel < LV_DL_CAM_MS) ? 50u : (lv_dl_tw >= 0 && oel < LV_DL_DROP0_MS + (lv_dl_nf - 1) * lv_dl_gap() + LV_DL_DROP_MS) ? 100u : LV_SEL_NOCAP); }
    lv_dl_trace_upd();
    if (lv_dl_phase == LV_DL_PLAY) {
        if (lv_dl_pf >= 0 && (int32_t)(lv_now - lv_dl_pt) >= LV_DL_DL_MS) { lv_dl_resolve(); lv_dl_trace_upd(); }
        if (lv_dl_deny && (int32_t)(lv_now - lv_dl_deny_t) >= LV_DL_DENY_MS) lv_dl_deny = false;
        if (lv_dl_found && (int32_t)(lv_now - lv_dl_found_t) >= LV_DL_DENY_MS) lv_dl_found = false;
#if LV_DL_SELFDESTRUCT
        { uint32_t dt = lv_now - lv_dl_sd_last; lv_dl_sd_last = lv_now; if (dt > 250) dt = 250;
          if (lv_dl_sdk && (int32_t)(lv_now - lv_dl_sdk_t) >= LV_DL_DENY_MS) lv_dl_sdk = 0;
          if (lv_dl_phase == LV_DL_PLAY && lv_dl_tw >= 0 && lv_dl_sd > 0 && (lv_dl_sd -= (int32_t)(dt * (uint32_t)LV_DL_SD_SPD)) <= 0) {
              lv_dl_sd = 0; bool in = false; for (int q = 0; q < lv_dl_nf; ++q) { const LvDlFile &f = lv_dl_fo[lv_dl_tw][q]; if (f.tgt >= 0 && f.st != LV_DL_S_GOT) in = true; }
              if (in) { lv_dl_blown = true; lv_dl_blown_tw = lv_dl_tw; lv_dl_phase = LV_DL_TRACED; lv_dl_tend = lv_dl_pt = lv_now; lv_dl_pf = -1; lv_hc_log("TOWER %d SELF-DESTRUCT. FILE LOST.", lv_dl_tw + 1); }
              else { const int t = lv_dl_tw; lv_hc_log("TOWER %d BLEW. NOTHING LOST.", t + 1); lv_dl_close_tower(); lv_hc_w.sel = t; memcpy(lv_hc_w.selmac, lv_hc_w.f[t].mac, 6); lv_dl_sdk = 2; lv_dl_sdk_t = lv_now; lv_dl_sdk_tw = t; }
          } }
#endif
        if (lv_dl_phase == LV_DL_PLAY && lv_dl_trace >= 1000) { lv_dl_phase = LV_DL_TRACED; lv_dl_tend = lv_dl_pt = lv_now; lv_dl_pf = -1; lv_hc_log("TRACE COMPLETE. CONNECTION CUT."); }
    }
    if (lv_dl_phase == LV_DL_DONE && (int32_t)(lv_now - lv_dl_pt) >= 1500) lv_dl_phase = LV_DL_WIN;
    if (lv_dl_phase == LV_DL_TRACED && (int32_t)(lv_now - lv_dl_pt) >= 1500) lv_dl_phase = LV_DL_OVER;
    /* the radar's own layout (drift camera), then the game camera: ease to the open tower / back to the drift */
    lv_rd_cam_hold = (lv_dl_cam_mode == 1 || lv_dl_tw >= 0);   /* park next to folders; no sway, no clamp push */
    lv_hc_swap();
    const float cel = (float)(lv_dl_clk.vt - lv_dl_cam_t), u = lv_dl_ease(cel / (float)LV_DL_CAM_MS);
    /* v1.2.13: the locked lv_select.h pull-up. Open: the camera parks (no pan), the opened tower flies in to the close-up;
     * close: it flies back out while the camera eases back to the drift. */
    lv_rd_pull_scr = LV_SC_DL;
    if (lv_dl_cam_tw >= 0 && lv_dl_cam_tw < lv_nf) memcpy(lv_rd_pull_mac, lv_f[lv_dl_cam_tw].mac, 6);
    lv_rd_pull_p = lv_dl_cam_mode == 1 ? u : lv_dl_cam_mode == 2 ? lv_dl_pull_p0 * (1.0f - u) : 0.0f;
    lv_rd_layout();
    const float drift = lv_rd_camx;
    float cx = drift;
    if (lv_dl_cam_mode == 1) cx = lv_dl_cam_from;
    else if (lv_dl_cam_mode == 2) { cx = lv_sel_cam(lv_dl_cam_from, drift, cel, LV_DL_CAM_MS); if (u >= 1) { lv_dl_cam_mode = 0; lv_rd_pull_p = 0; } }
    if (cx != drift) { lv_rd_camx = cx; lv_rd_place(); if (!lv_rd_cam_hold) lv_rd_clamp(); }
    if (lv_dl_cam_mode != 1 && lv_dl_tw < 0) lv_rd_cam_hold = false;
    lv_dl_cam_at = lv_rd_camx;
    lv_hc_nhb = 0;   /* finger boxes: >= 40 px wide, >= 60 px tall, nearest centre wins (lv_hc_hit) */
    for (int n = 0; n < lv_rd_n && lv_hc_nhb < LV_HC_MAXT; ++n) {
        const LvTower &T = lv_rd_tw[n]; LvHcHit &h = lv_hc_hb[lv_hc_nhb++]; int xa, xb, ya, yb; lv_rd_box(T, 3, xa, xb, ya, yb);
        h.i = T.i; h.Z = T.Z; h.cx = (xa + xb) / 2; h.xa = xa - 6; h.xb = xb + 6; h.ya = ya - 6; h.yb = yb + 6;
        if (h.cx - 20 < h.xa) h.xa = h.cx - 20; if (h.cx + 19 > h.xb) h.xb = h.cx + 19; if (h.yb - h.ya < 59) h.ya = h.yb - 59;
        if (h.ya < LV_RD_Y0) h.ya = LV_RD_Y0; if (h.yb > LV_RD_PTOP - 1) h.yb = LV_RD_PTOP - 1; if (h.xa < 0) h.xa = 0; if (h.xb > 239) h.xb = 239;
    }
    lv_hc_swap();
}
/* ---- drawing ---- */
static unsigned lv_dl_tcol(int tr) { return tr < 550 ? 0x30ff90 : tr < 820 ? 0xffb000 : 0xff385c; }
static void lv_dl_chrome(LvBand &B)   /* rows 0..33: the INFO top chrome (stripes, label, 11-bar meter, big number) + header strip */
{
    const int frame = (int)((lv_now - lv_dl_t0) * 3 / 100); char t[48];
    lvi_rect(B, 0, 0, 240, 24, 0x060913);
    for (int y = 0; y < 3; y++) for (int x = 0; x < 240; x++) lvi_px(B, x, y, ((x + y) / 5) & 1 ? 0xeb31ac : 0xdca137);
    for (int y = 21; y < 24; y++) for (int x = 0; x < 240; x++) lvi_px(B, x, y, ((x - y) / 5) & 1 ? 0xdca137 : 0xeb31ac);
    lvi_txt(B, 3, 5, "HACKBOT // GIBSON", 0xb9bdc9);
    const bool ended = lv_dl_phase == LV_DL_DONE || lv_dl_phase == LV_DL_TRACED || lv_dl_phase == LV_DL_WIN || lv_dl_phase == LV_DL_OVER;
    if (lv_dl_tw >= 0) snprintf(t, sizeof t, "TOWER %d", lv_dl_tw + 1); else if (ended) snprintf(t, sizeof t, lv_dl_phase == LV_DL_DONE || lv_dl_phase == LV_DL_WIN ? "GOT IT" : "TRACED"); else snprintf(t, sizeof t, "TOWERS");
    lvi_txt(B, 3, 12, t, lv_dl_phase == LV_DL_DONE || lv_dl_phase == LV_DL_WIN ? 0x30ff90 : 0xf3baff, true);
    const int tr = lv_dl_trace, lit = (tr * 11 + 999) / 1000; const bool jp = false; (void)frame;   /* timer only: a wrong file no longer flashes the meter */
    for (int i = 0; i < 11; i++) { int h = 5 + i; unsigned c = i < 6 ? 0x30ff90 : i < 9 ? 0xffb000 : 0xff385c;
        lvi_rect(B, 84 + i * 6, 20 - h, 5, h, i < lit ? (jp ? 0xff385c : c) : 0x142637); }
    snprintf(t, sizeof t, "%d%%", tr / 10); lvi_txt(B, 191 - (int)strlen(t) * 6, 12, t, jp ? 0xffffff : lv_dl_tcol(tr), true);
    lvi_txt(B, 200, 5, "TRACE", 0xffb000);
    const char *w2 = lv_dl_phase == LV_DL_TRACED || lv_dl_phase == LV_DL_OVER ? "CUT" : ended ? "CLEAR" : tr >= 800 ? "HOT!" : "LIVE";
    lvi_txt(B, 200, 14, w2, tr >= 800 || lv_dl_phase == LV_DL_TRACED || lv_dl_phase == LV_DL_OVER ? 0xff385c : 0x75d7e9);
    lvi_rect(B, 0, 24, 240, 10, 0x0b1826);
    int ns = 0; for (int i = 0; i < LV_DL_NT; ++i) ns += lv_dl_seen[i];
    snprintf(t, sizeof t, "HACKBOT // CHECKED %d/%d TOWERS", ns, LV_DL_NT); lvi_txt(B, 3, 27, t, 0x6fc9df);
    const int s10 = lv_dl_secs10(); snprintf(t, sizeof t, "L%d %d.%dS", lv_dl_round, s10 / 10, s10 % 10); lvi_txt(B, 237 - lv_w35(t), 27, t, 0xe1ad45);
}
#if LV_DL_SELFDESTRUCT
#define LV_DL_SD_TITLE (lv_dl_blown ? "BLOWN UP" : "TRACED")
#else
#define LV_DL_SD_TITLE "TRACED"
#endif
static void lv_dl_card(LvBand &B)   /* win / lose card (the Hot/Cold card frame; no bot face: Mobandy "you dont need a bot floating") */
{
    const uint16_t W = lv_hex(0xffffff), G = lv_hex(0xa0a0a8), TAN = lv_hex(LV_TAN), CY = lv_hex(0x30e8ff);
    b_fill(B, 12, 60, 216, 181, 0x0000); b_frame(B, 12, 60, 216, 181, LV_HC_MAG); b_frame(B, 14, 62, 212, 177, lv_dim(LV_HC_MAG, 3, 8));
    const bool win = lv_dl_phase == LV_DL_WIN, one = lv_dl_need == 1; const int s10 = lv_dl_secs10(); char t[64], sz[12], tt[24]; lv_dl_size(lv_dl_tmb[0], sz, sizeof sz);
    snprintf(tt, sizeof tt, "LEVEL %d CLEAR", lv_dl_round); const char *title = win ? tt : LV_DL_SD_TITLE; const uint16_t tc = lv_hex(win ? 0x30ff60 : 0xff3030); const int tw = (int)strlen(title) * 12;
    b_text(B, 120 - tw / 2 + 1, 79, title, lv_hex(win ? 0x0a4a1a : 0x4a0a0a), 2); b_text(B, 120 - tw / 2, 78, title, tc, 2);
    if (one) snprintf(t, sizeof t, "%s  %s", LV_DL_NAMES[lv_dl_tnm[0]], sz); else if (win) snprintf(t, sizeof t, "ALL %d FILES DOWNLOADED", lv_dl_need); else snprintf(t, sizeof t, "GOT %d OF %d FILES", lv_dl_got, lv_dl_need);
    b_text(B, 120 - (int)strlen(t) * 3, 104, t, lv_hex(0xffc34a));
    if (win) { int n = snprintf(t, sizeof t, one ? "FOUND IN TOWER" : "FOUND IN TOWERS"); for (int i = 0; i < LV_DL_NT; ++i) for (int k = 0; k < lv_dl_need; ++k) if (lv_dl_ttw[k] == i) n += snprintf(t + n, sizeof t - n, " %d", i + 1); lv_hc_ctext(B, 120, t, W); }
    else {
#if LV_DL_SELFDESTRUCT
           if (lv_dl_blown) { snprintf(t, sizeof t, "TOWER %d BLEW WITH THE FILE INSIDE.", lv_dl_blown_tw + 1); lv_hc_ctext(B, 120, t, W); } else
#endif
           lv_hc_ctext(B, 120, "THEY TRACED YOU. CONNECTION CUT.", W);
           if (one) snprintf(t, sizeof t, "IT WAS IN TOWER %d, FILE %d", lv_dl_ttw[0] + 1, lv_dl_tf[0] + 1);
           else { int n = snprintf(t, sizeof t, "MISSED:"); for (int k = 0; k < lv_dl_need; ++k) if (!lv_dl_tgot[k]) n += snprintf(t + n, sizeof t - n, " T%d F%d", lv_dl_ttw[k] + 1, lv_dl_tf[k] + 1); }
           lv_hc_ctext(B, 130, t, TAN); }
    int ns = 0; for (int i = 0; i < LV_DL_NT; ++i) ns += lv_dl_seen[i];
    snprintf(t, sizeof t, "TIME %d.%dS  TOWERS %d/%d  WRONG %d", s10 / 10, s10 % 10, ns, LV_DL_NT, lv_dl_wrong); lv_hc_ctext(B, 142, t, G);
    if (win) { snprintf(t, sizeof t, "TRACE %d%%", lv_dl_trace / 10); lv_hc_ctext(B, 152, t, G);
               b_text35(B, 120 - 34, 170, "SCORE", TAN); snprintf(t, sizeof t, "%d", lv_dl_score); b_text(B, 120 - 10, 166, t, W, 2); }
    else
#if LV_DL_SELFDESTRUCT
         if (lv_dl_blown) lv_hc_ctext(B, 152, "TIP: GET OUT BEFORE IT HITS 0", lv_hex(0xf7ba4a)); else
#endif
         lv_hc_ctext(B, 152, "TIP: NAME AND SIZE MUST BOTH MATCH", lv_hex(0xf7ba4a));
    if (win) snprintf(t, sizeof t, "NEXT: LEVEL %d = FIND %d FILES", lv_dl_round + 1, lv_dl_need_for(lv_dl_round + 1)); else snprintf(t, sizeof t, "RETRY = LEVEL %d AGAIN, NEW SPOTS", lv_dl_round);
    lv_hc_ctext(B, 184, t, CY);
    if (lv_dl_best >= 0) { snprintf(t, sizeof t, "BEST %d", lv_dl_best); lv_hc_ctext(B, 194, t, G); }
    lv_hc_button(B, LV_HC_BTN_L, win ? "NEXT LEVEL" : "RETRY", LV_HC_MAG, true); lv_hc_button(B, LV_HC_BTN_R, "BACK", CY, false);
}
static void lv_dl_banner(LvBand &B, bool win, const char *a, const char *b, const char *sub, uint32_t c, uint32_t t0)
{
    const int f = (int)((lv_now - t0) / 150);
    lvi_rect(B, 10, 104, 220, 58, 0x000000); lvi_box(B, 10, 104, 220, 58, f % 2 ? c : 0xffffff); lvi_box(B, 12, 106, 216, 54, (c & 0xfefefe) >> 1);
    b_text(B, 120 - (int)strlen(a) * 6, 112, a, lv_hex(c), 2); b_text(B, 120 - (int)strlen(b) * 6, 130, b, lv_hex(c), 2);
    lvi_txt(B, 120 - lv_w35(sub) / 2, 150, sub, 0xffffff);
    if (!win && f % 2 == 0) for (int k = 0; k < 2; ++k) lvi_box(B, k, 34 + k, 240 - 2 * k, 213 - 2 * k, 0xff385c);
}
#if LV_DL_SELFDESTRUCT
static void lv_dl_sd_draw(LvBand &B)   /* the open tower's fuse: lv_dl_banner's frame law (black, red box, half-dim inner box), on the tower body */
{
    const LvTower *T = nullptr; for (int n = 0; n < lv_rd_n; ++n) if (lv_rd_tw[n].i == (lv_dl_blown ? lv_dl_blown_tw : lv_dl_tw)) T = &lv_rd_tw[n];
    if (!T || (int)(lv_dl_clk.vt - lv_dl_cam_t) < LV_DL_CAM_MS) return;   /* after the pull-up lands, on the tower itself */
    int xa, xb, ya, yb; lv_rd_box(*T, 1, xa, xb, ya, yb);
    const int w = 66, h = 36; int x = (xa + xb) / 2 - w / 2; if (x < 2) x = 2; if (x > LV_DL_FX - 4 - w) x = LV_DL_FX - 4 - w;
    int y = (ya < LV_RD_Y0 ? LV_RD_Y0 : ya) + 18; if (y + h > LV_RD_PTOP - 12) y = LV_RD_PTOP - 12 - h;
    const int rm = lv_dl_blown ? 0 : lv_dl_sd_real(lv_dl_sd), s10 = (rm + 99) / 100; const bool urg = rm <= LV_DL_SD_URGENT;
    const uint32_t c = 0xff385c, t0 = lv_dl_blown ? lv_dl_pt : 0; const int f = (int)((lv_now - t0) / 150);
    lvi_rect(B, x, y, w, h, 0x000000); lvi_box(B, x, y, w, h, urg && f % 2 ? 0xffffff : c); lvi_box(B, x + 2, y + 2, w - 4, h - 4, (c & 0xfefefe) >> 1);
    lvi_txt(B, x + w / 2 - lv_w35("SELF-DESTRUCT") / 2, y + 5, "SELF-DESTRUCT", c);
    char t[8]; snprintf(t, sizeof t, "%d.%d", s10 / 10, s10 % 10); const int tw = (int)strlen(t) * 12;
    b_text(B, x + w / 2 - tw / 2, y + 12, t, lv_hex(urg && f % 2 == 0 ? 0xffffff : c), 2);
    const int bw = w - 8, fill = bw * rm / lv_dl_sd_real(LV_DL_SD_MS);   /* fuse left (the download-bar law) */
    lvi_box(B, x + 4, y + h - 7, bw, 4, c); if (fill > 2) lvi_rect(B, x + 5, y + h - 6, fill - 2, 2, c);
}
#endif
static void lv_dl_folder(LvBand &B, int i, int ys, float ph)   /* the INFO Joey folder frame (tab, 3D lip, scanlines), one file inside */
{
    const LvDlFile &fo = lv_dl_fo[lv_dl_tw][i]; const bool dl = lv_dl_pf == i;
    static const unsigned neon[3] = {0x30e8ff, 0xff2ad4, 0xffb000};
    const unsigned c = fo.st == LV_DL_S_GOT ? 0x30ff90 : fo.st == LV_DL_S_DENY ? 0xff385c : neon[i % 3];
    const int x = LV_DL_FX, w = LV_DL_FW, y = ys + 8, h = lv_dl_fbody();   /* body top y */
    if (!b_yhit(B, y - 8, y + h + 1)) return;   /* v1.2.11 speed: the folder (tab y-8 .. shadow y+h+1) misses this band */
    if (ph >= 0.65f) lvi_rect(B, x + 3, y + 2, w, h, 0x061321);
    lvi_line(B, x + w, y + 2, x + w + 3, y + 5, c); lvi_line(B, x + w + 3, y + 5, x + w + 3, y + h + 1, c); lvi_line(B, x + 3, y + h + 1, x + w + 3, y + h + 1, c);
    if (ph >= 0.65f) { lvi_rect(B, x, y, w, h, fo.st == LV_DL_S_GOT ? 0x06200f : fo.st == LV_DL_S_DENY ? 0x220610 : 0x050c16); for (int yy = y + 2; yy < y + h - 1; yy += 4) lvi_rect(B, x + 1, yy, w - 2, 1, 0x081422);
                       lvi_rect(B, x, y - 7, 60, 8, 0x050c16); }
    lvi_line(B, x, y - 8, x + 55, y - 8, c); lvi_line(B, x + 55, y - 8, x + 60, y - 3, c); lvi_line(B, x + 60, y - 3, x + 60, y, c); lvi_line(B, x, y - 8, x, y + h - 1, c);
    lvi_line(B, x + 60, y, x + w - 1, y, c); lvi_line(B, x + w - 1, y, x + w - 1, y + h - 1, c); lvi_line(B, x, y + h - 1, x + w - 1, y + h - 1, c);
    if (ph < 0.65f) return;
    char t[32], sz[12]; snprintf(t, sizeof t, "FILE %d", i + 1); lvi_txt(B, x + 4, y - 6, t, c);
    if (h < 40) {   /* smaller folders (4+ a tower): same fonts; name + size share one row, the bar sits under it */
        const bool idle = fo.st == LV_DL_S_IDLE && !dl; const int el = (int)(lv_now - lv_dl_pt);
        const int ty = h >= 26 ? y + 4 : y + (h - 9) / 2;   /* 7 px text row; 6-7 files: centred above the thin bar */
        lvi_icon(B, x + 4, ty - 1, fo.st == LV_DL_S_DENY ? 1 : 0, c); lvi_txt(B, x + 16, ty, lv_dl_name(fo), idle ? 0xffffff : c, true);
        lv_dl_size(fo.mb10, sz, sizeof sz); lvi_txt(B, x + w - 4 - (int)strlen(sz) * 6 + 1, ty, sz, idle ? 0xffc34a : c, true);
        if (h >= 26) {   /* 4-5 files: the bar + its word, as before */
            const int bw = 70, by = y + h - 11, fill = fo.st == LV_DL_S_GOT || fo.st == LV_DL_S_DENY ? bw - 4 : dl ? (bw - 4) * (el > LV_DL_DL_MS ? LV_DL_DL_MS : el) / LV_DL_DL_MS : 0;
            lvi_box(B, x + 4, by, bw, 7, c); lvi_rect(B, x + 6, by + 2, fill, 3, c);
            if (fo.st == LV_DL_S_GOT) snprintf(t, sizeof t, "DOWNLOADED"); else if (fo.st == LV_DL_S_DENY) snprintf(t, sizeof t, "DENIED");
            else if (dl) { const int p = 100 * el / LV_DL_DL_MS; snprintf(t, sizeof t, "%d%%", p > 99 ? 99 : p); } else snprintf(t, sizeof t, "TAP=DOWNLOAD");
            lvi_txt(B, x + w - 4 - lv_w35(t), by + 1, t, idle ? 0x6a8090 : c);
        } else {         /* 6-7 files: a 2 px bar across the folder; state = colour + icon (X = denied) */
            const int bw = w - 8, fill = fo.st == LV_DL_S_GOT || fo.st == LV_DL_S_DENY ? bw : dl ? bw * (el > LV_DL_DL_MS ? LV_DL_DL_MS : el) / LV_DL_DL_MS : 0;
            lvi_rect(B, x + 4, y + h - 4, bw, 2, 0x142637); lvi_rect(B, x + 4, y + h - 4, fill, 2, c);   /* text ends >= 2 rows above */
        }
        return;
    }
    lvi_icon(B, x + 4, y + 6, fo.st == LV_DL_S_DENY ? 1 : 0, c); lvi_txt(B, x + 16, y + 7, lv_dl_name(fo), fo.st == LV_DL_S_IDLE && !dl ? 0xffffff : c, true);
    lv_dl_size(fo.mb10, sz, sizeof sz); lvi_txt(B, x + w - 4 - (int)strlen(sz) * 6 + 1, y + 20, sz, fo.st == LV_DL_S_IDLE && !dl ? 0xffc34a : c, true);
    const int el = (int)(lv_now - lv_dl_pt), bw = 70;
    const int fill = fo.st == LV_DL_S_GOT || fo.st == LV_DL_S_DENY ? bw - 4 : dl ? (bw - 4) * (el > LV_DL_DL_MS ? LV_DL_DL_MS : el) / LV_DL_DL_MS : 0;
    lvi_box(B, x + 4, y + 34, bw, 7, c); lvi_rect(B, x + 6, y + 36, fill, 3, c);
    if (fo.st == LV_DL_S_GOT) snprintf(t, sizeof t, "DOWNLOADED"); else if (fo.st == LV_DL_S_DENY) snprintf(t, sizeof t, "DENIED");
    else if (dl) { const int p = 100 * el / LV_DL_DL_MS; snprintf(t, sizeof t, "%d%%", p > 99 ? 99 : p); }
    else snprintf(t, sizeof t, "TAP=DOWNLOAD");
    lvi_txt(B, x + w - 4 - lv_w35(t), y + 35, t, fo.st == LV_DL_S_IDLE && !dl ? 0x6a8090 : c);
}
static void lv_dl_tick_mark(LvBand &B, int x, int y, uint16_t c) { b_line(B, x, y + 2, x + 2, y + 4, c); b_line(B, x + 2, y + 4, x + 6, y, c); b_line(B, x, y + 3, x + 2, y + 5, c); b_line(B, x + 2, y + 5, x + 6, y + 1, c); }
static void lv_dl_terminal(LvBand &B)   /* rows 287..319: the radar terminal box; 2 log lines + the MISSION as the prompt line, always */
{
    const int x0 = 1, y0 = 287, x1 = 238, y1 = 319; char t[64], sz[12];
    b_fill(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_hex(0x000000)); b_frame(B, x0, y0, x1 - x0 + 1, y1 - y0 + 1, lv_ink(LV_ACC));
    const LvHcWorld &w = lv_hc_w; int typed = (int)((lv_now - w.tl_t) / 15);
    if (lv_dl_need > 1) {   /* 2+ files: the mission takes the whole box (the panel above shows the last result): FIND got/n, then every
                               target (name size; green + tick = downloaded) in 2 columns x 3 rows, 7 px apart */
        (void)typed; int x = b_text35(B, x0 + 3, y0 + 3, ">", lv_hex(LV_PROMPT_INK)); snprintf(t, sizeof t, "FIND %d/%d FILES", lv_dl_got, lv_dl_need);
        x = b_text35(B, x + 3, y0 + 3, t, lv_hex(0xffffff)); if ((lv_now / 500) & 1) b_fill(B, x + 2, y0 + 7, 4, 1, lv_hex(0xffffff));
        b_text35(B, x1 - 3 - lv_w35("NAME + SIZE"), y0 + 3, "NAME + SIZE", lv_hex(0x707078));
        b_fill(B, x0 + 2, y0 + 9, x1 - x0 - 3, 1, lv_ink(LV_ACCD));
        for (int k = 0; k < lv_dl_need; ++k) {
            const int xk = k < 3 ? x0 + 3 : x0 + 111, y = y0 + 12 + (k % 3) * 7; const bool g = lv_dl_tgot[k];
            lv_dl_size(lv_dl_tmb[k], sz, sizeof sz); snprintf(t, sizeof t, "%s %s", LV_DL_NAMES[lv_dl_tnm[k]], sz);
            const int xx = b_text35(B, xk, y, t, g ? lv_hex(0x30ff90) : lv_hex(0xffc34a));
            if (g) { b_line(B, xx + 2, y + 2, xx + 4, y + 4, lv_hex(0x30ff90)); b_line(B, xx + 4, y + 4, xx + 8, y, lv_hex(0x30ff90)); }
        }
        return;
    }
    snprintf(t, sizeof t, "%.52s", w.tl[1]); b_text35(B, x0 + 3, y0 + 3, t, lv_hex(0xa0a0a8));
    snprintf(t, sizeof t, "%.52s", w.tl[2]); if (typed < (int)strlen(t)) t[typed < 0 ? 0 : typed] = 0; b_text35(B, x0 + 3, y0 + 10, t, lv_hex(0xffffff));
    b_fill(B, x0 + 2, y0 + 17, x1 - x0 - 3, 1, lv_ink(LV_ACCD));
    lv_dl_size(lv_dl_tmb[0], sz, sizeof sz);
    int x = b_text(B, x0 + 3, y0 + 21, ">", lv_hex(LV_PROMPT_INK)); x = b_text(B, x + 2, y0 + 21, "FIND", lv_hex(0xffffff));
    snprintf(t, sizeof t, "%s %s", LV_DL_NAMES[lv_dl_tnm[0]], sz); x = b_text(B, x + 4, y0 + 21, t, lv_hex(0xffc34a));
    if ((lv_now / 500) & 1) b_fill(B, x + 1, y0 + 28, 5, 1, lv_hex(0xffffff));
}
static void lv_dl_panel(LvBand &B)   /* rows 247..285: the radar's locked panel frame: what to do now + the checked towers */
{
    const int py0 = LV_RD_PTOP; char t[48];
    b_fill(B, 2, py0, 236, 285 - py0, 0x0000); b_frame(B, 2, py0, 236, 285 - py0, LV_HC_MAG);
    const uint16_t W = lv_hex(0xffffff), TAN = lv_hex(LV_TAN);
    if (lv_dl_tw >= 0 && lv_dl_phase == LV_DL_PLAY) {
        const bool nt = lv_dl_note[0] && lv_dl_pf < 0;
        if (lv_dl_pf >= 0) snprintf(t, sizeof t, "DOWNLOADING..."); else if (lv_dl_why[0]) snprintf(t, sizeof t, "WRONG FILE!"); else if (nt) snprintf(t, sizeof t, "FILE FOUND! %d/%d", lv_dl_got, lv_dl_need); else snprintf(t, sizeof t, "TAP THE RIGHT FILE");
        b_text(B, 6, 252, t, lv_dl_why[0] ? lv_hex(0xff385c) : nt ? lv_hex(0x30ff90) : W);
        if (lv_dl_why[0]) b_text35(B, 6, 264, lv_dl_why, lv_hex(0xff8080)); else if (nt) b_text35(B, 6, 264, lv_dl_note, lv_hex(0x80ffb0)); else b_text35(B, 6, 264, "NAME AND SIZE MUST MATCH", TAN);
        snprintf(t, sizeof t, "NOT HERE? TAP %s", "< TOWERS"); b_text35(B, 6, 273, t, lv_hex(0x707078));
        const uint16_t c = lv_hex(0x30e8ff); b_fill(B, 173, 249, 63, 34, 0x0000); b_frame(B, 173, 249, 63, 34, c); b_frame(B, 175, 251, 59, 30, lv_dim(c, 3, 8));
        b_text(B, 173 + (63 - 8 * 6 + 1) / 2, 262, "< TOWERS", c);
        return;
    }
    const bool won = lv_dl_phase == LV_DL_DONE || lv_dl_phase == LV_DL_WIN, cut = lv_dl_phase == LV_DL_TRACED || lv_dl_phase == LV_DL_OVER;
    const bool m = lv_dl_need > 1;
    b_text(B, 6, 252, won ? (m ? "ALL FILES DOWNLOADED" : "FILE DOWNLOADED") : cut ? "CONNECTION CUT" : "TAP A TOWER TO OPEN IT", won ? lv_hex(0x30ff90) : cut ? lv_hex(0xff385c) : W);
    b_text35(B, 6, 264, won ? (m ? "YOU GOT EVERY FILE ON THE LIST" : "YOU GOT THE RIGHT FILE") : cut ? "THE TRACE FOUND YOU" : m ? "EACH FILE IS IN A DIFFERENT TOWER" : "CHECK EACH TOWER FOR THE FILE", TAN);
    b_text35(B, 6, 273, "CHECKED", lv_hex(0x707078));
    for (int i = 0; i < LV_DL_NT; ++i) {   /* 5 tower chips: T1..T5, green tick = checked */
        const int x = 38 + i * 26, y = 271; const bool s = lv_dl_seen[i];
        b_frame(B, x, y, 22, 10, s ? lv_hex(0x30ff90) : lv_hex(0x30e8ff)); snprintf(t, sizeof t, "T%d", i + 1); b_text35(B, x + 3, y + 3, t, s ? lv_hex(0x30ff90) : lv_hex(0x30e8ff));
        if (s) lv_dl_tick_mark(B, x + 13, y + 2, lv_hex(0x30ff90));
    }
}
static void lv_dl_sel_frame(LvBand &B, const LvRect &r, uint16_t c)   /* the open-folder cursor: a double theme frame */
{
    b_frame(B, r.x0, r.y0, r.x1 - r.x0 + 1, r.y1 - r.y0 + 1, c);
    b_frame(B, r.x0 + 1, r.y0 + 1, r.x1 - r.x0 - 1, r.y1 - r.y0 - 1, c);
}
static void lv_dl_save_card(LvBand &B)   /* same magenta Hot/Cold card frame as LEVEL CLEAR / TRACED (12,60 216x181), over the rain */
{
    const uint16_t W = lv_hex(0xffffff), G = lv_hex(0xa0a0a8), TAN = lv_hex(LV_TAN), CY = lv_hex(0x30e8ff), MAG = LV_HC_MAG;
    const uint16_t GRN = lv_hex(0x30ff60), GOLD = lv_hex(0xffc34a), DIM = lv_hex(0x707078);
    b_fill(B, 12, 60, 216, 181, 0x0000); b_frame(B, 12, 60, 216, 181, MAG); b_frame(B, 14, 62, 212, 177, lv_dim(MAG, 3, 8));
    char t[48], path[40];
    auto stamp = [&](int y) {
        if (lv_dl_exdata) {
            const char *s = lv_dl_sd_ok ? "EX.DATA  SD /BOTcorps/hackbot/" : "EX.DATA  NVS (NO SD)";
            b_text35(B, 120 - lv_w35(s) / 2, y, s, DIM);
        }
    };
    auto prog = [&](const LvDlSave &s, int y, uint16_t c) {
        snprintf(t, sizeof t, "L%d  SCORE %d", s.level, (int)s.score); b_text35(B, 120 - lv_w35(t) / 2, y, t, c);
        snprintf(t, sizeof t, "TOWERS %d/%d  FILES %d/%d", s.towers, LV_DL_NT, s.files_got, s.files_need ? s.files_need : s.files_got);
        b_text35(B, 120 - lv_w35(t) / 2, y + 9, t, G);
    };
    if (lv_dl_phase == LV_DL_SLOTS) {
        const char *title = "SAVE SLOTS"; const int tw = (int)strlen(title) * 12;
        b_text(B, 120 - tw / 2 + 1, 71, title, lv_hex(0x4a0a3a), 2); b_text(B, 120 - tw / 2, 70, title, MAG, 2);
        stamp(88);
        for (int i = 0; i < LV_DL_NSLOT; ++i) {
            const LvRect r = lv_dl_slot_rect(i); const LvDlSave &s = lv_dl_slot[i]; const bool on = i == lv_dl_slot_i;
            if (on) lv_dl_sel_frame(B, r, MAG);
            snprintf(t, sizeof t, "SLOT %d", i + 1); b_text(B, r.x0 + 8, r.y0 + 6, t, on ? W : G);
            if (!s.used) b_text(B, r.x0 + 8, r.y0 + 20, "NEW GAME", CY);
            else {
                snprintf(t, sizeof t, "L%d  SCORE %d", s.level, (int)s.score); b_text35(B, r.x0 + 8, r.y0 + 20, t, GOLD);
                snprintf(t, sizeof t, "TOWERS %d/%d  FILES %d/%d", s.towers, LV_DL_NT, s.files_got, s.files_need);
                b_text35(B, r.x0 + 8, r.y0 + 29, t, on ? W : G);
            }
        }
        return;
    }
    if (lv_dl_phase == LV_DL_PAUSE) {
        const char *title = "PAUSE"; const int tw = (int)strlen(title) * 12;
        b_text(B, 120 - tw / 2 + 1, 71, title, lv_hex(0x4a3a0a), 2); b_text(B, 120 - tw / 2, 70, title, GOLD, 2);
        snprintf(t, sizeof t, "SLOT %d IN USE", lv_dl_slot_i + 1); b_text35(B, 120 - lv_w35(t) / 2, 90, t, CY);
        { int ns = 0; for (int tw = 0; tw < LV_DL_NT; ++tw) ns += lv_dl_seen[tw];
          LvDlSave cur{}; cur.used = true; cur.level = (uint8_t)(lv_dl_round < 1 ? 1 : lv_dl_round);
          cur.score = (int16_t)lv_dl_score; cur.towers = (uint8_t)ns; cur.files_got = (uint8_t)lv_dl_got; cur.files_need = (uint8_t)lv_dl_need;
          prog(cur, 102, GOLD); }
        stamp(122);
        static const char *const ITEM[LV_DL_PMENU] = {"RESUME", "SAVE", "EXIT"};
        for (int i = 0; i < LV_DL_PMENU; ++i) {
            const LvRect r = lv_dl_pause_rect(i); const bool on = i == lv_dl_menu;
            if (on) lv_dl_sel_frame(B, r, MAG);
            const int lw = (int)strlen(ITEM[i]) * 6;
            b_text(B, 120 - lw / 2, r.y0 + 12, ITEM[i], on ? W : CY);
        }
        return;
    }
    /* SAVEOK */
    const char *title = "SLOT SAVED"; const int tw = (int)strlen(title) * 12;
    b_text(B, 120 - tw / 2 + 1, 79, title, lv_hex(0x0a4a1a), 2); b_text(B, 120 - tw / 2, 78, title, GRN, 2);
    snprintf(t, sizeof t, "SLOT %d", lv_dl_slot_i + 1); b_text(B, 120 - (int)strlen(t) * 3, 104, t, GOLD);
    prog(lv_dl_slot[lv_dl_slot_i], 124, W);
    if (lv_dl_sd_ok) { lv_dl_slot_path(lv_dl_slot_i, path, sizeof path); snprintf(t, sizeof t, "SD  %s", path); }
    else snprintf(t, sizeof t, "NVS  hbsl%d  (NO SD)", lv_dl_slot_i + 1);
    b_text35(B, 120 - lv_w35(t) / 2, 148, t, CY);
    if (lv_dl_sd_ok) { const char *fb = "NVS FALLBACK IF NO SD"; b_text35(B, 120 - lv_w35(fb) / 2, 158, fb, DIM); }
    stamp(176);
    const char *ok = "A=OK"; b_text(B, 120 - (int)strlen(ok) * 3, 200, ok, W);
}

static void lv_dl_scene(LvBand &B, void *)
{
    if (lv_dl_phase == LV_DL_INTRO) {   /* full-screen terminal: the mission types out */
        b_fill(B, B.x0, B.y0, B.w, B.h, 0x0000); b_frame(B, 0, 0, 240, 320, lv_ink(LV_ACC)); b_frame(B, 2, 2, 236, 316, lv_ink(LV_ACCD));
        char sz[12], l2[40], hd[40], tl[40], tg[24], tx[LV_DL_MAXT][32]; lv_dl_size(lv_dl_tmb[0], sz, sizeof sz); snprintf(l2, sizeof l2, "> %d TOWERS ONLINE. %d FILES EACH.", LV_DL_NT, lv_dl_nf);
        snprintf(hd, sizeof hd, "HACKBOT // GIBSON LINK   LEVEL %d", lv_dl_round); snprintf(tg, sizeof tg, "%s", LV_DL_NAMES[lv_dl_tnm[0]]);
        struct L { const char *s; uint32_t c; int sc; } Ls[24]; int NL = 0;
        auto add = [&](const char *a, uint32_t c, int sc) { Ls[NL++] = {a, c, sc}; };
        add(hd, 0xff2ad4, 1); add("> DIALING GIBSON ..... OK", 0xa0a0a8, 1); add("", 0, 1); add(l2, 0xa0a0a8, 1); add("", 0, 1);
        if (lv_dl_need == 1) { add("> TARGET: FIND THIS FILE", 0xffffff, 1); add(tg, 0xffc34a, 2); add(sz, 0xffc34a, 2); }
        else { snprintf(tl, sizeof tl, "> TARGETS: FIND ALL %d FILES", lv_dl_need); add(tl, 0xffffff, 1);
               for (int k = 0; k < lv_dl_need; ++k) { char z[12]; lv_dl_size(lv_dl_tmb[k], z, sizeof z); snprintf(tx[k], sizeof tx[k], "%d  %-12s %6s", k + 1, LV_DL_NAMES[lv_dl_tnm[k]], z); add(tx[k], 0xffc34a, 3); } }
        add("", 0, 1); add(lv_dl_need == 1 ? "> IT IS IN ONE TOWER." : "> EACH IS IN A DIFFERENT TOWER.", 0xffffff, 1); add("> OPEN EACH TOWER, READ THE FILES.", 0xffffff, 1);
        add("> NAME AND SIZE MUST MATCH.", 0xffffff, 1); add("", 0, 1);
        add("> TRACE RISES WITH TIME (90S)", 0xffb000, 1); add("> WRONG FILE = ACCESS DENIED", 0xffb000, 1); add("> TRACE 100% = TRACED", 0xff385c, 1); int budget = lv_dl_typed_all ? 1 << 20 : (int)((lv_now - lv_dl_intro_t) / 12); int y = 12;
        for (int k = 0; k < NL && budget > 0; ++k) {
            const int len = (int)strlen(Ls[k].s); char t[48]; snprintf(t, sizeof t, "%.*s", budget < len ? budget : len, Ls[k].s); budget -= len + 4;
            const int x = Ls[k].sc >= 2 ? 26 : 10;   /* sc 3 = a target row of a 2+ file list (1x, indented) */ if (Ls[k].sc == 2) b_text(B, x, y, t, lv_hex(Ls[k].c), 2); else b_text(B, x, y, t, lv_hex(Ls[k].c));
            y += Ls[k].sc == 2 ? 18 : (len ? 12 : 6);
        }
        lv_dl_intro_done = budget > 0 || lv_dl_typed_all;
        if (lv_dl_intro_done) {
            b_text35(B, 10, 252, "TRACE MODE: TIMER ONLY (90S)", lv_hex(LV_TAN));
            b_text35(B, 10, 260, "FAKE GAME FILES, NOT SCAN DATA", lv_hex(0x707078));
            b_text35(B, 10, 268, "DEFAULT: LEVEL 1 = 1 FILE, +1 A LEVEL, MAX 5", lv_hex(0x707078));
            int x = b_text(B, 10, 280, ">_", lv_hex(LV_PROMPT_INK)); x = b_text(B, x + 4, 280, "TAP TO START", lv_hex(0xffffff), 1);
            if ((lv_now / 500) & 1) b_fill(B, x + 2, 280, 6, 8, lv_hex(0xffffff));
        } else if ((lv_now / 250) & 1) b_fill(B, 10, y, 6, 8, lv_hex(0xffffff));
        lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
        return;
    }
    /* v1.2.15: the same INFO glyph plates, with this game's own bytes, never a scanner MAC.
     * Six-byte window [tower number, file slot, name id, mb10, state, target+1]; rotates among the real files every 300ms.
     * The original lvi_micro still selects byte pairs at its unchanged 30fps clock. */
    if (lv_dl_tw >= 0) {
        lv_rd_dl_rows_frame = (int)((uint32_t)(lv_now - lv_hc_w.lock_t) * 3u / 100u);
        const int k = (lv_rd_dl_rows_frame / 9) % lv_dl_nf; const LvDlFile &file = lv_dl_fo[lv_dl_tw][k];
        lv_rd_dl_rows[0] = (uint8_t)(lv_dl_tw + 1); lv_rd_dl_rows[1] = (uint8_t)(k + 1);
        lv_rd_dl_rows[2] = file.name; lv_rd_dl_rows[3] = file.mb10; lv_rd_dl_rows[4] = file.st; lv_rd_dl_rows[5] = (uint8_t)(file.tgt + 1);
    }
    lv_hc_swap(); lv_scene_radar(B, 0); lv_hc_swap();
    /* tower tags under each base; checked towers dimmed + green tick */
    if (B.y0 == 0) { lv_dl_ntag = 0; lv_dl_tag_tw = lv_dl_tw; }
    for (int k = 0; k < lv_hc_nhb; ++k) {
        const LvHcHit &h = lv_hc_hb[k]; const int i = h.i; char t[8]; snprintf(t, sizeof t, "T%d", i + 1);
        const LvTower *T = nullptr; for (int n = 0; n < lv_rd_n; ++n) if (lv_rd_tw[n].i == i) T = &lv_rd_tw[n];
        if (lv_dl_seen[i] && i != lv_dl_tw && T) { int xa, xb, ya, yb; lv_rd_box(*T, 1, xa, xb, ya, yb); if (ya < LV_RD_Y0) ya = LV_RD_Y0; if (yb > LV_RD_PTOP - 1) yb = LV_RD_PTOP - 1;
            for (int y = ya; y <= yb; ++y) for (int x = xa; x <= xb; ++x) b_blend(B, x, y, 0x0000, 5); }
        if (lv_dl_tw >= 0 && i != lv_dl_tw) continue;   /* a tower is open: only its own tag (no crowding next to the folders) */
        int y = T ? T->yb + (i == lv_dl_tw ? 7 : 3) : h.yb - 3;   /* the open tower's tag sits below its lock brackets (pad 3) */ if (y > LV_RD_PTOP - 8) y = LV_RD_PTOP - 8;
        const bool ck = lv_dl_seen[i] && i != lv_dl_tw; const int wd = lv_w35(t) + (ck ? 9 : 0);
        if (B.y0 == 0 && lv_dl_ntag < LV_DL_NT) lv_dl_tags[lv_dl_ntag++] = {(int16_t)(h.cx - wd / 2 - 1), (int16_t)(y - 1), (int16_t)(h.cx + wd / 2 + 1), (int16_t)(y + 5), (int16_t)i};
        b_fill(B, h.cx - wd / 2 - 1, y - 1, wd + 2, 7, 0x0000); b_text35(B, h.cx - wd / 2, y, t, ck ? lv_hex(0x30ff90) : lv_hex(0x30e8ff));
        if (lv_dl_seen[i] && i != lv_dl_tw) lv_dl_tick_mark(B, h.cx - wd / 2 + 9, y - 1, lv_hex(0x30ff90));
    }
    if (lv_dl_tw >= 0) {   /* the open tower's 3 folders drop in from the top of the scene */
        const int el = (int)(lv_dl_clk.vt - lv_dl_open_t);
        for (int i = lv_dl_nf - 1; i >= 0; --i) {   /* lv_select.h: the locked drop law */
            int y; float u; if (!lv_sel_drop(el, LV_DL_DROP0_MS, lv_dl_gap(), LV_DL_DROP_MS, i, LV_RD_Y0 - 50, LV_DL_FY0 + i * lv_dl_fdy(), y, u)) continue;
            lv_dl_folder(B, i, y, u > 1 ? 1 : u);
        }
    }
#if LV_DL_SELFDESTRUCT
    if ((lv_dl_phase == LV_DL_PLAY && lv_dl_tw >= 0 && lv_dl_sd > 0) || (lv_dl_phase == LV_DL_TRACED && lv_dl_blown)) lv_dl_sd_draw(B);
    if (lv_dl_phase == LV_DL_PLAY && lv_dl_tw >= 0 && lv_dl_sd > 0 && lv_dl_sd_real(lv_dl_sd) <= LV_DL_SD_URGENT && !((lv_now - lv_dl_t0) / 200 % 2)) for (int k = 0; k < 2; ++k) lvi_box(B, k, 34 + k, 240 - 2 * k, 213 - 2 * k, 0xff385c);
    if (lv_dl_sdk && lv_dl_phase == LV_DL_PLAY) { char a[16], t[32]; snprintf(a, sizeof a, "TOWER %d", lv_dl_sdk_tw + 1);
        if (lv_dl_sdk == 1) { const int s10 = (lv_dl_sdk_left + 99) / 100; snprintf(t, sizeof t, "OUT WITH %d.%dS LEFT", s10 / 10, s10 % 10); lv_dl_banner(B, true, "CLOSE", "CALL", t, 0x30ff90, lv_dl_sdk_t); }
        else lv_dl_banner(B, true, a, "BLEW", "NO TARGET INSIDE. NOTHING LOST", 0xffb000, lv_dl_sdk_t); }
#endif
    if (lv_dl_deny && lv_dl_phase == LV_DL_PLAY) lv_dl_banner(B, false, "ACCESS", "DENIED", "WRONG FILE", 0xff385c, lv_dl_deny_t);
    if (lv_dl_found && lv_dl_phase == LV_DL_PLAY) { char t[32]; snprintf(t, sizeof t, "%d OF %d. KEEP GOING", lv_dl_got, lv_dl_need); lv_dl_banner(B, true, "FILE", "FOUND", t, 0x30ff90, lv_dl_found_t); }
    if (lv_dl_phase == LV_DL_PLAY && lv_dl_trace >= 800 && !((lv_now - lv_dl_t0) / 200 % 2)) for (int k = 0; k < 2; ++k) lvi_box(B, k, 34 + k, 240 - 2 * k, 213 - 2 * k, 0xff385c);
#if LV_DL_DECK   /* v1.2.14 (Mobandy 11:34 AM ET "hackerbot has no control ui"): the v1.2.11 MOCK's game deck, real: it replaces the panel + terminal; d-pad cursor = theme frame on folder lv_dl_fsel */
    if (lv_dl_tw >= 0 && lv_dl_fsel >= 0 && lv_sel_landed((int)(lv_dl_clk.vt - lv_dl_open_t), LV_DL_DROP0_MS, lv_dl_gap(), LV_DL_DROP_MS, lv_dl_fsel)) { const LvRect r = lv_dl_ftap(lv_dl_fsel); const uint16_t k = lv_ink(LV_ACC);
        b_frame(B, r.x0 + 1, r.y0 + 1, r.x1 - r.x0 - 1, r.y1 - r.y0 - 1, k); b_frame(B, r.x0 + 2, r.y0 + 2, r.x1 - r.x0 - 3, r.y1 - r.y0 - 3, k); }
    { char a[40], b[24];
      if (lv_dl_phase == LV_DL_SLOTS) { snprintf(a, sizeof a, "SLOT %d OF 3  UP/DN PICK", lv_dl_slot_i + 1); snprintf(b, sizeof b, "A=OPEN"); pad_hc_deck(B, a, b, "SLOT", "----"); }
      else if (lv_dl_phase == LV_DL_PAUSE) { snprintf(a, sizeof a, "PAUSE  SLOT %d", lv_dl_slot_i + 1); snprintf(b, sizeof b, "A=SELECT"); pad_hc_deck(B, a, b, "ITEM", "----"); }
      else if (lv_dl_phase == LV_DL_SAVEOK) { snprintf(a, sizeof a, "SAVED  SLOT %d", lv_dl_slot_i + 1); snprintf(b, sizeof b, "A=OK"); pad_hc_deck(B, a, b, "----", "----"); }
      else if (lv_dl_tw >= 0) { snprintf(a, sizeof a, "TOWER %d  FILE %d/%d", lv_dl_tw + 1, lv_dl_fsel + 1, lv_dl_nf); snprintf(b, sizeof b, "A=DOWNLOAD"); pad_hc_deck(B, a, b); }
      else { snprintf(a, sizeof a, "GOT %d OF %d  L/R PICK TOWER", lv_dl_got, lv_dl_need); snprintf(b, sizeof b, "A=OPEN"); pad_hc_deck(B, a, b); } }
#else
    lv_dl_panel(B); lv_dl_terminal(B);
#endif
    if (lv_dl_phase == LV_DL_DONE) lv_dl_banner(B, true, "DOWNLOAD", "COMPLETE", lv_dl_need > 1 ? "ALL FILES" : "RIGHT FILE", 0x30ff90, lv_dl_pt);
#if LV_DL_SELFDESTRUCT
    if (lv_dl_phase == LV_DL_TRACED && lv_dl_blown) { char t[40]; int k = -1; for (int j = 0; j < lv_dl_need; ++j) if (lv_dl_ttw[j] == lv_dl_blown_tw && !lv_dl_tgot[j]) k = j;
        snprintf(t, sizeof t, "%s LOST IN TOWER %d", k >= 0 ? LV_DL_NAMES[lv_dl_tnm[k]] : "FILE", lv_dl_blown_tw + 1); lv_dl_banner(B, false, "SELF", "DESTRUCT", t, 0xff385c, lv_dl_pt); } else
#endif
    if (lv_dl_phase == LV_DL_TRACED) lv_dl_banner(B, false, "TRACE", "COMPLETE", "CONNECTION CUT", 0xff385c, lv_dl_pt);
    if (lv_dl_phase == LV_DL_WIN || lv_dl_phase == LV_DL_OVER) lv_dl_card(B);
    if (lv_dl_phase == LV_DL_SLOTS || lv_dl_phase == LV_DL_PAUSE || lv_dl_phase == LV_DL_SAVEOK) lv_dl_save_card(B);
    lv_draw_backbot(B, LV_R_BACKBOT.x0, LV_R_BACKBOT.y0);
    lv_dl_chrome(B);
}
static void lv_dl_frame(void)
{
    lv_dl_tick();
    static const LvRect all = {0, 0, 239, 319};
    lv_render(lv_blit_dirty_hw, all, lv_dl_scene, 0);   /* v1.2.4: skip theme re-ink; look unchanged. v1.2.17: changed rows only */
}
static void lv_go_soon(int scr, const char *tty_cmd);
#if LV_DL_DECK
/* v1.2.14: the deck's keys on the real game actions. 1 up 2 down 3 left 4 right (lv_pad_dpad_at), 5 A, 6 B (GIBSON's A / B). */
static int lv_dl_deck_id(int x, int y) {   /* mock/v1219: L/R rocker boxes (v1.2.13), not the v1.2.14 4-way diagonals. A OPEN / B BACK unchanged. */
#if LV_DL_SPEED
    if (x >= PAD_MID_X && x < PAD_MID_X + PAD_MID_W && y >= PAD_MID_YH && y < PAD_MID_YW + PAD_MID_H) return 10;   /* mock/v1220 item 5: SPEED (GIBSON's id 10 box) */
#endif
    if (lv_in(PAD_HIT_L, x, y)) return 3;
    if (lv_in(PAD_HIT_R, x, y)) return 4;
    return lv_in(PAD_HIT_A, x, y) ? 5 : lv_in(PAD_HIT_B, x, y) ? 6 : 0;
}
static void lv_dl_touch(bool down, int x, int y) { const bool pad = lv_dl_phase == LV_DL_PLAY || lv_dl_phase == LV_DL_SLOTS || lv_dl_phase == LV_DL_PAUSE || lv_dl_phase == LV_DL_SAVEOK; lv_hc_dpad_held = down && pad && y >= PAD_Y0 ? lv_dl_deck_id(x, y) : 0; }
static const char *lv_dl_tap(int x, int y);
static void lv_dl_pick(int dir)   /* L / R / UP / DOWN on the game world's towers (lv_rd_dpad_pick, the GIBSON rows law) */
{
    lv_hc_swap();
    const int j = lv_rd_dpad_pick(lv_rd_sel, dir);
    if (j >= 0) { lv_rd_sel = j; memcpy(lv_rd_selmac, lv_f[j].mac, 6); lv_rd_glow = true; lv_rd_glow_t = lv_now; }
    lv_hc_swap();
}
static const char *lv_dl_deck(int x, int y)
{
    static const int DIR[5] = {0, 2, 3, 0, 1};   /* d-pad id -> lv_rd_dpad_pick dir */
    const int id = lv_dl_deck_id(x, y); if (!id) return "dl-deck";
#if LV_DL_SPEED
    if (id == 10) { lv_dl_speed = lv_dl_speed % 3 + 1; static const char *const S[3] = {"dl-speed-1x", "dl-speed-2x", "dl-speed-3x"}; return S[lv_dl_speed - 1]; }
#endif
    if (lv_dl_tw >= 0) {   /* a tower is open */
        if (id == 1 || id == 2) { lv_dl_fsel = (lv_dl_fsel + (id == 2 ? 1 : lv_dl_nf - 1)) % lv_dl_nf; return id == 1 ? "dl-pad-up-file" : "dl-pad-down-file"; }
        if (id == 5) { const LvRect r = lv_dl_ftap(lv_dl_fsel); return lv_dl_tap((r.x0 + r.x1) / 2, (r.y0 + r.y1) / 2); }   /* A = a tap on that folder */
        if (lv_dl_pf >= 0) return "dl-busy";
        const int t = lv_dl_tw; lv_dl_close_tower(); lv_hc_w.sel = t; memcpy(lv_hc_w.selmac, lv_hc_w.f[t].mac, 6);   /* B / L / R: close it (as "< TOWERS"), the cursor stays on it */
        if (id == 6) return "dl-pad-b-towers";
        lv_dl_pick(DIR[id]); return id == 3 ? "dl-pad-left" : "dl-pad-right";
    }
    if (id == 6) { lv_dl_open_pause(); return "dl-pad-b-pause"; }   /* field B opens the pause menu (RESUME / SAVE / EXIT). EXIT goes home without saving. */
    if (lv_dl_cam_mode == 2 && (int32_t)(lv_sel_clk_now(lv_dl_clk, lv_now) - lv_dl_cam_t) < LV_DL_CAM_MS) return "dl-camera-moving";
    if (id == 5) { int i = lv_hc_w.sel; if (i < 0 || i >= lv_hc_w.nf) { lv_dl_pick(1); i = lv_hc_w.sel; } if (i < 0) return "dl-pad-a-empty"; lv_dl_open_tower(i); return "dl-pad-a-open"; }
    lv_dl_pick(DIR[id]); static const char *const N[5] = {"", "dl-pad-up-row", "dl-pad-down-row", "dl-pad-left", "dl-pad-right"}; return N[id];
}
#endif
static const char *lv_dl_menu_deck(int id)
{
#if LV_DL_SPEED
    if (id == 10) { lv_dl_speed = lv_dl_speed % 3 + 1; static const char *const S[3] = {"dl-speed-1x", "dl-speed-2x", "dl-speed-3x"}; return S[lv_dl_speed - 1]; }
#endif
    if (lv_dl_phase == LV_DL_SLOTS) {
        if (id == 1) { lv_dl_slot_i = (lv_dl_slot_i + LV_DL_NSLOT - 1) % LV_DL_NSLOT; return "dl-slot-up"; }
        if (id == 2) { lv_dl_slot_i = (lv_dl_slot_i + 1) % LV_DL_NSLOT; return "dl-slot-down"; }
        if (id == 5) { lv_dl_apply_slot(lv_dl_slot_i); return lv_dl_slot[lv_dl_slot_i].used && lv_dl_phase == LV_DL_PLAY ? "dl-slot-continue" : "dl-slot-new"; }
        if (id == 6) { lv_go_soon(LV_SC_HOME, 0); return "dl-slot-back"; }
        return "dl-slot";
    }
    if (lv_dl_phase == LV_DL_PAUSE) {
        if (id == 1) { lv_dl_menu = (lv_dl_menu + LV_DL_PMENU - 1) % LV_DL_PMENU; return "dl-pause-up"; }
        if (id == 2) { lv_dl_menu = (lv_dl_menu + 1) % LV_DL_PMENU; return "dl-pause-down"; }
        if (id == 6) { lv_dl_resume_play(); return "dl-pause-back"; }
        if (id == 5) {
            if (lv_dl_menu == 1) { lv_dl_do_save(); return "dl-pause-save"; }
            if (lv_dl_menu == 2) { lv_go_soon(LV_SC_HOME, 0); return "dl-pause-exit"; }   /* home, no save */
            lv_dl_resume_play(); return "dl-pause-resume";
        }
        return "dl-pause";
    }
    if (lv_dl_phase == LV_DL_SAVEOK) {
        if (id == 5 || id == 6) { lv_dl_resume_play(); return "dl-saved-ok"; }
        return "dl-saved";
    }
    return "dl-menu";
}
static const char *lv_dl_tap(int x, int y)
{
    if (lv_dl_phase == LV_DL_SLOTS) {
        if (y >= PAD_Y0) return lv_dl_menu_deck(lv_dl_deck_id(x, y));
        for (int i = 0; i < LV_DL_NSLOT; ++i) if (lv_in(lv_dl_slot_rect(i), x, y)) {
            if (lv_dl_slot_i != i) { lv_dl_slot_i = i; return "dl-slot-pick"; }
            lv_dl_apply_slot(i); return lv_dl_phase == LV_DL_PLAY ? "dl-slot-continue" : "dl-slot-new";
        }
        return "dl-slot";
    }
    if (lv_dl_phase == LV_DL_PAUSE) {
        if (y >= PAD_Y0) return lv_dl_menu_deck(lv_dl_deck_id(x, y));
        for (int i = 0; i < LV_DL_PMENU; ++i) if (lv_in(lv_dl_pause_rect(i), x, y)) {
            if (lv_dl_menu != i) { lv_dl_menu = i; return "dl-pause-pick"; }
            return lv_dl_menu_deck(5);
        }
        return "dl-pause";
    }
    if (lv_dl_phase == LV_DL_SAVEOK) {
        if (y >= PAD_Y0) return lv_dl_menu_deck(lv_dl_deck_id(x, y));
        lv_dl_resume_play(); return "dl-saved-ok";
    }
    if (lv_dl_phase == LV_DL_INTRO) {
        if (!lv_dl_intro_done) { lv_dl_typed_all = true; return "dl-intro-skip-typing"; }   /* first tap while typing: show it all */
        lv_dl_play(); return "dl-start";
    }
    if (lv_dl_phase == LV_DL_WIN || lv_dl_phase == LV_DL_OVER) {
        const LvRect L = {(int16_t)(LV_HC_BTN_L.x0 - 3), (int16_t)(LV_HC_BTN_L.y0 - 3), (int16_t)(LV_HC_BTN_L.x1 + 2), (int16_t)(LV_HC_BTN_L.y1 + 3)};
        const LvRect R = {(int16_t)(LV_HC_BTN_R.x0 - 2), (int16_t)(LV_HC_BTN_R.y0 - 3), (int16_t)(LV_HC_BTN_R.x1 + 3), (int16_t)(LV_HC_BTN_R.y1 + 3)};
        if (lv_in(L, x, y)) { const bool w = lv_dl_phase == LV_DL_WIN; if (w) { ++lv_dl_round; lv_dl_try = 0; } else ++lv_dl_try; lv_dl_enter(); return w ? "dl-next-level" : "dl-retry"; }   /* both open that level's intro (its file list) */
        if (lv_in(R, x, y)) { lv_go_soon(LV_SC_HOME, 0); return "dl-back-home"; }
        return "dl-card";
    }
    if (lv_dl_phase != LV_DL_PLAY) return "dl-wait";
#if LV_DL_DECK
    if (y >= PAD_Y0) return lv_dl_deck(x, y);   /* v1.2.14: the deck owns rows 247.. (the old panel's "< TOWERS" = B) */
#endif
    if (lv_dl_tw < 0) {
        if (lv_dl_cam_mode == 2 && (int32_t)(lv_sel_clk_now(lv_dl_clk, lv_now) - lv_dl_cam_t) < LV_DL_CAM_MS) return "dl-camera-moving";
        if (y < LV_RD_Y0 || y >= LV_RD_PTOP) return "dl-none";
        const int i = lv_hc_hit(x, y); if (i < 0) return "dl-miss";
        lv_dl_open_tower(i); return "dl-open-tower";
    }
    if (lv_in(LV_DL_BTN_T, x, y)) { if (lv_dl_pf >= 0) return "dl-busy"; lv_dl_close_tower(); return "dl-towers"; }
    const int el = (int)(lv_sel_clk_now(lv_dl_clk, lv_now) - lv_dl_open_t);
    for (int i = 0; i < lv_dl_nf; ++i) if (lv_in(lv_dl_ftap(i), x, y)) {
        if (!lv_sel_landed(el, LV_DL_DROP0_MS, lv_dl_gap(), LV_DL_DROP_MS, i)) return "dl-folder-falling";   /* lv_select.h landing rule */
        if (lv_dl_pf >= 0) return "dl-busy";
        LvDlFile &f = lv_dl_fo[lv_dl_tw][i]; if (f.st != LV_DL_S_IDLE) return "dl-file-done";
        lv_dl_why[0] = lv_dl_note[0] = 0; f.st = LV_DL_S_DL; lv_dl_pf = i; lv_dl_pt = lv_now; ++lv_dl_dls;
        char sz[12]; lv_dl_size(f.mb10, sz, sizeof sz); lv_hc_log("DOWNLOAD %s %s", lv_dl_name(f), sz); return "dl-download";
    }
    return "dl-none";
}
#endif
