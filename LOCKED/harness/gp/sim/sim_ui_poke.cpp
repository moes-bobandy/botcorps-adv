/* Sim-only: botcorps_ui.cpp compiled in this TU so a script can set hub focus state that the
 * CYD can only reach by keyboard arrows (no keys on the CYD; the sim has no key op).
 * Firmware source is unmodified; this file is not part of any PlatformIO env. */
#include "ui/botcorps_ui.cpp"
void sim_poke_hub_zone(int z) { s_hub_zone = z; s_dirty = true; }
void sim_poke_hterm(int on) { s_hub_term = !!on; s_dirty = true; }
void sim_poke_hubvar(int v) { if (v < 0) v = 0; if (v > 2) v = 2; s_hub5_var = v; s_dirty = true; }
/* Sim-only: run TEXT through the Adv plate ENTER path. */
void sim_poke_cmd(const char *t) {
    int cap = 0; char *b = pt_buf(&cap);
    if (b) snprintf(b, (size_t)cap, "%s", t);
    s_pt_mode = 0; s_cyd_adv_focus = true; s_term_prompt = true;
    ui_dispatch(K_OK, 0);
}
void sim_poke_screen(int id) { s_hub_term = false; s_pt_mode = 0; s_cyd_adv_focus = false; s_cursor = 0; enter_screen(id, SCR_GIBSON); s_dirty = true; }
int sim_peek_screen(void) { return s_screen; }
int sim_peek_plate(void) { return s_cyd_term_plate ? 1 : 0; }
void sim_peek_pt(int *a) { a[0] = s_pt_bx; a[1] = s_pt_by; a[2] = s_pt_bw; a[3] = s_pt_bh; a[4] = s_pt_mode; }
void sim_peek_buf(char *out, int n) {
    int cap = 0; char *b = pt_buf(&cap);
    CydClItems ci; cyd_cl_items(&ci);
    const char *ll = s_log_n > 0 ? log_line(s_log_n - 1) : "";
    snprintf(out, (size_t)n, "mode=%d sym=%d screen=%d sheet=%d/%d focus=%d ovl=%d cmds=%d cl=%d lvl=%d top=%d n=%d fires=%u buf='%s' log='%s'",
             s_pt_mode, s_pt_sym ? 1 : 0, s_screen, s_sheet ? 1 : 0, s_sheet_i,
             s_cyd_adv_focus ? 1 : 0, (int)s_term_ovl, s_focus_cmds ? 1 : 0,
             cyd_cl_on() ? 1 : 0, ci.level, s_cyd_cl_top, ci.n, (unsigned)s_cyd_tap_fires,
             b ? b : "", ll ? ll : "");
}
void sim_poke_cl(int on) { if (on) cyd_cl_open(); else cyd_cl_close(); }
/* SELECT chip grid hit rects: "clrow i x y w h tok" / "clbtn k x y w h" */
void sim_peek_clkeys(void) {
    if (!cyd_cl_on()) { fprintf(stderr, "[sim] clkeys none\n"); return; }
    if (s_pt_bw > 0) cyd_adv_geo(s_pt_bx, s_pt_by, s_pt_bw, s_pt_bh);
    CydClItems c; cyd_cl_items(&c); CydClGeo G; cyd_cl_geo(c, &G);
    const int top = cyd_cl_clamp_top(c, G);
    fprintf(stderr, "[sim] clkeys box %d %d %d %d row_y %d lvl %d n %d top %d vis %d\n", G.bx, G.by, G.bw, G.bh, G.row_y, c.level, c.n, top, G.vis);
    for (int k = 0; k < G.vis; ++k) {
        int x, y, w, h;
        if (!cyd_cl_tok_chip(G, k, &x, &y, &w, &h)) continue;
        fprintf(stderr, "[sim] clrow %d %d %d %d %d %s\n", top + k, x, y, w, h, c.tok[top + k]);
    }
    for (int k = 0; k < CYD_CL_NB; ++k) { int x, y, w, h; cyd_cl_btn(G, k, &x, &y, &w, &h); fprintf(stderr, "[sim] clbtn %d %d %d %d %d\n", k, x, y, w, h); }
}
void sim_peek_keys(void) {
    /* letter OSK removed from the CYD (owner 8:15 AM ET Oct 26): report mode + plate/pads only */
    fprintf(stderr, "[sim] keys none mode=%d (osk removed)\n", s_pt_mode);
    if (s_pt_bw <= 0) return;
    cyd_adv_geo(s_pt_bx, s_pt_by, s_pt_bw, s_pt_bh);
    fprintf(stderr, "[sim] plate %d %d %d %d py=%d cmd=%d atk=%d eqy=%d rety=%d row=%d rowh=%d sheetw=%d\n",
            s_adv_sx, s_adv_sy, s_adv_sw, s_adv_sh, s_adv_py, s_adv_cmd_x, s_adv_atk_x,
            s_adv_btn_y, s_adv_atk_y, -1, CYD_ADV_ROW_H, cyd_adv_sheet_w());
    for (int n = 0; n < 12; ++n) {
        int x, y, ww, ch;
        if (cyd_adv_pal_chip(n, &x, &y, &ww, &ch))
            fprintf(stderr, "[sim] chip %d %s %d %d %d %d\n", n, CMDS[n], x, y, ww, ch);
    }
    { int hx, hy, hw, hh;
      cyd_adv_btn_hit(s_adv_cmd_x, s_adv_btn_y, &hx, &hy, &hw, &hh);
      fprintf(stderr, "[sim] pad eq %d %d %d %d\n", hx, hy, hw, hh);
      cyd_adv_btn_hit(s_adv_gls_x, s_adv_gls_y, &hx, &hy, &hw, &hh);
      fprintf(stderr, "[sim] pad gls %d %d %d %d\n", hx, hy, hw, hh);
      cyd_adv_btn_hit(s_adv_atk_x, s_adv_atk_y, &hx, &hy, &hw, &hh);
      fprintf(stderr, "[sim] pad ret %d %d %d %d\n", hx, hy, hw, hh); }
}
void sim_poke_clr(void) { int cap = 0; char *b = pt_buf(&cap); if (b) b[0] = 0; s_dirty = true; }
void sim_poke_put(const char *t) {
    int cap = 0; char *b = pt_buf(&cap);
    if (b) snprintf(b, (size_t)cap, "%s", t);
    s_cyd_adv_focus = true; s_term_prompt = true; s_dirty = true;
}
int sim_peek_which(int tx, int ty) {
    /* hub4: report TERMINALbot-board flag, hub hit at (tx,ty), theme index, screen */
    hb_layout_sync();
    const int h = (s_screen == SCR_GIBSON) ? hb_hit(tx, ty) : -9;
    fprintf(stderr, "[sim] hub hterm=%d hit=%d color=%d screen=%d cl=%d\n", s_hub_term ? 1 : 0, h, (int)s_color, s_screen, cyd_cl_on() ? 1 : 0);
    return h;
}
void sim_poke_nosheet(void) { s_sheet = false; s_sheet_i = 0; s_dirty = true; }
/* open Adv OSK / sheet / palette / attack for capture scripts */
void sim_poke_osk(int on) { (void)on; fprintf(stderr, "[sim] osk removed\n"); }
void sim_poke_sheet(int on) { s_sheet = !!on; if (on) { s_sheet_i = 0; s_pt_mode = 0; s_focus_cmds = false; } s_dirty = true; }
void sim_poke_palette(int on) { s_focus_cmds = !!on; if (on) { s_sheet = false; s_term_ovl = 0; } s_dirty = true; }
void sim_poke_attack(int on) { s_term_ovl = on ? 2 : 0; if (on) { s_sheet = false; s_focus_cmds = false; } s_dirty = true; }
/* hub8 sim-only: BOOT-inverse proof. GIBSON and BOOT drawn by drawBotChipK at the same scale,
 * same millis, no info line, side by side on a flat key colour; prints rects + the invert map. */
void sim_bootproof(int k16)
{
    Gfx g(botcorps_disp());
    const bool P = botcorps_cyd_orient != 0;
    const uint8_t bg = botcorps_pal_fit(0x07E0);
    g.fillScreen(bg);
    const int w = mb_w(P, k16), h = mb_h(P, k16), gx = 8, bx = 8 + w + 16, y = 8;
    drawBotChipK(g, gx, y, P, MB_GIB, k16, nullptr);
    drawBotChipK(g, bx, y, P, MB_BOOT, k16, nullptr);
    int dcx, dcy, D, ty; mb_geo(P, &dcx, &dcy, &D, &ty, k16, k16 >= 16);
    fprintf(stderr, "[proof] k16 %d P %d bg %u bg565 %04x chipw %d chiph %d gib %d %d boot %d %d nametop %d sw %d gibname %s bootname %s\n",
            k16, P ? 1 : 0, bg, botcorps_rgb565(bg), w, h, gx, y, bx, y, ty, w - 2 * mb_S(3, k16),
            mb_name(MB_GIB, w - 2 * mb_S(3, k16)), mb_name(MB_BOOT, w - 2 * mb_S(3, k16)));
    for (int i = 0; i < 256; ++i) fprintf(stderr, "[ivmap] %d %04x %d %04x\n", i, botcorps_rgb565(i), mb_iv(i), botcorps_rgb565(mb_iv(i)));
}
#include "sim_parts.inc"
/* hub9 sim-only: Defend round peek + field geometry + device screen xy */
void sim_peek_dl(void)
{
    const uint32_t t = millis();
    int fx, fy, fw, fh; cdl_rect(&fx, &fy, &fw, &fh);
    fprintf(stderr, "[sim] dl on=%d live=%d over=%d bot=%d charge=%d left=%d score=%d hi=%d field %d %d %d %d screen=%d\n",
            s_dl_on, dl_live() ? 1 : 0, cdl_over() ? 1 : 0, dl_bot(), dl_charge_at(t), s_dl_on ? dl_time_left(t) : 0,
            s_dl_score, s_dl_hi, fx, fy, fw, fh, s_screen);
    if (cdl_over()) for (int k = 0; k < 2; ++k) { int x, y, w, h; cdl_btn(k, &x, &y, &w, &h); fprintf(stderr, "[sim] dlbtn %d %d %d %d %d\n", k, x, y, w, h); }
    for (int i = 0; i < DL_MAX; ++i) {
        if (!s_dl_d[i].live) continue;
        int ex, ey; dl_xy(i, &ex, &ey); int x, y; cdl_map(ex, ey, &x, &y);
        char nm[16]; cdl_kind_name(s_dl_d[i].kind, nm, sizeof nm);
        fprintf(stderr, "[sim] dldev %d %s %d %d hp=%d\n", i, nm, x, y, s_dl_d[i].hp);
    }
}
int sim_dl_dev_xy(int i, int *x, int *y)
{
    if (i < 0 || i >= DL_MAX || !s_dl_d[i].live) return 0;
    int ex, ey; dl_xy(i, &ex, &ey); cdl_map(ex, ey, x, y);
    return 1;
}
