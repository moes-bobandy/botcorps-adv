/* cyd-sim: canned ESP32 (no PSRAM) heap numbers; allocations are plain malloc. */
#pragma once
#include <stdlib.h>
#include <stdint.h>
#define MALLOC_CAP_INTERNAL (1 << 11)
#define MALLOC_CAP_8BIT (1 << 2)
#define MALLOC_CAP_DMA (1 << 3)
#define MALLOC_CAP_SPIRAM (1 << 10)
#define MALLOC_CAP_DEFAULT (1 << 12)
size_t heap_caps_get_free_size(uint32_t caps);
size_t heap_caps_get_largest_free_block(uint32_t caps);
size_t heap_caps_get_minimum_free_size(uint32_t caps);
static inline void *heap_caps_malloc(size_t n, uint32_t) { return malloc(n); }
static inline void *heap_caps_calloc(size_t a, size_t n, uint32_t) { return calloc(a, n); }
static inline void heap_caps_free(void *p) { free(p); }
