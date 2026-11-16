/* ticker: stripes + scrolling bold text over the whole top bar */
struct LvTickCtx { int span0, span1; };
static void lv_scene_ticker(LvBand &B, void *vctx)
{
    LvTickCtx *c = (LvTickCtx *)vctx;
    if (lv_hunt_bar_on()) { lv_hunt_bar(B); return; }   /* hunting: the banner stops scrolling and becomes the hunt bar */
    const bool flash = lv_tk_alert && ((lv_now / 350) & 1);
    const uint16_t bg = lv_tk_alert ? (flash ? lv_hex(0x6a0000) : lv_hex(0x3a0000)) : lv_hex(0x2a0400);
    b_fill(B, B.x0, B.y0, B.w, B.h, bg);
    const int ph = (int)(lv_now / 40);
    for (int y = B.y0; y < B.y0 + B.h; ++y) {
        if (!(y <= 3 || y >= 21)) continue;
        for (int x = B.x0; x < B.x0 + B.w; ++x) {
            const bool red = (((x + (y <= 3 ? y : -y) + ph) / 5) & 1);
            b_pix(B, x, y, red ? lv_hex(flash ? 0xff2020 : 0xe00000) : lv_hex(0xffb000));
        }
    }
    const int tx = c->span1 + 1 - lv_tk_off;
    const uint16_t ink = lv_tk_alert ? lv_hex(0xffffff) : lv_hex(0xffc830);
    b_text(B, tx + 1, 6, lv_tk_msg, lv_hex(0x200000), 2);
    b_text(B, tx, 5, lv_tk_msg, ink, 2);
}
