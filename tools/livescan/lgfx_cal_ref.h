/* Host copy of the LovyanGFX 1.x touch math the firmware uses, so cal accuracy can be checked off-device.
 * Copied from .pio/libdeps/cyd_livescan/LovyanGFX/src/lgfx/v1/panel/Panel_Device.inl:
 *   Panel_Device::setCalibrate(uint16_t*)  and  Panel_Device::convertRawXY() at rotation 0.
 * CYD config: panel 240x320, touch offset_rotation 0 (no w/h swap), UI rotation 0 (r == 0 path). */
#pragma once
#include <stdint.h>
struct LgfxCalRef {
    float _affine[6];
    void setCalibrate(const uint16_t *parameters, int w = 240, int h = 320)
    {
        uint32_t vect[6] = {0,0,0,0,0,0};
        float mat[3][3] = {{0,0,0},{0,0,0},{0,0,0}};
        float a;
        for ( int i = 0; i < 4; ++i ) {
            int32_t tx = (i & 2) ? (w - 1) : 0;
            int32_t ty = (i & 1) ? (h - 1) : 0;
            int32_t px = parameters[i*2  ];
            int32_t py = parameters[i*2+1];
            a = px * px; mat[0][0] += a;
            a = px * py; mat[0][1] += a; mat[1][0] += a;
            a = px;      mat[0][2] += a; mat[2][0] += a;
            a = py * py; mat[1][1] += a;
            a = py;      mat[1][2] += a; mat[2][1] += a;
            mat[2][2] += 1;
            vect[0] += px * tx; vect[1] += py * tx; vect[2] += tx;
            vect[3] += px * ty; vect[4] += py * ty; vect[5] += ty;
        }
        for ( int k = 0; k < 3; ++k ) {
            float t = mat[k][k];
            for ( int i = 0; i < 3; ++i ) mat[k][i] /= t;
            mat[k][k] = 1 / t;
            for ( int j = 0; j < 3; ++j ) {
                if ( j == k ) continue;
                float u = mat[j][k];
                for ( int i = 0; i < 3; ++i ) {
                    if ( i != k ) mat[j][i] -= mat[k][i] * u;
                    else mat[j][i] = -u / t;
                }
            }
        }
        float v0 = vect[0], v1 = vect[1], v2 = vect[2];
        _affine[0] = mat[0][0] * v0 + mat[0][1] * v1 + mat[0][2] * v2;
        _affine[1] = mat[1][0] * v0 + mat[1][1] * v1 + mat[1][2] * v2;
        _affine[2] = mat[2][0] * v0 + mat[2][1] * v1 + mat[2][2] * v2;
        float v3 = vect[3], v4 = vect[4], v5 = vect[5];
        _affine[3] = mat[0][0] * v3 + mat[0][1] * v4 + mat[0][2] * v5;
        _affine[4] = mat[1][0] * v3 + mat[1][1] * v4 + mat[1][2] * v5;
        _affine[5] = mat[2][0] * v3 + mat[2][1] * v4 + mat[2][2] * v5;
    }
    void convertRawXY(int rx, int ry, int *ox, int *oy) const   /* r == 0 */
    {
        *ox = (int32_t)((_affine[0] * (float)rx + _affine[1] * (float)ry) + _affine[2]);
        *oy = (int32_t)((_affine[3] * (float)rx + _affine[4] * (float)ry) + _affine[5]);
    }
};
