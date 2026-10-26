/* CONTROLbot layout P+L — ctrltheme (pt359u): no decorative power; theme = centre module;
 * working chips re-spaced clear of the 3-line plate. */
static const CbR CB_P_RECT[9] = {
    { 78, 86, 84, 120 }, /* 0 = centre module (hub6: y86, under the meters) (theme); not painted as a side chip */
    { 35, 198, 36, 50 }, /* 1 MIX */
    { 36, 86, 34, 50 },  /* 2 SHAPE */
    { 178, 136, 20, 10 },/* 3 RAIN MODE */
    { 42, 148, 22, 22 }, /* 4 RAIN COL */
    { 170, 78, 34, 50 }, /* 5 BACKGROUND */
    { 180, 228, 10, 8 }, /* 6 BG MOTION */
    { 170, 175, 34, 50 },/* 7 MULTI */
    { 96, 228, 64, 18 }  /* 8 ORIENT */
};
static const int CB_P_MOD[4] = { 78, 86, 84, 120 };
static const int CB_P_MET[3] = { 36, 45, 76 };   /* hub6: under the power strip */
/* SER/TACT/PWR kept for ABI; paint no longer draws them (decorative / duplicate of crown). */
static const int CB_P_SER[2] = { 0, 0 };
static const int CB_P_TACT[2][2] = { { 0, 0 }, { 0, 0 } };
static const int CB_P_PWR[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
static const int8_t CB_P_RN[9] = { 0, 3, 3, 3, 3, 3, 3, 3, 3 };
static const int16_t CB_P_RT[9][6][2] = {
    { { 0, 0 } },
    { { 71, 223 }, { 78, 223 }, { 78, 198 } },
    { { 70, 111 }, { 78, 111 }, { 78, 138 } },
    { { 178, 141 }, { 162, 141 }, { 162, 138 } },
    { { 64, 159 }, { 78, 159 }, { 78, 138 } },
    { { 170, 103 }, { 162, 103 }, { 162, 138 } },
    { { 180, 232 }, { 162, 232 }, { 162, 198 } },
    { { 170, 200 }, { 162, 200 }, { 162, 138 } },
    { { 128, 228 }, { 128, 206 }, { 128, 206 } }
};
static const int CB_P_DN = 1;
static const int8_t CB_P_DL[4] = { 3, 0, 0, 0 };
static const int16_t CB_P_DT[4][6][2] = { { { 74, 62 }, { 74, 78 }, { 78, 78 } }, { { 0, 0 } }, { { 0, 0 } }, { { 0, 0 } } };

/* hub6 L: power strip y27..36 under the top bar; shared 74x96 main chip at 123,38; chip
 * columns mirrored about x=160 (SHAPE|MIX  ...  BG|MULTI), labelled parts mirrored under them,
 * the two wide toggles (ORIENT | BG MOTION) mirrored on the bottom row. Everything <= 177. */
static const CbR CB_L_RECT[9] = {
    { 123, 38, 74, 96 },   /* 0 centre module (shared main-bot chip) */
    { 80, 40, 36, 48 },    /* 1 MIX    left inner  */
    { 40, 40, 36, 48 },    /* 2 SHAPE  left outer  */
    { 62, 96, 28, 28 },    /* 3 RAIN MODE  left (labels 127..146) */
    { 230, 96, 28, 28 },   /* 4 RAIN COL   right mirror */
    { 204, 40, 36, 48 },   /* 5 BACKGROUND right inner (mirror MIX) */
    { 208, 160, 72, 16 },  /* 6 BG MOTION  wide toggle, right bottom (mirror ORIENT) */
    { 244, 40, 36, 48 },   /* 7 MULTI  right outer (mirror SHAPE) */
    { 40, 160, 72, 16 }    /* 8 ORIENT     wide toggle, left bottom */
};
static const int CB_L_MOD[4] = { 123, 38, 74, 96 };
static const int CB_L_MET[3] = { 0, 0, 0 };  /* L: no room under centred module; theme on module silk */
static const int CB_L_SER[2] = { 0, 0 };
static const int CB_L_TACT[2][2] = { { 0, 0 }, { 0, 0 } };
static const int CB_L_PWR[5][3] = { { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 }, { 0, 0, 0 } };
static const int8_t CB_L_RN[9] = { 0, 3, 3, 3, 3, 3, 3, 3, 3 };
static const int16_t CB_L_RT[9][6][2] = {
    { { 0, 0 } },
    { { 116, 62 }, { 123, 62 }, { 123, 62 } },
    { { 58, 88 }, { 58, 91 }, { 123, 91 } },
    { { 90, 110 }, { 123, 110 }, { 123, 110 } },
    { { 230, 110 }, { 197, 110 }, { 197, 110 } },
    { { 204, 62 }, { 197, 62 }, { 197, 62 } },
    { { 208, 168 }, { 180, 168 }, { 180, 134 } },
    { { 262, 88 }, { 262, 91 }, { 197, 91 } },
    { { 112, 168 }, { 140, 168 }, { 140, 134 } }
};
static const int CB_L_DN = 0;   /* hub6: decor stub would cross MIX */
static const int8_t CB_L_DL[4] = { 2, 0, 0, 0 };
static const int16_t CB_L_DT[4][6][2] = { { { 80, 50 }, { 123, 50 } }, { { 0, 0 } }, { { 0, 0 } }, { { 0, 0 } } };
