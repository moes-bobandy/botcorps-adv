/* Simulated XPT2046 panel for host tests: raw = affine(screen x,y) + uniform noise (+-noise raw counts).
 * Not real hardware data. Used by sim_live (CALbot renders) and cal_accuracy (cal math check). */
#pragma once
#include <stdint.h>
struct TouchModel {
    const char *name;
    double ax, bx, cx, ay, by, cy;   /* rx = ax + bx*sx + cx*sy ; ry = ay + by*sx + cy*sy */
    int noise;
    uint32_t seed;
    int rnd(void) { seed = seed * 1664525u + 1013904223u; return noise ? (int)((seed >> 8) % (uint32_t)(2 * noise + 1)) - noise : 0; }
    void raw(double sx, double sy, int *rx, int *ry, bool noisy = true)
    {
        double x = ax + bx * sx + cx * sy, y = ay + by * sx + cy * sy;
        if (noisy) { x += rnd(); y += rnd(); }
        *rx = x < 0 ? 0 : x > 4095 ? 4095 : (int)(x + 0.5); *ry = y < 0 ? 0 : y > 4095 ? 4095 : (int)(y + 0.5);
    }
};
