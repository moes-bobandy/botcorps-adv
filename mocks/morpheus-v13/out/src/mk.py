from pathlib import Path
L='/workspace/cyd-morpheus-v6-wt/src/live/'
s=Path(L+'lv_morpheus_v12.h').read_text()
s=s[:s.index('/* MOCK')]+'''/* MOCK 2026-11-13 v13 - MORPHEUSbot "DOJO CONSTRUCT" (built from v12; v12 / v11 kept on disk, not included).
 * Mobandy 2026-11-13: v12 rejected ("close-up pillar looks wrong", "folders don't look like the download game", "make the damn definition
 * better", "the towers look like shit ... the idle room towers too, redo them", no lock bracket / select effect, red pill removed).
 * - Engine motion unchanged: lv_rd_layout / lv_rd_place / mo11_drift tables, camera 0.5 sin(0.45 t), room, rain, chrome.
 * - NEW SKIN (mo11_tower_fx, every pillar, idle + close-up): opaque lacquer monolith, lit top + perspective side face from the
 *   lv_rd_place back corners, crisp 1 px parchment / timber / green edges, recessed icon plate with an integer-scaled (8/16/32/48 px)
 *   nearest-neighbour device icon, shoji window cassettes (lit paper / dark / Matrix-green panes) on the front + perspective ribs on the side.
 * - TAP: no pill, flood or brackets; straight into the push-in (400 ms). Search bar on top of the tower; 3 DOWNLOAD-structured Dojo
 *   folders (SCROLL n tab, icon + big file name, big size, 70x7 bar + action line; DROP0 300, GAP 110, DROP 380 ms). EX.DATA only. */
'''+s[s.index('#if LV_MO_INSIDE'):]
R=lambda a,b: (a in s) or (_ for _ in ()).throw(SystemExit('missing: '+a[:60]))
def rep(a,b):
    global s; R(a,b); s=s.replace(a,b)
rep('#define MO12_PUSH0 600','#define MO12_PUSH0 0'); rep('#define MO12_PUSH_MS 560','#define MO12_PUSH_MS 400')
rep('#define MO12_BAR0 1000','#define MO12_BAR0 120'); rep('#define MO12_DROP0 1050','#define MO12_DROP0 300')
rep('MO12_FX = 106, MO12_FW = 128, MO12_FY0 = 58, MO12_FDY = 47, MO12_FH = 36','MO12_FX = 92, MO12_FW = 143, MO12_FY0 = 60, MO12_FDY = 62, MO12_FH = 46')
rep('"%d/4 SCROLLS"','"%d/3 SCROLLS"'); rep('  4 SCROLLS','  3 SCROLLS'); rep('  DRAFT TEXT','  EX.DATA')
a=s.index('static void mo12_body('); b=s.index('/* ---- v12 Dojo folders',a)
s=s[:a]+Path('/tmp/morpheus_v13/skin.inc').read_text()+s[b:]
rep('mo_brackets(B, T, mo_ease(el / 250.0f));','/* v13: no select effect (Mobandy: no lock bracket) */')
rep('if (el < MO_PILL_MS + 200) mo_pill(B, (T.x0 + T.x1) / 2, T.yt, el > MO_PILL_MS ? 1.f : el / (float)MO_PILL_MS);','/* v13: red pill removed */')
rep('static float mo_flood(void) { if (itw_mo_sel < 0) return 0; const uint32_t el = lv_now - itw_mo_t0; float f = el > MO_PILL_MS ? (el - MO_PILL_MS) / (float)MO_FLOOD_MS : 0; return f > 1 ? 1 : f; }','static float mo_flood(void) { return 0; }   /* v13: no green flood */')
rep('for (int j = 0; j < 4; ++j) { const int tt','for (int j = 0; j < 3; ++j) { const int tt'); rep('for (int j = 3; j >= 0; --j) { const int tt','for (int j = 2; j >= 0; --j) { const int tt')
rep('for (int j = 0; j < 4; ++j) printf("%d/%d%s", mo12_fys[j], mo12_fph100[j], j < 3 ? "," : "\\n");','for (int j = 0; j < 3; ++j) printf("%d/%d%s", mo12_fys[j], mo12_fph100[j], j < 2 ? "," : "\\n");')
rep('itw_mo_phase = el < MO_PILL_MS ? 1 : el < MO12_DROP0 + 3 * MO12_GAP + MO12_DROP_MS ? 2 : 3;','itw_mo_phase = el < MO12_PUSH_MS ? 1 : el < MO12_DROP0 + 2 * MO12_GAP + MO12_DROP_MS ? 2 : 3;')
a=s.index('    b_text35(B, x + 5, y - 6, MO12_TAB[j]'); b=s.index('\n}',a)
s=s[:a]+Path('/tmp/morpheus_v13/fold.inc').read_text()+s[b:]
rep('h = MO12_FH, tw = lv_w35(MO12_TAB[j]) + 12','h = MO12_FH, tw = 60')
rep('#define MO12_ZC 0.60f','#define MO12_ZC 0.72f'); rep('#define MO12_SX 46.0f','#define MO12_SX 38.0f')
rep('const int xl = MO12_FX - 4,','const int xl = MO12_FX - 3,')
rep('snprintf(a, sizeof a, "RED PILL TAKEN  EX.DATA");','snprintf(a, sizeof a, "PILLAR OPEN  EX.DATA");')
rep('"TAP A PILLAR = TAKE THE RED PILL"','"TAP A PILLAR = OPEN IT"')
rep('    if (solid) mo12_rect(B, x + 3, y + 2, w, h, lip, 236);   /* 3D lip block */',
 '    if (solid) { mo12_rect(B, x + 6, y + 5, w, h, lv_hex(0x1e1a16), 110); mo12_rect(B, x + 2, y - 6, 60, 4, lv_hex(0x1e1a16), 70); }   /* v13.1 drop shadow */\n    if (solid) mo12_rect(B, x + 3, y + 2, w, h, lip, 236);   /* 3D lip block */')
rep('    if (!solid) return;\n    mo12_rect(B, x + 1, y + 2, 2, h - 4, g, 230);','    if (!solid) return;\n    { const uint16_t ol = lv_hex(0x1e1a16);   /* v13.1 ink outline (top / left / tab), separates the folder from the parchment wall */\n      mo12_line(B, x - 1, y - 9, x + tw - 4, y - 9, ol); mo12_line(B, x + tw - 4, y - 9, x + tw + 1, y - 4, ol); mo12_line(B, x + tw + 1, y - 4, x + tw + 1, y - 1, ol);\n      mo12_line(B, x + tw + 1, y - 1, x + w, y - 1, ol); mo12_line(B, x - 1, y - 9, x - 1, y + h, ol); mo12_line(B, x - 1, y + h, x + 3, y + h, ol); }\n    mo12_rect(B, x + 1, y + 2, 2, h - 4, g, 230);')
rep('mo12_line(B, x + w, y + 2, x + w + 3, y + 5, tim); mo12_line(B, x + w + 3, y + 5, x + w + 3, y + h + 1, tim); mo12_line(B, x + 3, y + h + 1, x + w + 3, y + h + 1, tim);','{ const uint16_t ol = lv_hex(0x1e1a16); mo12_line(B, x + w, y + 2, x + w + 3, y + 5, ol); mo12_line(B, x + w + 3, y + 5, x + w + 3, y + h + 1, ol); mo12_line(B, x + 3, y + h + 1, x + w + 3, y + h + 1, ol); }')
Path(L+'lv_morpheus_v13.h').write_text(s)
p=Path(L+'lv_inside_towers.h'); t=p.read_text(); assert ('lv_morpheus_v12.h' in t) or ('#include "lv_morpheus_v13.h"' in t)
p.write_text(t.replace('#include "lv_morpheus_v12.h"','#include "lv_morpheus_v13.h"'))
print('ok', len(s))
