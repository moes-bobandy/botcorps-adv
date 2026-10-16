/* CYD OSK typing gate. No device, no Arduino, no Cursor.
 * Build: g++ -std=gnu++17 -I src tools/host/cyd_type_test.cpp -o /tmp/cyd_type_test
 *
 * Walks the shared key table the way a tap does (cyd_osk_hit -> cyd_osk_at)
 * and fails if a letter, space, comma, period, or symbol-layer glyph cannot
 * be produced. This is the BOTCORPS_CYD case the UI hook cannot cover,
 * because the host renderer is not built with BOTCORPS_CYD.
 */
#include "ui/botcorps_cyd_osk.h"

#include <stdio.h>
#include <string.h>

static int expect_hit(int x, int y, int want)
{
    const int hit = cyd_osk_hit(x, y);
    if (hit != want) {
        printf("hit (%d,%d) got %d want %d\n", x, y, hit, want);
        return 1;
    }
    return 0;
}

static int collect(char *out, int cap, bool sym)
{
    int n = 0;
    for (int i = 0; i < CYD_OSK_N; ++i) {
        const CydOskKey k = cyd_osk_at(i);
        const int hit = cyd_osk_hit(k.x + k.w / 2, k.y + k.h / 2);
        if (hit != i) {
            printf("key %d center miss hit=%d\n", i, hit);
            return -1;
        }
        const char ch = cyd_osk_char(k, sym);
        if (!ch) continue;
        if (n + 1 >= cap) return -1;
        out[n++] = ch;
    }
    out[n] = 0;
    return n;
}

static int has_all(const char *got, const char *need)
{
    for (const char *p = need; *p; ++p) {
        if (!strchr(got, *p)) {
            printf("missing '%c' in \"%s\"\n", *p, got);
            return 1;
        }
    }
    return 0;
}

int main(void)
{
    const int space = cyd_osk_hit(CYD_OSK_X0 + 2 * CYD_OSK_COL + 4,
                                  CYD_OSK_Y0 + 4 * CYD_OSK_ROW + 4);
    if (expect_hit(CYD_OSK_X0 + 2 * CYD_OSK_COL + 4,
                   CYD_OSK_Y0 + 4 * CYD_OSK_ROW + 4, space))
        return 1;
    const CydOskKey sp = cyd_osk_at(space);
    if (sp.kind != CYD_OSK_SPACE || cyd_osk_char(sp, false) != ' ') {
        printf("space key mismatch\n");
        return 1;
    }

    char letters[64];
    char symbols[64];
    if (collect(letters, (int)sizeof(letters), false) < 0) return 2;
    if (collect(symbols, (int)sizeof(symbols), true) < 0) return 3;
    if (has_all(letters, "abcdefghijklmnopqrstuvwxyz ")) return 4;
    if (has_all(symbols, ",.-|<>@")) return 5;

    /* Same alphabet the UI hook types, produced only through key hits. */
    const char *need = "abcdefghijklmnopqrstuvwxyz ,.-|<>@";
    char typed[64];
    int n = 0;
    for (const char *p = need; *p; ++p) {
        int found = -1;
        for (int i = 0; i < CYD_OSK_N; ++i) {
            const CydOskKey k = cyd_osk_at(i);
            if (cyd_osk_char(k, false) == *p || cyd_osk_char(k, true) == *p) {
                found = i;
                break;
            }
        }
        if (found < 0) {
            printf("no key for '%c'\n", *p);
            return 6;
        }
        const CydOskKey k = cyd_osk_at(found);
        const char ch = cyd_osk_char(k, cyd_osk_char(k, false) == *p ? false : true);
        if (ch != *p) return 7;
        typed[n++] = ch;
    }
    typed[n] = 0;
    if (strcmp(typed, need) != 0) {
        printf("typed \"%s\"\n", typed);
        return 8;
    }
    printf("cyd type gate ok keys=%d letters=%s\n", CYD_OSK_N, letters);
    return 0;
}
