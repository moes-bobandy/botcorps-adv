# Old gold TERMINALbot bot-screen (no home-layout change)

Last pager page: 7db0c6dcadc31c5a663227b0c3e5499321fdf5d3 (v1.2.5).
Slot LV_PG_TERMINAL = 3 in the 5-page order BOTpheus, HACKbot, GIBSONbot,
TERMINALbot, CONTROLbot (default start GIBSONbot). Tapping the page opened
LV_SC_TERM. Home-board chip 4 LV_HOME_CHIP[4] = {139,156,207,255} still
routes to LV_SC_TERM in current firmware (lv_core.h); not moved.

Look: metallic-gold remapped old HACKbot face (lv_pg_gold) and gold ATK/TRACE
panel (lv_pg_pan_hackbot gold=1). Shared rain was not applied on that page.
Pitch-black rain was added in a98a25bd9a458437f8457714a7238852749ce8ff to
GIBSONbot in the same commit that dropped this page. There is no committed
gold+black-rain TERMINALbot page.

Proof render from 7db0c6d (host sim, page 3):
/workspace/out/fw-bigbot-1117/v1219-fixes/terminalbot-7db0c6d/terminalbot_gold@3x.png
