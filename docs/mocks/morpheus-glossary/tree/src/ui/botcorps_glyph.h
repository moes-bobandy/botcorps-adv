#pragma once

#include <stdint.h>
#include <stdbool.h>

namespace lgfx { inline namespace v1 { class LovyanGFX; } }

/* 12x12 icon art. Writes palette indices (band byte, or native RGB565 on a panel). */
void botcorps_glyph_blit(lgfx::LovyanGFX &dst, int x, int y, int id,
                         uint8_t c1, uint8_t c2, uint8_t c3,
                         bool (*allow)(int px, int py));
