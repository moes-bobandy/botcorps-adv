/* fw-3locks-1116 (Mobandy 2026-11-16 1:17 PM ET: "Build CONTROLbot Color UI v6, BOTpheus EB3 and the HACKbot game into one firmware").
 * Build switches shared by the firmware and the host sim (tools/livescan/sim_live.cpp), so both compile the same screens.
 *  - BOTpheus (was Morpheus) EB3 towers: LOCKED 006aa03 (towers3c/EB3). The EB3 sim was built with exactly these four defines
 *    (towers3c/src/eb3_body.py: -DMO_BODY=5 -DMO_EB2=1 -DMO_EB2_MINFW=10.5f -DMO_EB2_KZ=0.6f).
 *  - HACKbot game: LOCKED cyd-lock-store 3503679 (source lv_scanbot_download.h). Needs LV_HC_GAME + LV_RD_INFO for shared code;
 *    LV_RD_INFO_UI 0 keeps the GIBSON radar's locked 3-button row (no INFO button), so no other locked screen changes.
 *  - The CONTROLbot / TERMINALbot "inside" mocks are NOT part of this build (0). */
#ifndef LV_BUILD_3LOCKS_H
#define LV_BUILD_3LOCKS_H
#ifndef MO_BODY
#define MO_BODY 5
#endif
#ifndef MO_EB2
#define MO_EB2 1
#endif
#ifndef MO_EB2_MINFW
#define MO_EB2_MINFW 10.5f
#endif
#ifndef MO_EB2_KZ
#define MO_EB2_KZ 0.6f
#endif
#ifndef LV_MO_INSIDE
#define LV_MO_INSIDE 1
#endif
#ifndef LV_CT_INSIDE
#define LV_CT_INSIDE 0
#endif
#ifndef LV_TR_INSIDE
#define LV_TR_INSIDE 0
#endif
#ifndef LV_HC_GAME
#define LV_HC_GAME 1
#endif
#ifndef LV_DL_GAME
#define LV_DL_GAME 1
#endif
#ifndef LV_RD_INFO
#define LV_RD_INFO 1
#endif
#ifndef LV_RD_INFO_UI
#define LV_RD_INFO_UI 0
#endif
#ifndef LV_BOTPHEUS_NAME
#define LV_BOTPHEUS_NAME 1   /* 1 = the BOTpheus header label reads "BOTPHEUS" (Mobandy rename); 0 = the locked "MORPHEUS" pixels */
#endif
#ifndef LV_CUI
#define LV_CUI 1             /* CONTROLbot Color UI v6 (24 themes, tinted ticker) */
#endif
#endif
