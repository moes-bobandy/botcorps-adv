/* HOME hub7 (Mobandy 4:57 PM ET Oct 26): ONE home layout (replaces the hub5 A/B/C options).
 * GIBSON upper centre; BOOT / SCAN / CONTROL / TERMINAL around it, every one drawn with the
 * shared main-bot chip (drawBotChipK) at one uniform scale, wired to GIBSON with real copper.
 * BOOT = GIBSON inverted (not tappable), SCAN gold, TERM silver, CONTROL theme.
 * s_hub5_var is kept for the host API but no longer changes the board.
 * hub8: home P re-packed (GIBSON full size upper centre, CTRL/TERM large below); s_hub_term =
 * TERMINALbot board: TERM is the centre chip with BOOT/SCAN/CTRL only (no GIBSON). */
enum { HB4_BOOT = 0, HB4_SCAN = 1, HB4_CTRL = 2, HB4_TERM = 3, HB4_GIB = 4, HB4_N = 5 };
enum { HUB5_A = 0, HUB5_B = 1, HUB5_C = 2, HUB5_N = 3 };

[[maybe_unused]] static int s_hub5_var = HUB5_A;
static bool s_hub_term = false;

/* hub8: per-slot rects + scale {x, y, w, h, k16}; w/h = mb_w/mb_h at k16. Index = HB4_*.
 * Home: [HB4_GIB] = GIBSON (centre). TERMINALbot board: [HB4_TERM] = TERM (centre), no GIBSON
 * (Mobandy: "TERMINALbot will get just the bots no Gibson") -> [HB4_GIB] is empty (w = 0). */
/* hub12 (Mobandy 8:54 PM ET): SCAN <-> CONTROL swapped on HOME — SCANbot takes the big lower-left
 * slot (k13), CONTROLbot the small upper-right one (k8). Route geometry is per slot, so copper is unchanged. */
static const int HN_HOME_P[5][5] = {          /* GIBSON full size, BOOT/CTRL flank, SCAN/TERM large below */
    { 32, 51, 42, 60, 8 }, { 33, 157, 68, 98, 13 }, { 166, 51, 42, 60, 8 }, { 139, 157, 68, 98, 13 },
    { 78, 30, 84, 120, 16 }
};
static const int HN_HOME_L[5][5] = {
    { 42, 28, 56, 72, 12 }, { 42, 104, 56, 72, 12 }, { 222, 28, 56, 72, 12 }, { 222, 104, 56, 72, 12 },   /* hub12 swap */
    { 123, 30, 74, 96, 16 }
};
static const int HN_TERM_P[5][5] = {          /* TERM full size, BOOT/SCAN flank, CTRL large below */
    { 32, 51, 42, 60, 8 }, { 166, 51, 42, 60, 8 }, { 86, 157, 68, 98, 13 }, { 78, 30, 84, 120, 16 },
    { 0, 0, 0, 0, 0 }
};
static const int HN_TERM_L[5][5] = {          /* TERM top centre, CTRL under it (height-limited), BOOT/SCAN either side at TERM size */
    { 48, 64, 60, 78, 13 }, { 212, 64, 60, 78, 13 }, { 134, 110, 51, 66, 11 }, { 130, 28, 60, 78, 13 },
    { 0, 0, 0, 0, 0 }
};
/* copper (2 px, cb_line at x and x+1 so mirrors use W-2-x); <= 4 points per route */
static const int8_t HN_RN_P[6] = { 2, 2, 3, 3, 2, 2 };
static const int16_t HN_RT_P[6][4][2] = {
    { { 74, 95 }, { 77, 95 } },                                   /* BOOT -> GIBSON */
    { { 162, 95 }, { 165, 95 } },                                 /* CTRL -> GIBSON (hub12 swap) */
    { { 112, 150 }, { 112, 200 }, { 101, 200 } },                 /* GIBSON -> SCAN (hub12 swap) */
    { { 126, 150 }, { 126, 200 }, { 137, 200 } },                 /* GIBSON -> TERM */
    { { 52, 111 }, { 52, 157 } },                                 /* BOOT -> SCAN (hub12 swap) */
    { { 186, 111 }, { 186, 157 } } };                             /* CTRL -> TERM (hub12 swap) */
static const int8_t HN_RN_L[6] = { 2, 2, 4, 4, 3, 3 };
static const int16_t HN_RT_L[6][4][2] = {
    { { 98, 62 }, { 123, 62 } },                                  /* BOOT -> GIBSON */
    { { 220, 62 }, { 195, 62 } },                                 /* CTRL -> GIBSON (hub12 swap) */
    { { 98, 140 }, { 110, 140 }, { 110, 110 }, { 123, 110 } },    /* SCAN -> GIBSON (hub12 swap) */
    { { 220, 140 }, { 208, 140 }, { 208, 110 }, { 195, 110 } },   /* TERM -> GIBSON */
    { { 150, 126 }, { 150, 160 }, { 98, 160 } },                  /* GIBSON bus -> SCAN (hub12 swap) */
    { { 168, 126 }, { 168, 160 }, { 220, 160 } } };               /* GIBSON bus -> TERM */
static const int8_t HT_RN_P[5] = { 2, 2, 2, 3, 3 };
static const int16_t HT_RT_P[5][4][2] = {
    { { 74, 95 }, { 77, 95 } },                                   /* BOOT -> TERM */
    { { 162, 95 }, { 165, 95 } },                                 /* SCAN -> TERM */
    { { 119, 150 }, { 119, 157 } },                               /* TERM -> CTRL */
    { { 52, 111 }, { 52, 200 }, { 84, 200 } },                    /* BOOT -> CTRL */
    { { 186, 111 }, { 186, 200 }, { 154, 200 } } };               /* SCAN -> CTRL */
static const int8_t HT_RN_L[5] = { 2, 2, 2, 3, 3 };
static const int16_t HT_RT_L[5][4][2] = {
    { { 108, 80 }, { 129, 80 } },                                 /* BOOT -> TERM */
    { { 210, 80 }, { 189, 80 } },                                 /* SCAN -> TERM */
    { { 159, 106 }, { 159, 110 } },                               /* TERM -> CTRL */
    { { 78, 142 }, { 78, 160 }, { 133, 160 } },                   /* BOOT -> CTRL */
    { { 240, 142 }, { 240, 160 }, { 185, 160 } } };              /* SCAN -> CTRL */

static const int HB_TBOT_P[4] = { 31, 261, 177, 49 };
static const int HB_TBOT_L[4] = { 34, 184, 252, 49 };
static const char *const HB_TBOT_SILK = "TERMINALBOT";
static const char *const HB_SILK1[5] = { "BOOT", "SCAN", "CONTROL", "TERMINAL", "GIBSON" };

/* active tables (home or TERMINALbot board, per orientation) */
static int HB_CHIPR[5][4];
static int HB_KS[5];                            /* per-slot chip scale (k/16) */
static const int *HB_TBOT = HB_TBOT_P;
static const int8_t *HB_ROUTE_N = HN_RN_P;
static const int16_t (*HB_RT)[4][2] = HN_RT_P;
static int HB_RNcount = 6;
static const int *HB_MOD = HB_CHIPR[HB4_GIB];   /* centre chip rect (GIBSON, or TERM on the term board) */
static int HB_KMID = 16;                        /* centre chip scale */

static void hb_layout_sync(void)
{
    const bool P = botcorps_cyd_orient != 0;
    const int (*T)[5] = s_hub_term ? (P ? HN_TERM_P : HN_TERM_L) : (P ? HN_HOME_P : HN_HOME_L);
    HB_TBOT = P ? HB_TBOT_P : HB_TBOT_L;
    if (s_hub_term) { HB_ROUTE_N = P ? HT_RN_P : HT_RN_L; HB_RT = P ? HT_RT_P : HT_RT_L; HB_RNcount = 5; }
    else { HB_ROUTE_N = P ? HN_RN_P : HN_RN_L; HB_RT = P ? HN_RT_P : HN_RT_L; HB_RNcount = 6; }
    for (int i = 0; i < 5; ++i) {
        for (int q = 0; q < 4; ++q) HB_CHIPR[i][q] = T[i][q];
        HB_KS[i] = T[i][4];
    }
    const int mid = s_hub_term ? HB4_TERM : HB4_GIB;
    HB_MOD = HB_CHIPR[mid];
    HB_KMID = HB_KS[mid];
}
