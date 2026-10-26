/* HOME hub5 (Mobandy 4:07 PM ET Oct 26): THREE selectable boards A/B/C.
 * Gibson square ~1.25x the other four. Rich PCB circuitry. SCAN metallic gold,
 * TERM metallic silver, BOOT black+white shades (not tappable), CONTROL=theme.
 * Default variant A via s_hub5_var. TERMINALbot board via s_hub_term. */
enum { HB4_BOOT = 0, HB4_SCAN = 1, HB4_CTRL = 2, HB4_TERM = 3, HB4_GIB = 4, HB4_N = 5 };
enum { HUB5_A = 0, HUB5_B = 1, HUB5_C = 2, HUB5_N = 3 };

static int s_hub5_var = HUB5_A;
static bool s_hub_term = false;

/* Variant A — radial corners */
static const int HA_CHIP_P[5][4] = {
    /* hub6: main chip = CONTROLbot module 84x120 at 78,78; 40 sq corners */
    { 36, 32, 40, 40 }, { 164, 32, 40, 40 }, { 36, 206, 40, 40 }, { 164, 206, 40, 40 },
    { 78, 78, 84, 120 }
};
static const int HA_CHIP_L[5][4] = {
    /* hub6: main chip = CONTROLbot module 74x96; 44 sq corners mirrored 40/236; bottom ends 174 */
    { 40, 34, 44, 44 }, { 236, 34, 44, 44 }, { 40, 130, 44, 44 }, { 236, 130, 44, 44 },
    { 123, 34, 74, 96 }
};
static const int8_t HA_RN[4] = { 3, 3, 3, 3 };
static const int8_t HA_RN_L[4] = { 4, 4, 4, 4 };
static const int16_t HA_RT_P[4][4][2] = {
    { { 56, 72 }, { 56, 100 }, { 78, 100 } },
    { { 184, 72 }, { 184, 100 }, { 162, 100 } },
    { { 56, 206 }, { 56, 176 }, { 78, 176 } },
    { { 184, 206 }, { 184, 176 }, { 162, 176 } } };
static const int16_t HA_RT_L[4][4][2] = {
    { { 84, 56 }, { 104, 56 }, { 104, 82 }, { 123, 82 } },
    { { 236, 56 }, { 216, 56 }, { 216, 82 }, { 197, 82 } },
    { { 84, 152 }, { 104, 152 }, { 104, 110 }, { 123, 110 } },
    { { 236, 152 }, { 216, 152 }, { 216, 110 }, { 197, 110 } } };

/* Variant B — horizontal backplane bus */
static const int HB_CHIP_P[5][4] = {
    { 36, 32, 40, 40 }, { 164, 32, 40, 40 }, { 36, 206, 40, 40 }, { 164, 206, 40, 40 },
    { 78, 78, 84, 120 }
};
static const int HB_CHIP_L[5][4] = {
    { 40, 34, 44, 44 }, { 236, 34, 44, 44 }, { 40, 130, 44, 44 }, { 236, 130, 44, 44 },
    { 123, 34, 74, 96 }
};
static const int8_t HB5B_RN[4] = { 4, 4, 4, 4 };
static const int16_t HB_RT_P[4][4][2] = {
    { { 56, 72 }, { 56, 90 }, { 78, 90 }, { 78, 90 } },
    { { 184, 72 }, { 184, 90 }, { 162, 90 }, { 162, 90 } },
    { { 56, 206 }, { 56, 186 }, { 78, 186 }, { 78, 186 } },
    { { 184, 206 }, { 184, 186 }, { 162, 186 }, { 162, 186 } } };
static const int16_t HB_RT_L[4][4][2] = {
    { { 62, 78 }, { 62, 96 }, { 123, 96 }, { 123, 96 } },
    { { 258, 78 }, { 258, 96 }, { 197, 96 }, { 197, 96 } },
    { { 62, 130 }, { 62, 112 }, { 123, 112 }, { 123, 112 } },
    { { 258, 130 }, { 258, 112 }, { 197, 112 }, { 197, 112 } } };

/* Variant C — diamond/compass + ring */
static const int HC_CHIP_P[5][4] = {
    { 100, 32, 40, 40 }, { 166, 118, 40, 40 }, { 34, 118, 40, 40 }, { 100, 206, 40, 40 },
    { 78, 78, 84, 120 }
};
static const int HC_CHIP_L[5][4] = {
    /* hub6 L: W CTRL / E SCAN beside the main chip, BOOT|TERM pair below, mirrored about x=160 */
    { 76, 134, 40, 40 }, { 240, 62, 40, 40 }, { 40, 62, 40, 40 }, { 204, 134, 40, 40 },
    { 123, 34, 74, 96 }
};
static const int8_t HC_RN[4] = { 3, 3, 3, 3 };
static const int16_t HC_RT_P[4][4][2] = {
    { { 120, 72 }, { 120, 78 }, { 120, 78 } },
    { { 166, 138 }, { 162, 138 }, { 162, 138 } },
    { { 74, 138 }, { 78, 138 }, { 78, 138 } },
    { { 120, 206 }, { 120, 198 }, { 120, 198 } } };
static const int16_t HC_RT_L[4][4][2] = {
    { { 96, 134 }, { 96, 118 }, { 123, 118 } },
    { { 240, 82 }, { 197, 82 }, { 197, 82 } },
    { { 80, 82 }, { 123, 82 }, { 123, 82 } },
    { { 224, 134 }, { 224, 118 }, { 197, 118 } } };

static const int HB_TBOT_P[4] = { 31, 261, 177, 49 };
static const int HB_TBOT_L[4] = { 34, 184, 252, 49 };
static const char *const HB_TBOT_SILK = "TERMINALBOT";
static const char *const HB_SILK1[5] = { "BOOT", "SCAN", "CONTROL", "TERMINAL", "GIBSON" };

/* TERMINALbot board — square silver centre, BOOT/SCAN/CONTROL around */
static const int HT_CHIP_P[5][4] = {
    { 36, 32, 40, 40 }, { 164, 32, 40, 40 }, { 100, 206, 40, 40 }, { 78, 78, 84, 120 }, { 0, 0, 0, 0 }
};
static const int HT_CHIP_L[5][4] = {
    /* hub6: TERMINALBOT main chip 74x96 at y30; CTRL 132..172 (sel 175) clears plate cast 182 */
    { 40, 34, 44, 44 }, { 236, 34, 44, 44 }, { 140, 132, 40, 40 }, { 123, 30, 74, 96 }, { 0, 0, 0, 0 }
};
static const int8_t HT_RN[3] = { 3, 3, 3 };
static const int16_t HT_RT_P[3][4][2] = {
    { { 56, 72 }, { 56, 100 }, { 78, 100 } },
    { { 184, 72 }, { 184, 100 }, { 162, 100 } },
    { { 120, 206 }, { 120, 202 }, { 120, 198 } } };
static const int16_t HT_RT_L[3][4][2] = {
    { { 84, 56 }, { 104, 56 }, { 123, 56 } },
    { { 236, 56 }, { 216, 56 }, { 197, 56 } },
    { { 160, 132 }, { 160, 129 }, { 160, 126 } } };

static const int (*HB_CHIPR)[4] = HA_CHIP_P;
static const int *HB_TBOT = HB_TBOT_P;
static const int8_t *HB_ROUTE_N = HA_RN;
static const int16_t (*HB_RT)[4][2] = HA_RT_P;
static int HB_RNcount = 4;
static const int *HB_MOD = HA_CHIP_P[HB4_GIB];

static void hb_layout_sync(void)
{
    const bool P = botcorps_cyd_orient != 0;
    HB_TBOT = P ? HB_TBOT_P : HB_TBOT_L;
    if (s_hub_term) {
        HB_CHIPR = P ? HT_CHIP_P : HT_CHIP_L;
        HB_ROUTE_N = HT_RN;
        HB_RT = P ? HT_RT_P : HT_RT_L;
        HB_RNcount = 3;
        HB_MOD = HB_CHIPR[HB4_TERM]; /* centre is TERM slot on term board */
        return;
    }
    const int v = (s_hub5_var < 0 || s_hub5_var >= HUB5_N) ? HUB5_A : s_hub5_var;
    if (v == HUB5_B) {
        HB_CHIPR = P ? HB_CHIP_P : HB_CHIP_L;
        HB_ROUTE_N = HB5B_RN;
        HB_RT = P ? HB_RT_P : HB_RT_L;
        HB_RNcount = 4;
    } else if (v == HUB5_C) {
        HB_CHIPR = P ? HC_CHIP_P : HC_CHIP_L;
        HB_ROUTE_N = HC_RN;
        HB_RT = P ? HC_RT_P : HC_RT_L;
        HB_RNcount = 4;
    } else {
        HB_CHIPR = P ? HA_CHIP_P : HA_CHIP_L;
        HB_ROUTE_N = P ? HA_RN : HA_RN_L;
        HB_RT = P ? HA_RT_P : HA_RT_L;
        HB_RNcount = 4;
    }
    HB_MOD = HB_CHIPR[HB4_GIB];
}
