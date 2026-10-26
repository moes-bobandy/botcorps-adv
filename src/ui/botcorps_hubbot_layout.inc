/* HOME hub7 (Mobandy 4:57 PM ET Oct 26): ONE home layout (replaces the hub5 A/B/C options).
 * GIBSON upper centre; BOOT / SCAN / CONTROL / TERMINAL around it, every one drawn with the
 * shared main-bot chip (drawBotChipK) at one uniform scale, wired to GIBSON with real copper.
 * BOOT = GIBSON inverted (not tappable), SCAN gold, TERM silver, CONTROL theme.
 * s_hub5_var is kept for the host API but no longer changes the board. s_hub_term = TERMINALbot
 * board: TERM takes the centre slot and GIBSON takes TERM's slot. */
enum { HB4_BOOT = 0, HB4_SCAN = 1, HB4_CTRL = 2, HB4_TERM = 3, HB4_GIB = 4, HB4_N = 5 };
enum { HUB5_A = 0, HUB5_B = 1, HUB5_C = 2, HUB5_N = 3 };

static int s_hub5_var = HUB5_A;
static bool s_hub_term = false;

/* chip scale (k/16 of the orientation's main-chip size): [0] = the four bots, [1] = centre */
static const int HN_K_P[2] = { 10, 11 };   /* P: bots 53x75, GIBSON 58x83 */
static const int HN_K_L[2] = { 12, 16 };   /* L: bots 56x72, GIBSON 74x96 (full) */
/* rects (x, y, w, h); w/h = mb_w/mb_h at that scale. Index = HB4_*; [HB4_GIB] = centre slot. */
static const int HN_CHIP_P[5][4] = {
    { 31, 34, 53, 75 }, { 156, 34, 53, 75 }, { 60, 152, 53, 75 }, { 127, 152, 53, 75 },
    { 91, 30, 58, 83 }
};
static const int HN_CHIP_L[5][4] = {
    { 42, 28, 56, 72 }, { 222, 28, 56, 72 }, { 42, 104, 56, 72 }, { 222, 104, 56, 72 },
    { 123, 30, 74, 96 }
};
/* copper (2 px, cb_line at x and x+1 so mirrors use W-2-x); <= 4 points per route */
static const int8_t HN_RN_P[6] = { 2, 2, 4, 4, 2, 2 };
static const int16_t HN_RT_P[6][4][2] = {
    { { 84, 72 }, { 91, 72 } },                                   /* BOOT -> GIBSON */
    { { 155, 72 }, { 148, 72 } },                                 /* SCAN -> GIBSON */
    { { 86, 152 }, { 86, 132 }, { 104, 132 }, { 104, 113 } },     /* CTRL -> GIBSON */
    { { 152, 152 }, { 152, 132 }, { 134, 132 }, { 134, 113 } },   /* TERM -> GIBSON */
    { { 70, 109 }, { 70, 152 } },                                 /* BOOT -> CTRL */
    { { 168, 109 }, { 168, 152 } } };                             /* SCAN -> TERM */
static const int8_t HN_RN_L[6] = { 2, 2, 4, 4, 3, 3 };
static const int16_t HN_RT_L[6][4][2] = {
    { { 98, 62 }, { 123, 62 } },                                  /* BOOT -> GIBSON */
    { { 220, 62 }, { 195, 62 } },                                 /* SCAN -> GIBSON */
    { { 98, 140 }, { 110, 140 }, { 110, 110 }, { 123, 110 } },    /* CTRL -> GIBSON */
    { { 220, 140 }, { 208, 140 }, { 208, 110 }, { 195, 110 } },   /* TERM -> GIBSON */
    { { 150, 126 }, { 150, 160 }, { 98, 160 } },                  /* GIBSON bus -> CTRL */
    { { 168, 126 }, { 168, 160 }, { 220, 160 } } };               /* GIBSON bus -> TERM */

static const int HB_TBOT_P[4] = { 31, 261, 177, 49 };
static const int HB_TBOT_L[4] = { 34, 184, 252, 49 };
static const char *const HB_TBOT_SILK = "TERMINALBOT";
static const char *const HB_SILK1[5] = { "BOOT", "SCAN", "CONTROL", "TERMINAL", "GIBSON" };

/* active tables; on the TERMINALbot board TERM and GIBSON swap slots */
static int HB_CHIPR[5][4];
static const int *HB_TBOT = HB_TBOT_P;
static const int8_t *HB_ROUTE_N = HN_RN_P;
static const int16_t (*HB_RT)[4][2] = HN_RT_P;
static int HB_RNcount = 6;
static const int *HB_MOD = HB_CHIPR[HB4_GIB];   /* centre chip rect (GIBSON, or TERM on the term board) */
static int HB_KBOT = 10, HB_KMID = 11;

static void hb_layout_sync(void)
{
    const bool P = botcorps_cyd_orient != 0;
    const int (*T)[4] = P ? HN_CHIP_P : HN_CHIP_L;
    HB_TBOT = P ? HB_TBOT_P : HB_TBOT_L;
    HB_ROUTE_N = P ? HN_RN_P : HN_RN_L;
    HB_RT = P ? HN_RT_P : HN_RT_L;
    HB_RNcount = 6;
    HB_KBOT = (P ? HN_K_P : HN_K_L)[0];
    HB_KMID = (P ? HN_K_P : HN_K_L)[1];
    for (int i = 0; i < 5; ++i) {
        int src = i;
        if (s_hub_term && i == HB4_TERM) src = HB4_GIB;
        else if (s_hub_term && i == HB4_GIB) src = HB4_TERM;
        for (int q = 0; q < 4; ++q) HB_CHIPR[i][q] = T[src][q];
    }
    HB_MOD = HB_CHIPR[s_hub_term ? HB4_TERM : HB4_GIB];
}
