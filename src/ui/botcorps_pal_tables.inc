/* Nearest-code lut. Reserved entries store the exact native.
 * Index 0 and 255 are not reserved: they stay RGB332 black and white.
 * k_ramp_idx reuses a nearer kept code when a fade step has no exact slot. */
constexpr uint8_t k_is_res[256] = {
    0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
    1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0,
    0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 1, 0,
    0, 0, 0, 0, 1, 1, 1, 0, 1, 0, 0, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 1, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0,
    0, 0, 0, 1, 0, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
    0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 0, 1,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0,
    1, 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 1, 0, 0, 1, 0,
};
constexpr uint16_t k_exact_nat[256] = {
    0x0000, 0x0000, 0x0000, 0x0000, 0x0061, 0x094C, 0x0000, 0x0000,
    0x02C2, 0x128C, 0x0000, 0x0000, 0x1364, 0x134E, 0x0000, 0x0000,
    0x1485, 0x0000, 0x1452, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0041, 0x2087, 0x0000, 0x0000, 0x18C3, 0x198F, 0x0000, 0x0000,
    0x1980, 0x29C8, 0x21F4, 0x0000, 0x0000, 0x1B29, 0x0000, 0x331F,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x26E8, 0x36B8, 0x269C, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x61A0, 0x590F, 0x0000, 0x0000,
    0x6242, 0x4A69, 0x0000, 0x0000, 0x4B02, 0x4B27, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x5D3F,
    0x0000, 0x0000, 0x0000, 0x4E7F, 0x0000, 0x0000, 0x5FF8, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x60E2, 0x592C, 0x7154, 0x0000,
    0x7AC3, 0x0000, 0x0000, 0x0000, 0x0000, 0x7BEF, 0x0000, 0x6BBF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x7E6C, 0x7711, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x8CD4, 0x0000, 0x0000, 0x9E6E, 0x0000, 0x0000,
    0x0000, 0x9F0B, 0x0000, 0x0000, 0x0000, 0x0000, 0x8FF3, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xB21F, 0xA3A4, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0xAC7F, 0x0000, 0x0000, 0xA534, 0xBD3F,
    0x0000, 0x0000, 0xC638, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xE2FF,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xF586, 0xCE16, 0x0000,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0xCF89, 0x0000, 0xE77E,
    0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
    0x0000, 0xFA69, 0x0000, 0x0000, 0x0000, 0x0000, 0xFB72, 0x0000,
    0xFC43, 0xFC6B, 0x0000, 0x0000, 0x0000, 0xFDC9, 0x0000, 0x0000,
    0x0000, 0xF689, 0x0000, 0xF79C, 0x0000, 0x0000, 0xEFF9, 0x0000,
};
static const uint16_t k_ramp_src[] = {
    0xEFF9, 0x7E6C, 0x9E6E, 0x5D3F, 0x4E7F, 0x7711, 0x9F0B, 0xFB72, 0xAC7F, 0x36B8, 0xFC6B, 0xBD3F, 0xFDC9, 0xF689, 0xFA69, 0xE2FF, 0xCF89, 0xF79C, 0xFC43, 0x6BBF, 0x5FF8, 0x26E8, 0x269C, 0x331F, 0xB21F, 0xF586, 0xFFFF, 0x4B27, 0xC638, 0x1364, 0x1485, 0x134E, 0x1452, 0x198F, 0x21F4, 0x590F, 0x7154, 0x7AC3, 0xA3A4, 0x7BEF, 0xA534
};
constexpr uint8_t k_ramp_idx[][PAL_RAMP_N] = {
    { 254, 77, 36, 4 }, /* head */
    { 121, 12, 40, 4 }, /* code */
    { 149, 76, 40, 4 }, /* dev */
    { 87, 9, 36, 4 }, /* bot5D3F */
    { 91, 9, 36, 4 }, /* bot4E7F */
    { 122, 12, 40, 4 }, /* bot7711 */
    { 153, 76, 40, 4 }, /* bot9F0B */
    { 238, 100, 36, 4 }, /* botFB72 */
    { 179, 41, 36, 4 }, /* botAC7F */
    { 58, 45, 36, 4 }, /* bot36B8 */
    { 241, 72, 40, 4 }, /* botFC6B */
    { 183, 73, 36, 4 }, /* botBD3F */
    { 245, 72, 40, 4 }, /* mixFDC9 */
    { 249, 72, 40, 4 }, /* mixF689 */
    { 233, 100, 36, 4 }, /* mixFA69 */
    { 207, 101, 33, 4 }, /* mixE2FF */
    { 221, 76, 40, 4 }, /* mixCF89 */
    { 251, 73, 36, 4 }, /* mixF79C */
    { 240, 68, 36, 4 }, /* mixFC43 */
    { 111, 37, 36, 4 }, /* mix6BBF */
    { 94, 45, 36, 4 }, /* mix5FF8 */
    { 57, 8, 40, 4 }, /* solidG */
    { 59, 9, 36, 4 }, /* solidC */
    { 47, 5, 36, 4 }, /* solidB */
    { 171, 101, 33, 4 }, /* solidP */
    { 213, 72, 40, 4 }, /* solidA */
    { 255, 105, 41, 4 }, /* solidW */
    { 77, 40, 4, 4 }, /* dev2 */
    { 186, 73, 36, 4 }, /* c3 */
    { 12, 40, 4, 4 }, /* icon2_0 */
    { 16, 40, 4, 4 }, /* codeD_0 */
    { 13, 36, 4, 4 }, /* icon2_1 */
    { 18, 36, 4, 4 }, /* codeD_1 */
    { 37, 36, 4, 4 }, /* icon2_2 */
    { 42, 33, 4, 4 }, /* codeD_2 */
    { 69, 33, 4, 4 }, /* icon2_3 */
    { 102, 33, 4, 4 }, /* codeD_3 */
    { 104, 40, 4, 4 }, /* icon2_4 */
    { 172, 40, 4, 4 }, /* codeD_4 */
    { 109, 41, 4, 4 }, /* icon2_5 */
    { 182, 41, 36, 4 }, /* codeD_5 */
};
static const int k_ramp_n = (int)(sizeof(k_ramp_src) / sizeof(k_ramp_src[0]));
static_assert(sizeof(k_ramp_idx) / sizeof(k_ramp_idx[0]) == sizeof(k_ramp_src) / sizeof(k_ramp_src[0]),
              "ramp rows match sources");
static_assert(k_is_res[0] == 0 && k_is_res[255] == 0, "0 and 255 stay natural RGB332");
static_assert(k_is_res[PAL_FIELD] == 1 && k_exact_nat[PAL_FIELD] == 0x0041, "PAL_FIELD");
static_assert(k_is_res[PAL_HEAD] == 1 && k_exact_nat[PAL_HEAD] == 0xEFF9, "PAL_HEAD");
static_assert(k_is_res[PAL_CODE] == 1 && k_exact_nat[PAL_CODE] == 0x7E6C, "PAL_CODE");
static_assert(k_is_res[PAL_DEV] == 1 && k_exact_nat[PAL_DEV] == 0x9E6E, "PAL_DEV");
static_assert(k_is_res[PAL_SOLID_G] == 1 && k_exact_nat[PAL_SOLID_G] == 0x26E8, "PAL_SOLID_G");
static_assert(k_is_res[PAL_SOLID_C] == 1 && k_exact_nat[PAL_SOLID_C] == 0x269C, "PAL_SOLID_C");
static_assert(k_is_res[PAL_SOLID_B] == 1 && k_exact_nat[PAL_SOLID_B] == 0x331F, "PAL_SOLID_B");
static_assert(k_is_res[PAL_SOLID_P] == 1 && k_exact_nat[PAL_SOLID_P] == 0xB21F, "PAL_SOLID_P");
static_assert(k_is_res[PAL_SOLID_A] == 1 && k_exact_nat[PAL_SOLID_A] == 0xF586, "PAL_SOLID_A");
static_assert(k_is_res[PAL_DEV2] == 1 && k_exact_nat[PAL_DEV2] == 0x4B27, "PAL_DEV2");
static_assert(k_is_res[PAL_ICON_C3] == 1 && k_exact_nat[PAL_ICON_C3] == 0xC638, "PAL_ICON_C3");

static_assert(k_is_res[PAL_BOT_BACK] == 1 && k_exact_nat[PAL_BOT_BACK] == 0x0061, "PAL_BOT_BACK");
static_assert(k_ramp_idx[21][0] == PAL_SOLID_G, "solid green ramp");
static_assert(k_ramp_idx[22][0] == PAL_SOLID_C, "solid cyan ramp");
static_assert(k_ramp_idx[23][0] == PAL_SOLID_B, "solid blue ramp");
static_assert(k_ramp_idx[24][0] == PAL_SOLID_P, "solid purple ramp");
static_assert(k_ramp_idx[25][0] == PAL_SOLID_A, "solid amber ramp");
static_assert(k_ramp_idx[26][0] == PAL_WHITE, "solid white uses RGB332 white");
static_assert(k_ramp_idx[27][0] == PAL_DEV2, "icon second colour ramp");
static_assert(k_ramp_idx[28][0] == PAL_ICON_C3, "icon light ramp");
