/* Exact natives live in the slots below. Index 0 and 255 are not reserved. */
constexpr uint8_t k_is_res[256] = {
    0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    1, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 0, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    1, 1, 0, 0, 1, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0,
};
constexpr uint16_t k_exact_nat[256] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0041, 0x10C4, 0x0000, 0x0000,
    0x0862, 0x128C, 0x0000, 0x0000, 0x02C2, 0x0AC9, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0061, 0x2087, 0x0000, 0x0000, 0x0080, 0x1105, 0x0000, 0x0000,
    0x1082, 0x29C8, 0x21F4, 0x0000, 0x0000, 0x1B29, 0x0000, 0x331F,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x26E8, 0x36B8, 0x269C, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0841, 0x0000, 0x0000, 0x0000, 0x18C3, 0x592C, 0x0000, 0x0000,
    0x5AA2, 0x4A69, 0x0000, 0x0000, 0x4B02, 0x4B27, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5D3F,
    0x0000, 0x0000, 0x0000, 0x4E7F, 0x0000, 0x0000, 0x5FF8, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x60E2, 0x0000, 0x7154, 0x0000,
    0x6242, 0x0000, 0x0000, 0x0000, 0x0000, 0x5B29, 0x6BD0, 0x6BBF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7E6C, 0x7711, 0x4E7B, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x8CD4, 0x0000, 0x0000, 0x9E6E, 0xA534, 0x0000,
    0x0000, 0x9F0B, 0x0000, 0x0000, 0x0000, 0x0000, 0x8FF3, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xB21F, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xAC7F, 0x0000, 0x0000, 0xC638, 0xBD3F,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE38E, 0x0000, 0xE2FF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xF5A9, 0xCE16, 0x0000,
    0x0000, 0x0000, 0xD69A, 0x0000, 0x0000, 0xCF89, 0x0000, 0xE77E,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xFA69, 0x0000, 0x0000, 0x0000, 0x0000, 0xFB72, 0x0000,
    0xFC43, 0xFC6B, 0x0000, 0x0000, 0xF586, 0xFDC9, 0x0000, 0x0000,
    0x0000, 0xF689, 0x0000, 0xF79C, 0x0000, 0x0000, 0xEFF9, 0x0000,
};
static const uint16_t k_ramp_src[] = {
    0xEFF9, 0x7E6C, 0x9E6E, 0x5D3F, 0x4E7F, 0x7711, 0x9F0B, 0xFB72, 0xAC7F, 0x36B8, 0xFC6B, 0xBD3F, 0xFDC9, 0xF689, 0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8, 0x26E8, 0x269C, 0x331F, 0xB21F, 0xF586, 0xFFFF, 0x4B27, 0xC638, 0x1364, 0x1485, 0x134E, 0x1452, 0x198F, 0x21F4, 0x590F, 0x7154, 0x7AC3, 0xA3A4, 0x7BEF, 0xA534
};
constexpr uint8_t k_ramp_idx[][PAL_RAMP_N] = {
    { 254, 109, 68, 36 }, /* head */
    { 121, 76, 36, 36 }, /* code */
    { 149, 76, 36, 36 }, /* dev */
    { 87, 9, 37, 32 }, /* bot5D3F */
    { 91, 9, 37, 32 }, /* bot4E7F */
    { 122, 45, 36, 36 }, /* bot7711 */
    { 153, 76, 36, 36 }, /* bot9F0B */
    { 238, 101, 68, 36 }, /* botFB72 */
    { 179, 41, 37, 32 }, /* botAC7F */
    { 58, 13, 5, 36 }, /* bot36B8 */
    { 241, 104, 68, 36 }, /* botFC6B */
    { 183, 73, 37, 32 }, /* botBD3F */
    { 245, 104, 68, 36 }, /* mixFDC9 */
    { 249, 72, 68, 36 }, /* mixF689 */
    { 233, 100, 40, 36 }, /* mixFA69 */
    { 207, 69, 33, 32 }, /* mixE2FF */
    { 221, 76, 68, 36 }, /* mixCF89 */
    { 251, 109, 37, 36 }, /* mixF79C */
    { 240, 104, 68, 36 }, /* mixFC43 */
    { 111, 41, 5, 32 }, /* mix6BBF */
    { 94, 45, 37, 36 }, /* mix5FF8 */
    { 57, 12, 36, 36 }, /* solidG */
    { 59, 13, 37, 36 }, /* solidC */
    { 47, 9, 5, 32 }, /* solidB */
    { 171, 69, 5, 32 }, /* solidP */
    { 244, 104, 68, 36 }, /* solidA */
    { 255, 109, 41, 36 }, /* solidW */
    { 77, 68, 36, 36 }, /* dev2 */
    { 182, 73, 68, 36 }, /* c3 */
    { 12, 36, 36, 36 }, /* icon2_0 */
    { 16, 12, 36, 36 }, /* codeD_0 */
    { 9, 37, 36, 36 }, /* icon2_1 */
    { 18, 37, 8, 36 }, /* codeD_1 */
    { 9, 5, 32, 36 }, /* icon2_2 */
    { 42, 37, 8, 36 }, /* codeD_2 */
    { 69, 5, 32, 36 }, /* icon2_3 */
    { 102, 33, 8, 36 }, /* codeD_3 */
    { 104, 68, 36, 36 }, /* icon2_4 */
    { 172, 68, 36, 36 }, /* codeD_4 */
    { 110, 41, 36, 36 }, /* icon2_5 */
    { 150, 41, 40, 36 }, /* codeD_5 */
};
static const int k_ramp_n = (int)(sizeof(k_ramp_src) / sizeof(k_ramp_src[0]));
static_assert(sizeof(k_ramp_idx) / sizeof(k_ramp_idx[0]) ==
                  sizeof(k_ramp_src) / sizeof(k_ramp_src[0]), "ramp rows");
static_assert(k_is_res[0] == 0 && k_is_res[255] == 0, "0 and 255 stay natural");
static_assert(k_is_res[PAL_FIELD] == 1 && k_exact_nat[PAL_FIELD] == 0x0041, "PAL_FIELD");
static_assert(k_is_res[PAL_BOT_BACK] == 1 && k_exact_nat[PAL_BOT_BACK] == 0x0061, "PAL_BOT_BACK");
static_assert(k_is_res[PAL_BG] == 1 && k_exact_nat[PAL_BG] == 0x0862, "PAL_BG");
static_assert(k_is_res[PAL_EYE] == 1 && k_exact_nat[PAL_EYE] == 0x0080, "PAL_EYE");
static_assert(k_is_res[PAL_BAR] == 1 && k_exact_nat[PAL_BAR] == 0x10C4, "PAL_BAR");
static_assert(k_is_res[PAL_SEL] == 1 && k_exact_nat[PAL_SEL] == 0x1105, "PAL_SEL");
static_assert(k_is_res[PAL_PLATE] == 1 && k_exact_nat[PAL_PLATE] == 0x1082, "PAL_PLATE");
static_assert(k_is_res[PAL_HOLD] == 1 && k_exact_nat[PAL_HOLD] == 0xE38E, "PAL_HOLD");
static_assert(k_is_res[PAL_CYAN] == 1 && k_exact_nat[PAL_CYAN] == 0x4E7B, "PAL_CYAN");
static_assert(k_is_res[PAL_BOARD_L] == 1 && k_exact_nat[PAL_BOARD_L] == 0xD69A, "PAL_BOARD_L");
static_assert(k_is_res[PAL_STEEL] == 1 && k_exact_nat[PAL_STEEL] == 0x6BD0, "PAL_STEEL");
static_assert(k_is_res[PAL_DEEP] == 1 && k_exact_nat[PAL_DEEP] == 0x0841, "PAL_DEEP");
static_assert(k_is_res[PAL_AMBER] == 1 && k_exact_nat[PAL_AMBER] == 0xF5A9, "PAL_AMBER");
static_assert(k_is_res[PAL_HEAD] == 1 && k_exact_nat[PAL_HEAD] == 0xEFF9, "PAL_HEAD");
static_assert(k_is_res[PAL_CODE] == 1 && k_exact_nat[PAL_CODE] == 0x7E6C, "PAL_CODE");
static_assert(k_is_res[PAL_DEV] == 1 && k_exact_nat[PAL_DEV] == 0x9E6E, "PAL_DEV");
static_assert(k_is_res[PAL_SOLID_G] == 1 && k_exact_nat[PAL_SOLID_G] == 0x26E8, "PAL_SOLID_G");
static_assert(k_is_res[PAL_SOLID_C] == 1 && k_exact_nat[PAL_SOLID_C] == 0x269C, "PAL_SOLID_C");
static_assert(k_is_res[PAL_SOLID_B] == 1 && k_exact_nat[PAL_SOLID_B] == 0x331F, "PAL_SOLID_B");
static_assert(k_is_res[PAL_SOLID_P] == 1 && k_exact_nat[PAL_SOLID_P] == 0xB21F, "PAL_SOLID_P");
static_assert(k_is_res[PAL_SOLID_A] == 1 && k_exact_nat[PAL_SOLID_A] == 0xF586, "PAL_SOLID_A");
static_assert(((0x0061 >> 5) & 63) == (((0x0041 >> 5) & 63) + 1), "bot back is one green step off field");
