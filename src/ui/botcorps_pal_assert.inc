constexpr uint8_t botcorps_pal_ids[] = {
    PAL_BLACK, PAL_WHITE, PAL_FIELD, PAL_BG, PAL_BAR, PAL_LINE, PAL_INK, PAL_MUTED, PAL_AMBER, PAL_SEL, PAL_DEEP, PAL_IVORY, PAL_OK, PAL_HOLD, PAL_CYAN, PAL_MINT, PAL_PLATE, PAL_BOARD_L, PAL_BOARD_D, PAL_EYE, PAL_CODE, PAL_HEAD, PAL_B5D3F, PAL_B4E7F, PAL_B7711, PAL_B9F0B, PAL_BFB72, PAL_BAC7F, PAL_B36B8, PAL_BFC6B, PAL_BBD3F, PAL_DIAG, PAL_BOT_BACK, PAL_R_FDC9, PAL_R_F689, PAL_R_FA69, PAL_R_E2FF, PAL_R_CF89, PAL_R_FC43, PAL_R_6BBF, PAL_SOLID_G, PAL_SOLID_C, PAL_SOLID_B, PAL_SOLID_P, PAL_SOLID_A, PAL_DEV, PAL_DEV2, PAL_ICON_C3, PAL_STEEL, PAL_CHIN
};
constexpr bool botcorps_pal_ids_ok(void)
{
    const int n = (int)(sizeof(botcorps_pal_ids) / sizeof(botcorps_pal_ids[0]));
    for (int i = 0; i < n; ++i) {
        const uint8_t id = botcorps_pal_ids[i];
        if ((id == 0 && i != 0) || (id == 255 && i != 1)) return false;
    }
    return PAL_BLACK == 0 && PAL_WHITE == 255 && PAL_RAMP_N == 4
        && PAL_FIELD != 0 && PAL_FIELD != 255
        && PAL_SOLID_G != 0 && PAL_SOLID_C != 0 && PAL_SOLID_B != 0;
}
