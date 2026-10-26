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
    { 36, 40, 44, 44 }, { 160, 40, 44, 44 }, { 36, 200, 44, 44 }, { 160, 200, 44, 44 },
    { 92, 114, 56, 56 }
};
static const int HA_CHIP_L[5][4] = {
    /* L margins 40 / 280; GIB centre x=160; vertical pack above plate cast 182 */
    { 40, 34, 48, 48 }, { 232, 34, 48, 48 }, { 40, 134, 48, 48 }, { 232, 134, 48, 48 },
    { 132, 74, 56, 56 }
};
static const int8_t HA_RN[4] = { 3, 3, 3, 3 };
static const int16_t HA_RT_P[4][4][2] = {
    { { 58, 84 }, { 58, 114 }, { 92, 114 } },
    { { 182, 84 }, { 182, 114 }, { 148, 114 } },
    { { 58, 200 }, { 58, 170 }, { 92, 170 } },
    { { 182, 200 }, { 182, 170 }, { 148, 170 } } };
static const int16_t HA_RT_L[4][4][2] = {
    { { 88, 58 }, { 88, 74 }, { 132, 74 } },
    { { 232, 58 }, { 232, 74 }, { 188, 74 } },
    { { 64, 134 }, { 64, 130 }, { 132, 130 } },
    { { 256, 134 }, { 256, 130 }, { 188, 130 } } };

/* Variant B — horizontal backplane bus */
static const int HB_CHIP_P[5][4] = {
    { 40, 36, 44, 44 }, { 156, 36, 44, 44 }, { 40, 198, 44, 44 }, { 156, 198, 44, 44 },
    { 92, 112, 56, 56 }
};
static const int HB_CHIP_L[5][4] = {
    { 40, 34, 48, 48 }, { 232, 34, 48, 48 }, { 40, 134, 48, 48 }, { 232, 134, 48, 48 },
    { 132, 74, 56, 56 }
};
static const int8_t HB5B_RN[4] = { 4, 4, 4, 4 };
static const int16_t HB_RT_P[4][4][2] = {
    { { 62, 80 }, { 62, 100 }, { 92, 100 }, { 92, 112 } },
    { { 178, 80 }, { 178, 100 }, { 148, 100 }, { 148, 112 } },
    { { 62, 198 }, { 62, 178 }, { 92, 178 }, { 92, 168 } },
    { { 178, 198 }, { 178, 178 }, { 148, 178 }, { 148, 168 } } };
static const int16_t HB_RT_L[4][4][2] = {
    { { 64, 82 }, { 64, 102 }, { 132, 102 }, { 132, 102 } },
    { { 256, 82 }, { 256, 102 }, { 188, 102 }, { 188, 102 } },
    { { 64, 134 }, { 64, 120 }, { 132, 120 }, { 132, 120 } },
    { { 256, 134 }, { 256, 120 }, { 188, 120 }, { 188, 120 } } };

/* Variant C — diamond/compass + ring */
static const int HC_CHIP_P[5][4] = {
    { 98, 36, 44, 44 }, { 166, 118, 44, 44 }, { 36, 118, 44, 44 }, { 98, 196, 44, 44 },
    { 92, 112, 56, 56 }
};
static const int HC_CHIP_L[5][4] = {
    /* N BOOT / E SCAN / W CTRL / S TERM — mirrored about x=160; no overlaps */
    { 138, 30, 44, 44 }, { 236, 80, 44, 44 }, { 40, 80, 44, 44 }, { 138, 138, 44, 44 },
    { 132, 76, 56, 56 }
};
static const int8_t HC_RN[4] = { 3, 3, 3, 3 };
static const int16_t HC_RT_P[4][4][2] = {
    { { 120, 80 }, { 120, 112 }, { 120, 112 } },
    { { 166, 140 }, { 148, 140 }, { 148, 140 } },
    { { 80, 140 }, { 92, 140 }, { 92, 140 } },
    { { 120, 196 }, { 120, 168 }, { 120, 168 } } };
static const int16_t HC_RT_L[4][4][2] = {
    { { 160, 74 }, { 160, 76 }, { 160, 76 } },
    { { 236, 102 }, { 188, 102 }, { 188, 102 } },
    { { 84, 102 }, { 132, 102 }, { 132, 102 } },
    { { 160, 138 }, { 160, 132 }, { 160, 132 } } };

static const int HB_TBOT_P[4] = { 31, 261, 177, 49 };
static const int HB_TBOT_L[4] = { 34, 184, 252, 49 };
static const char *const HB_TBOT_SILK = "TERMINALBOT";
static const char *const HB_SILK1[5] = { "BOOT", "SCAN", "CONTROL", "TERMINAL", "GIBSON" };

/* TERMINALbot board — square silver centre, BOOT/SCAN/CONTROL around */
static const int HT_CHIP_P[5][4] = {
    { 36, 40, 44, 44 }, { 160, 40, 44, 44 }, { 98, 198, 44, 44 }, { 92, 112, 56, 56 }, { 0, 0, 0, 0 }
};
static const int HT_CHIP_L[5][4] = {
    { 40, 34, 48, 48 }, { 232, 34, 48, 48 }, { 136, 134, 48, 48 }, { 132, 74, 56, 56 }, { 0, 0, 0, 0 }
};
static const int8_t HT_RN[3] = { 3, 3, 3 };
static const int16_t HT_RT_P[3][4][2] = {
    { { 58, 84 }, { 58, 112 }, { 92, 112 } },
    { { 182, 84 }, { 182, 112 }, { 148, 112 } },
    { { 120, 198 }, { 120, 168 }, { 120, 168 } } };
static const int16_t HT_RT_L[3][4][2] = {
    { { 88, 58 }, { 88, 74 }, { 132, 74 } },
    { { 232, 58 }, { 232, 74 }, { 188, 74 } },
    { { 160, 134 }, { 160, 130 }, { 160, 130 } } };

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
        HB_ROUTE_N = HA_RN;
        HB_RT = P ? HA_RT_P : HA_RT_L;
        HB_RNcount = 4;
    }
    HB_MOD = HB_CHIPR[HB4_GIB];
}
