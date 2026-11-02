#pragma once
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
uint32_t millis(void);
struct HostSerial {
    void printf(const char *, ...) {}
    void println(const char *) {}
};
static HostSerial Serial;
struct HostESP {
    uint32_t getFreeHeap() { return 80000; }
};
static HostESP ESP;
namespace fonts {
struct Font { int id; };
extern const Font Font0;
}
