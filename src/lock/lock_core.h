#pragma once
/*
 * lock_core — portable clip player for the LOCKED screens.
 * Data comes from lock_data.h + lock_blob.bin (tools/lkenc.py).
 * An op is a run of raw-deflate chunks; each inflates to <=16 KB of
 * records { u16 y, u8 x, u8 n, n palette indices }, ended by y=0xffff.
 * Segment k plays ops op0..op0+n-1 (frame 0 is a keyframe). A looping
 * segment then plays op0+n (back to frame 0) and goes on from frame 1.
 * Shared by the firmware (ROM tinfl) and the host sim (zlib).
 */
#include <stdint.h>
#include "lock_data.h"

#define LK_INFL_CAP 16384u

typedef int  (*lk_inflate_fn)(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap);
typedef void (*lk_draw_fn)(int y, int x, int n, const uint8_t *idx);

struct LkIo {
    const uint8_t *blob;
    lk_inflate_fn inflate;
    lk_draw_fn draw;
    uint8_t *buf; /* LK_INFL_CAP bytes */
};

/* Play one op. Returns records drawn, or -1 on a bad chunk. */
static inline int lk_run_op(const LkIo &io, int op)
{
    int recs = 0;
    const uint32_t c0 = LK_OP[op][0], nc = LK_OP[op][1];
    for (uint32_t c = c0; c < c0 + nc; ++c) {
        const int out = io.inflate(io.blob + LK_CHUNK[c][0], LK_CHUNK[c][1], io.buf, LK_INFL_CAP);
        if (out < 4) return -1;
        int p = 0;
        while (p + 4 <= out) {
            const int y = io.buf[p] | (io.buf[p + 1] << 8);
            if (y == 0xffff) break;
            const int x = io.buf[p + 2], n = io.buf[p + 3];
            if (p + 4 + n > out || y >= 320 || x + n > 240) return -1;
            io.draw(y, x, n, io.buf + p + 4);
            p += 4 + n;
            ++recs;
        }
    }
    return recs;
}

static inline int lk_clip_of_seg(int seg)
{
    for (int c = 0; c < (int)(sizeof(LK_CLIP) / sizeof(LK_CLIP[0])); ++c)
        if (seg >= LK_CLIP[c].seg0 && seg < LK_CLIP[c].seg0 + LK_CLIP[c].nseg) return c;
    return 0;
}

/* Segment cursor. frame() is the segment frame index on screen
 * (clip frame = LK_SEG[seg].f0 + frame()). stop >= 0 holds at that frame. */
struct LkPlay {
    int seg = -1;
    int k = 0;       /* next op index within segment, 0..n (n = loop-back op) */
    int shown = -1;  /* frame on screen */
    int stop = -1;
    bool done = false;

    void start(int s, int stop_at = -1) { seg = s; k = 0; shown = -1; stop = stop_at; done = false; }
    /* Advance one frame. Returns true if something was drawn. */
    bool step(const LkIo &io)
    {
        if (seg < 0 || done) return false;
        if (stop >= 0 && shown == stop) return false;
        const LkSeg &S = LK_SEG[seg];
        if (k < S.n) {
            lk_run_op(io, S.op0 + k);
            shown = k++;
            return true;
        }
        if (S.loop) {
            lk_run_op(io, S.op0 + S.n);
            shown = 0;
            k = 1;
            return true;
        }
        done = true;
        return false;
    }
    bool holding(void) const { return done || (stop >= 0 && shown == stop); }
};
