#define LV_HOST 1
#include "lv_draw.h"
#include "themes_spec_v2.h"
#include <cstdio>
#include <cstdlib>
#include <vector>
/* usage: reink theme base.rgb role.u8 out.rgb samp.i16  (role 7 = badge fill: ONE ink per badge, sampled at samp (x,y));   (240x320 rgb24; roles: 0 keep 1 face 2 frame-hue 3 hdr 4 acc 5 accd 6 text-on-acc) */
int main(int argc, char **argv)
{
    if (argc != 6) return 2;
    const int t = atoi(argv[1]); const int N = 240 * 320;
    std::vector<uint8_t> rgb(N * 3), role(N);
    FILE *f = fopen(argv[2], "rb"); if (!f || fread(rgb.data(), 1, N * 3, f) != (size_t)N * 3) return 3; fclose(f);
    f = fopen(argv[3], "rb"); if (!f || fread(role.data(), 1, N, f) != (size_t)N) return 4; fclose(f);
    std::vector<int16_t> samp(N * 2);
    f = fopen(argv[5], "rb"); if (!f || fread(samp.data(), 2, N * 2, f) != (size_t)N * 2) return 5; fclose(f);
    for (int i = 0; i < N; ++i) {
        const int x = i % 240, y = i / 240; uint16_t c = lv_rgb(rgb[i * 3], rgb[i * 3 + 1], rgb[i * 3 + 2]), o = c;
        switch (role[i]) {
        case 1: { int fx, fy; sec_face_map(x, y, &fx, &fy); o = sec_multi_ink(t, c, fx, fy); break; }
        case 2: { const int R = (c >> 11) << 3, Bl = (c & 31) << 3, mx = R > Bl ? R : Bl; const uint32_t T = sec_theme_at(t, x, y);
                  o = lv_rgb((uint8_t)(((T >> 16) & 255) * mx / 255), (uint8_t)(((T >> 8) & 255) * mx / 255), (uint8_t)((T & 255) * mx / 255)); break; }  /* LV_TM_HUE */
        case 3: o = sec_sentinel(t, lv_hex(SEC_HDR[t]), x, y); break;
        case 4: o = sec_sentinel(t, lv_hex(SEC_ACC[t]), x, y); break;
        case 5: o = sec_sentinel(t, lv_hex(SEC_ACCD[t]), x, y); break;
        case 6: o = lv_hex(0x080808); break;
        case 7: o = sec_sentinel(t, lv_hex(SEC_ACC[t]), samp[i * 2], samp[i * 2 + 1]); break;
        default: break;
        }
        if (!role[i]) continue;
        rgb[i * 3] = (uint8_t)((o >> 11) << 3); rgb[i * 3 + 1] = (uint8_t)(((o >> 5) & 63) << 2); rgb[i * 3 + 2] = (uint8_t)((o & 31) << 3);
    }
    f = fopen(argv[4], "wb"); fwrite(rgb.data(), 1, N * 3, f); fclose(f);
    return 0;
}
