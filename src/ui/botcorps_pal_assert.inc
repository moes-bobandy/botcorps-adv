constexpr uint8_t botcorps_pal_ids[] = {
    PAL_BLACK, PAL_WHITE, PAL_FIELD, PAL_BG, PAL_BAR, PAL_LINE, PAL_INK, PAL_MUTED, PAL_AMBER, PAL_SEL, PAL_DEEP, PAL_IVORY, PAL_OK, PAL_HOLD, PAL_CYAN, PAL_MINT, PAL_PLATE, PAL_BOARD_L, PAL_BOARD_D, PAL_EYE, PAL_CODE, PAL_HEAD, PAL_B5D3F, PAL_B4E7F, PAL_B7711, PAL_B9F0B, PAL_BFB72, PAL_BAC7F, PAL_B36B8, PAL_BFC6B, PAL_BBD3F,     PAL_DIAG, PAL_BOT_BACK, PAL_R_FDC9, PAL_R_F689, PAL_R_FA69, PAL_R_E2FF, PAL_R_CF89, PAL_R_FC43, PAL_R_6BBF, PAL_SOLID_G, PAL_SOLID_C, PAL_SOLID_B, PAL_SOLID_P, PAL_SOLID_A, PAL_DEV, PAL_DEV2, PAL_ICON_C3, PAL_STEEL, PAL_CHIN, PAL_WIRE_C, PAL_WIRE_A, PAL_WIRE_M, PAL_SOFT, PAL_WASH_C, PAL_WASH_A, PAL_WASH_M,     PAL_HUB_HEAD, PAL_HUB_IVORY,
    PAL_TERM_INK_C, PAL_TERM_INK_A, PAL_TERM_INK_M, PAL_TERM_INK,
    PAL_TERM_M, PAL_TERM_M28, PAL_TERM_M22, PAL_TERM_M18, PAL_TERM_M35, PAL_TERM_M10,
    PAL_TERM_C, PAL_TERM_C28, PAL_TERM_C22, PAL_TERM_C18, PAL_TERM_C35, PAL_TERM_C10,
    PAL_CREAM
};
constexpr bool botcorps_pal_ids_ok(void)
{
    const int n = (int)(sizeof(botcorps_pal_ids) / sizeof(botcorps_pal_ids[0]));
    for (int i = 0; i < n; ++i) {
        const uint8_t id = botcorps_pal_ids[i];
        if ((id == 0 && i != 0) || (id == 255 && i != 1)) return false;
        for (int j = 0; j < i; ++j) {
            if (botcorps_pal_ids[j] == id) return false;
        }
    }
    if (PAL_BG == PAL_EYE || PAL_EYE == PAL_BOT_BACK || PAL_BG == PAL_BOT_BACK) return false;
    if (PAL_BAR == PAL_SEL || PAL_SEL == PAL_PLATE || PAL_BAR == PAL_PLATE) return false;
    if (PAL_FIELD == PAL_BOT_BACK || PAL_FIELD == 0 || PAL_FIELD == 255) return false;
    if (PAL_CYAN == PAL_B5D3F) return false;
    if (PAL_CYAN == PAL_B4E7F) return false;
    if (PAL_CYAN == PAL_B7711) return false;
    if (PAL_CYAN == PAL_B9F0B) return false;
    if (PAL_CYAN == PAL_BFB72) return false;
    if (PAL_CYAN == PAL_BAC7F) return false;
    if (PAL_CYAN == PAL_B36B8) return false;
    if (PAL_CYAN == PAL_BFC6B) return false;
    if (PAL_CYAN == PAL_BBD3F) return false;
    return PAL_BLACK == 0 && PAL_WHITE == 255 && PAL_RAMP_N == 4;
}
