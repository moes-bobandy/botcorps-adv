/* cyd-sim NimBLE shim: probe.cpp only does init()/deinit() as a heap probe. */
#pragma once
struct NimBLEDevice { static bool init(const char *) { return true; } static void deinit(bool = false) {} };
