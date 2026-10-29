/* cyd-sim Preferences (NVS) shim: in-memory key store, optionally loaded/saved to a file (sim_nvs.cpp). */
#pragma once
#include <stdint.h>
#include <stddef.h>
class Preferences {
    char ns_[16] = {0};
    bool ro_ = true, open_ = false;
public:
    bool begin(const char *ns, bool readOnly = false);
    void end(void);
    bool isKey(const char *k);
    bool remove(const char *k);
    bool clear(void);
    uint8_t getUChar(const char *k, uint8_t d = 0);
    uint16_t getUShort(const char *k, uint16_t d = 0);
    uint32_t getUInt(const char *k, uint32_t d = 0);
    int32_t getInt(const char *k, int32_t d = 0);
    bool getBool(const char *k, bool d = false) { return getUChar(k, d ? 1 : 0) != 0; }
    size_t putUChar(const char *k, uint8_t v);
    size_t putUShort(const char *k, uint16_t v);
    size_t putUInt(const char *k, uint32_t v);
    size_t putInt(const char *k, int32_t v);
    size_t putBool(const char *k, bool v) { return putUChar(k, v ? 1 : 0); }
};
