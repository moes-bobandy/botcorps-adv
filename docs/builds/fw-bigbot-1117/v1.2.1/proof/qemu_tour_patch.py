import sys
p=sys.argv[1]+'/src/live/lv_ui.cpp'
s=open(p).read()
s=s.replace('    const bool cal_ok = botcorps_cyd_has_valid_cal();','''#ifdef LV_QEMU_TOUR
    const bool cal_ok = true;   /* QEMU heap tour only: no XPT2046 / NVS cal in the emulator */
#else
    const bool cal_ok = botcorps_cyd_has_valid_cal();
#endif''',1)
s=s.replace('void lv_ui_loop(void)\n{','''#ifdef LV_QEMU_TOUR
static void qemu_tour(uint32_t now)
{
    static int st = 0; static uint32_t t0 = 0; if (!t0) t0 = now;
    const uint32_t el = now - t0;
    struct Step { uint32_t at; int what; } static const S[] = {
        {6000, 0}, {7000, 1}, {8000, 3}, {9000, 4}, {10000, 2}, {11000, 10}, {12000, 0}, {13000, 11}, {17000, 12}, {19000, 3}, {20000, 13}, {24000, 12}, {26000, 14}, {34000, 99}};
    while (st < (int)(sizeof S / sizeof S[0]) && el >= S[st].at) {
        const int w = S[st].what; char t[24];
        if (w >= 0 && w <= 4) { lv_go(LV_SC_HOME); lv_pg_set_page(w); snprintf(t, sizeof t, "tour page %d", w); }
        else if (w == 10) { lv_cb_panel_dirty = true; snprintf(t, sizeof t, "tour cb tap"); }
        else if (w == 11) { lv_go(LV_SC_MO); snprintf(t, sizeof t, "tour MO"); }
        else if (w == 12) { lv_go(LV_SC_HOME); snprintf(t, sizeof t, "tour HOME"); }
        else if (w == 13) { lv_go(LV_SC_DL); snprintf(t, sizeof t, "tour DL"); }
        else if (w == 14) { lv_job_start(LV_J_WIFI, -1); snprintf(t, sizeof t, "tour WIFI scan"); }
        else snprintf(t, sizeof t, "tour end");
        heap_log(t); ++st;
    }
    static uint32_t lastlog = 0; if (st > 0 && now - lastlog >= 1000) { lastlog = now; heap_log("tour tick"); }
}
#endif
void lv_ui_loop(void)
{
#ifdef LV_QEMU_TOUR
    qemu_tour(millis());
#endif''',1)
open(p,'w').write(s)
