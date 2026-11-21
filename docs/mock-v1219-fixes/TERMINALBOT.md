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


---

## Layer: LV_PG_TERMINAL_PAGE (default 1) — restore gold page as slot 3

Flag: `LV_PG_TERMINAL_PAGE` in `src/live/pager/lv_pager.h` (default 1; set 0 to drop this layer).

Restore is verbatim from 7db0c6d for page identity, plus two named changes:

1. Face square size/position: already the shared slot `LV_PG_FX` / `LV_PG_FACE_S` = x65..174 y88..197 (110×110 inclusive). Measured on 7db0c6d gold, new gold+rain, and CONTROLbot: all three occupy that exact bbox. The gold metallic bevel (dark right/bottom, champagne highlight) was kept as-is; it can look a hair inset vs CONTROLbot's flush purple rectangle, but the geometric square was not scaled.
2. Pitch-black matrix rain cloned from a98a25b GIBSON: same pal `{0,0,0}` ×3 and `lv_pg_rain_row(..., skip=1, page)`. `lv_pg_keep_feat` keeps the gold-face eye color `(40,24,8)/(41,24,8)` (`lv_pg_gold` of black) on top, same skip=1 law the other faces use. Eye pixel count stayed 968 before and after rain.

Page order with the flag on (7db0c6d): BOTpheus 0, HACKbot 1, GIBSONbot 2, TERMINALbot 3, CONTROLbot 4. Art `{0,2,1,3,-1}`; compose overrides TERMINAL to blob 2 + `lv_pg_gold`. Center-face tap → `lv_go_soon(LV_SC_TERM)` route `pager-terminal` (sim: tap 120,142 at t=9600 → scr=3 after the go). ATK/TRACE gold panel via existing `lv_pg_pan_hackbot(..., gold=1)`.

Pips: 5 pips, `CX0=104`, still centred on x=120 (was 4 pips, `CX0=108`).

Arrow chips (first/last, not HACK/CTL hardcodes): first page has no left chip, last page has no right chip. Because the first page is now BOTpheus (was HACKbot), HACKbot *gains* a left arrow and BOTpheus *loses* it. GIBSON and CONTROLbot arrow chips are unchanged.

### Gate vs branch head (sim_tw / 0ba57dc tree, same t=9600)

Control-strip bbox {1,247,238,319}: **0 px** on HACKbot, BOTpheus, GIBSON, CONTROLbot.

Outside pip band y25–29 and the first/last arrow chip boxes, other pages are **0 px**:

| page | total diff | pips | left arrow {5,32,28,67} | right arrow | outside those masks | strip |
|---|---|---|---|---|---|---|
| GIBSON | 130 | 130 in (102,25)-(137,29) | 0 | 0 | 0 | 0 |
| CONTROLbot | 136 | 136 in (102,25)-(139,29) | 0 | 0 | 0 | 0 |
| HACKbot | 825 | 130 | 695 (gained left chip) | 0 | 0 | 0 |
| BOTpheus | 837 | 142 | 695 (lost left chip) | 0 | 0 | 0 |

New TERMINAL vs 7db0c6d gold: 1666 px, bbox (66,88)-(172,197) — exactly the 1666 pitch-black rain glyphs. Gold metal, bevel, and eyes are otherwise the same.

Proofs: `/workspace/out/fw-bigbot-1117/v1219-fixes/terminalbot-page/` (`REPORT.txt`, `proof/new_terminal@3x.png` sha256 `08076d30ecf265ea2b5dce462bbd983de887c1e2aef3c57683a488eb396756ff`, `proof/terminal_vs_control_side@3x.png` sha256 `c6d122e5a476b6037d5b72e0ef27b9858f2b6ab0f3fd2fe84bdbef02f14c1a2d`).
