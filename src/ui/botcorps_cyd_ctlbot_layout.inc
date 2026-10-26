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

/* hub5 L (Mobandy 4:13): module centred x=160; left/right chip columns mirrored; pads ≥28 */
static const CbR CB_L_RECT[9] = {
    { 123, 34, 74, 96 },   /* 0 centre module — centre x=160 */
    { 40, 100, 36, 48 },   /* 1 MIX   left-lower  */
    { 40, 40, 36, 48 },    /* 2 SHAPE left-upper  */
    { 40, 154, 28, 28 },   /* 3 RAIN MODE  left of meters */
    { 252, 154, 28, 28 },  /* 4 RAIN COL   right (mirror) */
    { 244, 40, 36, 48 },   /* 5 BACKGROUND right-upper (mirror SHAPE) */
    { 220, 154, 28, 28 },  /* 6 BG MOTION  mid-right */
    { 244, 100, 36, 48 },  /* 7 MULTI right-lower (mirror MIX) */
    { 72, 154, 28, 28 }    /* 8 ORIENT     mid-left */
};
static const int CB_L_MOD[4] = { 123, 34, 74, 96 };
static const int CB_L_MET[3] = { 0, 0, 0 };  /* L: no room under centred module; theme on module silk */
static const int CB_L_SER[2] = { 0, 0 };
static const int CB_L_TACT[2][2] = { { 0, 0 }, { 0, 0 } };
static const int CB_L_PWR[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
static const int8_t CB_L_RN[9] = { 0, 2, 2, 2, 2, 2, 2, 2, 2 };
static const int16_t CB_L_RT[9][6][2] = {
    { { 0, 0 } },
    { { 76, 124 }, { 123, 124 } },
    { { 76, 64 }, { 123, 64 } },
    { { 54, 154 }, { 54, 126 } },
    { { 266, 154 }, { 266, 126 } },
    { { 244, 64 }, { 197, 64 } },
    { { 234, 154 }, { 197, 126 } },
    { { 244, 124 }, { 197, 124 } },
    { { 86, 154 }, { 112, 154 } }
};
static const int CB_L_DN = 1;
static const int8_t CB_L_DL[4] = { 2, 0, 0, 0 };
static const int16_t CB_L_DT[4][6][2] = { { { 80, 50 }, { 123, 50 } }, { { 0, 0 } }, { { 0, 0 } }, { { 0, 0 } } };
