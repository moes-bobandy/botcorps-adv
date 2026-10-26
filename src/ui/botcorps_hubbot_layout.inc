/* HOME hub4 (Mobandy 3:25–3:29 PM ET Oct 26): GIBSONbot big in the middle, the other four in the
 * board corners — TL BOOTbot (drawn, not tappable), TR SCANbot, BL CONTROLbot, BR TERMINALbot —
 * wired to GIBSON with copper. Shared 3-line Adv TERMINALBOT plate at the bottom, untouched.
 * Rects are x,y,w,h; all clear of the top bar (y>=28), the 34 px side headers and the plate. */
enum { HB4_BOOT = 0, HB4_SCAN = 1, HB4_CTRL = 2, HB4_TERM = 3, HB4_GIB = 4, HB4_N = 5 };

static const int HB_CHIP_P[5][4] = {
    { 36, 34, 38, 58 },    /* BOOT     TL */
    { 166, 34, 38, 58 },   /* SCAN     TR */
    { 36, 194, 38, 58 },   /* CONTROL  BL  (end 252; plate cast 259) */
    { 166, 194, 38, 58 },  /* TERMINAL BR */
    { 84, 34, 72, 218 }    /* GIBSON   centre, end 252 */
};
static const int HB_CHIP_L[5][4] = {
    { 38, 32, 52, 70 },    /* BOOT     TL */
    { 230, 32, 52, 70 },   /* SCAN     TR */
    { 38, 108, 52, 70 },   /* CONTROL  BL  (end 178; plate cast 182) */
    { 230, 108, 52, 70 },  /* TERMINAL BR */
    { 100, 32, 120, 146 }  /* GIBSON   centre, end 178 */
};

static const int HB_TBOT_P[4] = { 31, 261, 177, 49 };
static const int HB_TBOT_L[4] = { 34, 184, 252, 49 };
static const char *const HB_TBOT_SILK = "TERMINALBOT";
static const char *const HB_SILK1[5] = { "BOOT", "SCAN", "CONTROL", "TERMINAL", "GIBSON" };
static const int HB_SILK_PITCH = 6;

/* copper: corner chip inner edge -> GIBSON edge (orthogonal, via at the bend) */
static const int8_t HB_RN_P[5] = { 3, 3, 3, 3, 0 };
static const int16_t HB_RT_P[5][4][2] = {
    { { 55, 92 }, { 55, 104 }, { 84, 104 } },
    { { 185, 92 }, { 185, 104 }, { 155, 104 } },
    { { 55, 194 }, { 55, 182 }, { 84, 182 } },
    { { 185, 194 }, { 185, 182 }, { 155, 182 } },
    { { 0, 0 } }
};
static const int8_t HB_RN_L[5] = { 3, 3, 3, 3, 0 };
static const int16_t HB_RT_L[5][4][2] = {
    { { 90, 52 }, { 95, 57 }, { 100, 57 } },
    { { 230, 52 }, { 225, 57 }, { 219, 57 } },
    { { 90, 158 }, { 95, 153 }, { 100, 153 } },
    { { 230, 158 }, { 225, 153 }, { 219, 153 } },
    { { 0, 0 } }
};

/* rain covers: top bar, side pins (no rain on the pins), plate, five chips */
static const botcorps_cover_t HB_COVER_P[10] = {
    { 0, 0, 240, 26 }, { 0, 26, 34, 294 }, { 206, 26, 34, 294 }, { 0, 312, 240, 8 },
    { 27, 257, 185, 58 },
    { 33, 31, 44, 64 }, { 163, 31, 44, 64 }, { 33, 191, 44, 64 }, { 163, 191, 44, 64 }, { 80, 30, 80, 226 } };
static const botcorps_cover_t HB_COVER_L[10] = {
    { 0, 0, 320, 26 }, { 0, 26, 36, 214 }, { 284, 26, 36, 214 }, { 0, 230, 320, 10 },
    { 30, 180, 260, 56 },
    { 35, 29, 58, 76 }, { 227, 29, 58, 76 }, { 35, 105, 58, 76 }, { 227, 105, 58, 76 }, { 96, 28, 128, 154 } };


/* TERMINALbot board (Mobandy 3:30 PM ET): same PCB look, big TERMINALbot centre, BOOT TL (drawn, not
 * tappable), SCAN TR, CONTROL bottom-centre; NO GIBSON. Shown on SCR_GIBSON while s_hub_term is set. */
static const int HT_CHIP_P[5][4] = {
    { 36, 34, 38, 58 },    /* BOOT     TL */
    { 166, 34, 38, 58 },   /* SCAN     TR */
    { 98, 196, 44, 56 },   /* CONTROL  bottom-centre (end 252) */
    { 84, 34, 72, 150 },   /* TERMINAL centre (end 184) */
    { 0, 0, 0, 0 }         /* no GIBSON */
};
static const int HT_CHIP_L[5][4] = {
    { 38, 32, 52, 70 },
    { 230, 32, 52, 70 },
    { 134, 128, 52, 50 },  /* CONTROL  bottom-centre (end 178) */
    { 100, 32, 120, 88 },  /* TERMINAL centre (end 120) */
    { 0, 0, 0, 0 }
};
static const int8_t HT_RN[3] = { 3, 3, 2 };
static const int16_t HT_RT_P[3][4][2] = {
    { { 55, 92 }, { 55, 104 }, { 84, 104 } },
    { { 185, 92 }, { 185, 104 }, { 155, 104 } },
    { { 120, 196 }, { 120, 184 } } };
static const int16_t HT_RT_L[3][4][2] = {
    { { 90, 52 }, { 95, 57 }, { 100, 57 } },
    { { 230, 52 }, { 225, 57 }, { 219, 57 } },
    { { 160, 128 }, { 160, 120 } } };
static const botcorps_cover_t HT_COVER_P[9] = {
    { 0, 0, 240, 26 }, { 0, 26, 34, 294 }, { 206, 26, 34, 294 }, { 0, 312, 240, 8 }, { 27, 257, 185, 58 },
    { 33, 31, 44, 64 }, { 163, 31, 44, 64 }, { 95, 186, 50, 68 }, { 80, 30, 80, 158 } };
static const botcorps_cover_t HT_COVER_L[9] = {
    { 0, 0, 320, 26 }, { 0, 26, 36, 214 }, { 284, 26, 36, 214 }, { 0, 230, 320, 10 }, { 30, 180, 260, 56 },
    { 35, 29, 58, 76 }, { 227, 29, 58, 76 }, { 131, 120, 58, 60 }, { 96, 28, 128, 96 } };

static bool s_hub_term = false;   /* SCR_GIBSON shows the TERMINALbot board */
static const int (*HB_CHIPR)[4] = HB_CHIP_P;
static const int *HB_TBOT = HB_TBOT_P;
static const int8_t *HB_RN = HB_RN_P;
static const int16_t (*HB_RT)[4][2] = HB_RT_P;
static const int *HB_MOD = HB_CHIP_P[HB4_GIB];
static void hb_layout_sync(void)
{
    const bool P = botcorps_cyd_orient != 0;
    HB_CHIPR = s_hub_term ? (P ? HT_CHIP_P : HT_CHIP_L) : (P ? HB_CHIP_P : HB_CHIP_L);
    HB_TBOT = P ? HB_TBOT_P : HB_TBOT_L;
    HB_RN = P ? HB_RN_P : HB_RN_L;
    HB_RT = P ? HB_RT_P : HB_RT_L;
    HB_MOD = (P ? HB_CHIP_P : HB_CHIP_L)[HB4_GIB];   /* intro face geo always = home GIBSON */
}
