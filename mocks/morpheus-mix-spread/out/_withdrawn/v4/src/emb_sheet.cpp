/* morpheus-mix: dump the vector emblems (body / detail / halo coverage, 16, 32, 40 px) and the OLD 16x16 icon art (as shipped in v13.1),
 * for the old-vs-new sheet and the emblem gates. Output: raw bytes to stdout, layout documented in package.py. */
#include <stdio.h>
#include "mo_emblem.h"
#include "/workspace/cyd-morpheus-v6-wt/src/live/lv_icons.h"
static const int OLD_ICON[10] = {-1, LV_IC_LAPTOP, LV_IC_CCTV, LV_IC_DRONE, LV_IC_AIRTAG, LV_IC_KEYFOB, LV_IC_GLASSES, LV_IC_SKIMMER, LV_IC_FLIPPER, LV_IC_GALAXYTAG};
static const char *const DEA[12] = { "...aaaaaa...", ".aa......aa.", "a..aaaaaa..a", "..aa....aa..", "...a.aa.a...", ".....aa.....",
    ".....aa.....", "........c..c", ".........cc.", ".........cc.", "........c..c", "............" };
#include <stdlib.h>
int main(int argc, char **argv)
{
    if (argc > 1) { const int S = atoi(argv[1]);   /* v4: one size, all 18 (stability gate masks) */
        for (int i = 0; i < MO_EMB_N; ++i) { const MoEmb *E = mo_emb_get(i, S); fwrite(E->body, 1, S * S, stdout); fwrite(E->det, 1, S * S, stdout); fwrite(E->halo, 1, (S + 2) * (S + 2), stdout); }
        return 0; }
    const int SZ[3] = {16, 32, 40};
    for (int i = 0; i < MO_EMB_N; ++i) for (int s = 0; s < 3; ++s) { const MoEmb *E = mo_emb_get(i, SZ[s]); fwrite(E->body, 1, SZ[s] * SZ[s], stdout); fwrite(E->det, 1, SZ[s] * SZ[s], stdout); fwrite(E->halo, 1, (SZ[s] + 2) * (SZ[s] + 2), stdout); }
    for (int i = 0; i < 10; ++i) for (int r = 0; r < 16; ++r) for (int c = 0; c < 16; ++c) { char ch = OLD_ICON[i] < 0 ? DEA[r * 12 / 16][c * 12 / 16] : LV_ICON_ART[OLD_ICON[i]][r][c]; fputc(ch, stdout); }
    return 0;
}
