#pragma once

/*
 * One geometry for the CYD on-screen keyboard.
 * Paint and hit-test both use cyd_osk_at(). Rows are 15 px from y=165.
 * Columns are 30 px from x=8. The drawn cell is one pixel short of the
 * pitch so neighbouring keys do not share an edge.
 *
 * CYD_OSK_CLIP_Y is the first row the keyboard owns, including the prompt
 * echo drawn just above the keys. EXT band pushes stop there while open.
 */

#include <stdint.h>

constexpr int CYD_OSK_X0 = 8;
constexpr int CYD_OSK_Y0 = 165;
constexpr int CYD_OSK_COL = 30;
constexpr int CYD_OSK_ROW = 15;
constexpr int CYD_OSK_CLIP_Y = 151;

enum CydOskKind : uint8_t {
    CYD_OSK_CHAR = 1,
    CYD_OSK_SPACE = 2,
    CYD_OSK_BKSP = 3,
    CYD_OSK_ENTER = 4,
    CYD_OSK_TAB = 5,
    CYD_OSK_SYM = 6,
    CYD_OSK_HIDE = 7
};

struct CydOskSpec {
    int8_t row;
    int8_t col;
    int8_t span;
    uint8_t kind;
    char alpha;
    char symbol;
};

struct CydOskKey {
    int16_t x;
    int16_t y;
    int16_t w;
    int16_t h;
    uint8_t kind;
    char alpha;
    char symbol;
};

/* 10 columns. Symbol layer carries '-' '|' '<' '>' '@' plus comma and period. */
constexpr CydOskSpec CYD_OSK_SPEC[] = {
    { 0, 0, 1, CYD_OSK_CHAR, '1', '-' },
    { 0, 1, 1, CYD_OSK_CHAR, '2', '|' },
    { 0, 2, 1, CYD_OSK_CHAR, '3', '<' },
    { 0, 3, 1, CYD_OSK_CHAR, '4', '>' },
    { 0, 4, 1, CYD_OSK_CHAR, '5', '@' },
    { 0, 5, 1, CYD_OSK_CHAR, '6', '.' },
    { 0, 6, 1, CYD_OSK_CHAR, '7', ',' },
    { 0, 7, 1, CYD_OSK_CHAR, '8', '!' },
    { 0, 8, 1, CYD_OSK_CHAR, '9', '?' },
    { 0, 9, 1, CYD_OSK_CHAR, '0', '/' },
    { 1, 0, 1, CYD_OSK_CHAR, 'q', '+' },
    { 1, 1, 1, CYD_OSK_CHAR, 'w', '=' },
    { 1, 2, 1, CYD_OSK_CHAR, 'e', '_' },
    { 1, 3, 1, CYD_OSK_CHAR, 'r', '*' },
    { 1, 4, 1, CYD_OSK_CHAR, 't', '#' },
    { 1, 5, 1, CYD_OSK_CHAR, 'y', '$' },
    { 1, 6, 1, CYD_OSK_CHAR, 'u', '%' },
    { 1, 7, 1, CYD_OSK_CHAR, 'i', '^' },
    { 1, 8, 1, CYD_OSK_CHAR, 'o', '&' },
    { 1, 9, 1, CYD_OSK_CHAR, 'p', '(' },
    { 2, 0, 1, CYD_OSK_CHAR, 'a', ')' },
    { 2, 1, 1, CYD_OSK_CHAR, 's', '[' },
    { 2, 2, 1, CYD_OSK_CHAR, 'd', ']' },
    { 2, 3, 1, CYD_OSK_CHAR, 'f', '{' },
    { 2, 4, 1, CYD_OSK_CHAR, 'g', '}' },
    { 2, 5, 1, CYD_OSK_CHAR, 'h', ':' },
    { 2, 6, 1, CYD_OSK_CHAR, 'j', ';' },
    { 2, 7, 1, CYD_OSK_CHAR, 'k', '\'' },
    { 2, 8, 1, CYD_OSK_CHAR, 'l', '"' },
    { 2, 9, 1, CYD_OSK_TAB, 0, 0 },
    { 3, 0, 1, CYD_OSK_SYM, 0, 0 },
    { 3, 1, 1, CYD_OSK_CHAR, 'z', '~' },
    { 3, 2, 1, CYD_OSK_CHAR, 'x', '`' },
    { 3, 3, 1, CYD_OSK_CHAR, 'c', '\\' },
    { 3, 4, 1, CYD_OSK_CHAR, 'v', '\'' },
    { 3, 5, 1, CYD_OSK_CHAR, 'b', '&' },
    { 3, 6, 1, CYD_OSK_CHAR, 'n', '*' },
    { 3, 7, 1, CYD_OSK_CHAR, 'm', '+' },
    { 3, 8, 2, CYD_OSK_BKSP, 0, 0 },
    { 4, 0, 2, CYD_OSK_HIDE, 0, 0 },
    { 4, 2, 6, CYD_OSK_SPACE, ' ', ' ' },
    { 4, 8, 2, CYD_OSK_ENTER, 0, 0 },
};

constexpr int CYD_OSK_N = (int)(sizeof(CYD_OSK_SPEC) / sizeof(CYD_OSK_SPEC[0]));

constexpr CydOskKey cyd_osk_at(int i)
{
    const CydOskSpec s = CYD_OSK_SPEC[i];
    return {
        (int16_t)(CYD_OSK_X0 + (int)s.col * CYD_OSK_COL),
        (int16_t)(CYD_OSK_Y0 + (int)s.row * CYD_OSK_ROW),
        (int16_t)((int)s.span * CYD_OSK_COL - 1),
        (int16_t)(CYD_OSK_ROW - 1),
        s.kind,
        s.alpha,
        s.symbol
    };
}

constexpr int cyd_osk_hit(int x, int y)
{
    for (int i = 0; i < CYD_OSK_N; ++i) {
        const CydOskKey k = cyd_osk_at(i);
        if (x >= k.x && x < k.x + k.w && y >= k.y && y < k.y + k.h) return i;
    }
    return -1;
}

constexpr char cyd_osk_char(const CydOskKey &k, bool sym)
{
    if (k.kind == CYD_OSK_SPACE) return ' ';
    if (k.kind != CYD_OSK_CHAR) return 0;
    return sym ? k.symbol : k.alpha;
}

static_assert(CYD_OSK_N == 42, "alpha, symbol, and action keys");
static_assert(cyd_osk_at(0).x == 8 && cyd_osk_at(0).y == 165, "origin x=8 y=165");
static_assert(cyd_osk_at(1).x == cyd_osk_at(0).x + 30, "30px columns");
static_assert(cyd_osk_at(10).y == cyd_osk_at(0).y + 15, "15px rows");
static_assert(cyd_osk_at(40).kind == CYD_OSK_SPACE, "SPACE is a key");
static_assert(cyd_osk_at(38).kind == CYD_OSK_BKSP, "BKSP is a key");
static_assert(cyd_osk_at(41).kind == CYD_OSK_ENTER, "ENTER is a key");
static_assert(cyd_osk_at(39).kind == CYD_OSK_HIDE, "hide is a key");
static_assert(cyd_osk_at(30).kind == CYD_OSK_SYM, "symbol layer key");
static_assert(cyd_osk_at(29).kind == CYD_OSK_TAB, "TAB key");
static_assert(cyd_osk_char(cyd_osk_at(40), false) == ' ', "SPACE types a space");
static_assert(cyd_osk_at(6).kind == CYD_OSK_CHAR && cyd_osk_at(6).symbol == ',', "comma is not backspace");
static_assert(cyd_osk_at(5).kind == CYD_OSK_CHAR && cyd_osk_at(5).symbol == '.', "period is not tab");
static_assert(cyd_osk_at(0).symbol == '-' && cyd_osk_at(1).symbol == '|' &&
              cyd_osk_at(2).symbol == '<' && cyd_osk_at(3).symbol == '>' &&
              cyd_osk_at(4).symbol == '@', "symbol layer has - | < > @");
static_assert(cyd_osk_hit(8 + 2 * 30 + 4, 165 + 4 * 15 + 4) == 40, "SPACE hit matches the cell");
static_assert(cyd_osk_hit(0, 160) < 0, "old hit origin is not a key");
