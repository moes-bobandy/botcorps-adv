/* cyd-sim Arduino shim: the minimum the CYD firmware sources call. Host-only. */
#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <math.h>
#include <algorithm>

#ifndef PROGMEM
#define PROGMEM
#endif
#ifndef pgm_read_byte
#define pgm_read_byte(p)  (*(const uint8_t *)(p))
#endif
#ifndef pgm_read_word
#define pgm_read_word(p)  (*(const uint16_t *)(p))
#endif
#ifndef pgm_read_dword
#define pgm_read_dword(p) (*(const uint32_t *)(p))
#endif
#ifndef pgm_read_ptr
#define pgm_read_ptr(p)   (*(const void * const *)(p))
#endif
#define IRAM_ATTR
#define DRAM_ATTR

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2

/* Simulated clock (sim_clock.cpp). Virtual in --script mode, wall clock in live mode. */
uint32_t sim_millis(void);
void sim_delay(uint32_t ms);
static inline unsigned long millis(void) { return sim_millis(); }
static inline unsigned long micros(void) { return (unsigned long)sim_millis() * 1000ul; }
static inline void delay(unsigned long ms) { sim_delay((uint32_t)ms); }
static inline void delayMicroseconds(unsigned int) {}
static inline void yield(void) {}
static inline void pinMode(int, int) {}
static inline void digitalWrite(int, int) {}
static inline int digitalRead(int) { return 1; }
uint32_t esp_random(void);

struct SimSerial {
    void begin(unsigned long) {}
    int printf(const char *fmt, ...) __attribute__((format(printf, 2, 3)));
    void println(const char *s = "");
    void print(const char *s);
    void println(int v) { char b[16]; snprintf(b, sizeof b, "%d", v); println(b); }
    void print(int v) { char b[16]; snprintf(b, sizeof b, "%d", v); print(b); }
    void flush(void) {}
    int available(void) { return 0; }
    int read(void) { return -1; }
};
extern SimSerial Serial;

struct SimESP {
    uint32_t getFreeHeap(void);
    uint32_t getMaxAllocHeap(void);
    uint32_t getMinFreeHeap(void) { return getFreeHeap(); }
    uint32_t getPsramSize(void) { return 0; }
    uint32_t getFreePsram(void) { return 0; }
    void restart(void) { exit(0); }
};
extern SimESP ESP;
