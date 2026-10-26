/* CONTROLbot layout P+L — ctrltheme (pt359u): no decorative power; theme = centre module;
 * working chips re-spaced clear of the 3-line plate. */
static const CbR CB_P_RECT[9] = {
    { 78, 78, 84, 120 }, /* 0 = centre module (theme); not painted as a side chip */
    { 36, 198, 34, 50 }, /* 1 MIX */
    { 36, 86, 34, 50 },  /* 2 SHAPE */
    { 178, 136, 20, 10 },/* 3 RAIN MODE */
    { 42, 148, 22, 22 }, /* 4 RAIN COL */
    { 170, 78, 34, 50 }, /* 5 BACKGROUND */
    { 180, 232, 10, 8 }, /* 6 BG MOTION */
    { 170, 175, 34, 50 },/* 7 MULTI */
    { 96, 232, 64, 18 }  /* 8 ORIENT */
};
static const int CB_P_MOD[4] = { 78, 78, 84, 120 };
static const int CB_P_MET[3] = { 36, 40, 76 };
/* SER/TACT/PWR kept for ABI; paint no longer draws them (decorative / duplicate of crown). */
static const int CB_P_SER[2] = { 0, 0 };
static const int CB_P_TACT[2][2] = { { 0, 0 }, { 0, 0 } };
static const int CB_P_PWR[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
static const int8_t CB_P_RN[9] = { 0, 3, 3, 3, 3, 3, 3, 3, 3 };
static const int16_t CB_P_RT[9][6][2] = {
    { { 0, 0 } },
    { { 70, 223 }, { 78, 223 }, { 78, 198 } },
    { { 70, 111 }, { 78, 111 }, { 78, 138 } },
    { { 178, 141 }, { 162, 141 }, { 162, 138 } },
    { { 64, 159 }, { 78, 159 }, { 78, 138 } },
    { { 170, 103 }, { 162, 103 }, { 162, 138 } },
    { { 180, 236 }, { 162, 236 }, { 162, 198 } },
    { { 170, 200 }, { 162, 200 }, { 162, 138 } },
    { { 128, 232 }, { 128, 198 }, { 120, 198 } }
};
static const int CB_P_DN = 1;
static const int8_t CB_P_DL[4] = { 3, 0, 0, 0 };
static const int16_t CB_P_DT[4][6][2] = { { { 74, 62 }, { 74, 78 }, { 78, 78 } }, { { 0, 0 } }, { { 0, 0 } }, { { 0, 0 } } };

static const CbR CB_L_RECT[9] = {
    { 123, 40, 74, 104 },
    { 250, 100, 34, 50 },
    { 36, 90, 34, 50 },
    { 210, 100, 20, 10 },
    { 42, 150, 22, 22 },
    { 250, 40, 34, 50 },
    { 100, 158, 10, 8 },
    { 80, 90, 34, 50 },
    { 160, 155, 72, 18 }
};
static const int CB_L_MOD[4] = { 123, 40, 74, 104 };
static const int CB_L_MET[3] = { 36, 40, 76 };
static const int CB_L_SER[2] = { 0, 0 };
static const int CB_L_TACT[2][2] = { { 0, 0 }, { 0, 0 } };
static const int CB_L_PWR[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
static const int8_t CB_L_RN[9] = { 0, 2, 2, 3, 2, 2, 2, 2, 3 };
static const int16_t CB_L_RT[9][6][2] = {
    { { 0, 0 } },
    { { 250, 125 }, { 197, 125 } },
    { { 70, 115 }, { 123, 115 } },
    { { 210, 105 }, { 197, 105 }, { 197, 92 } },
    { { 64, 161 }, { 123, 161 } },
    { { 250, 65 }, { 197, 65 } },
    { { 110, 162 }, { 123, 162 } },
    { { 114, 115 }, { 123, 115 } },
    { { 196, 164 }, { 196, 144 }, { 160, 144 } }
};
static const int CB_L_DN = 1;
static const int8_t CB_L_DL[4] = { 2, 0, 0, 0 };
static const int16_t CB_L_DT[4][6][2] = { { { 74, 62 }, { 123, 62 } }, { { 0, 0 } }, { { 0, 0 } }, { { 0, 0 } } };
