#pragma once
#include <stdint.h>
#define MALLOC_CAP_INTERNAL 1
static inline uint32_t heap_caps_get_largest_free_block(int) { return 80000; }
