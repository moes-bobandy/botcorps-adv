/* cyd-sim runtime stubs: clock, Serial, ESP/heap numbers, WiFi, Preferences (NVS), esp_random. Host-only. */
#include "Arduino.h"
#include "esp_heap_caps.h"
#include "Preferences.h"
#include "WiFi.h"
#include <map>
#include <string>
#include <chrono>
#include <thread>
#include <random>

/* ---------- clock ----------
 * Virtual mode (scripted runs): millis() only moves when the firmware calls delay() (loop() ends in
 * delay(10)), so a script replays bit-identically. A spin guard advances 1 ms after 20000 millis()
 * calls with no delay so a busy-wait in firmware cannot hang the sim. Real mode: wall clock. */
bool sim_vclock = true;
bool sim_pace = false;            /* virtual clock, but sleep the wall clock too (watchable live) */
static volatile uint32_t s_vt = 0;
static uint32_t s_spin = 0;
static auto s_t0 = std::chrono::steady_clock::now();
uint32_t sim_millis(void)
{
    if (!sim_vclock)
        return (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(
                   std::chrono::steady_clock::now() - s_t0).count();
    if (++s_spin > 20000) { s_spin = 0; s_vt = s_vt + 1; }
    return s_vt;
}
void sim_delay(uint32_t ms)
{
    s_spin = 0;
    if (sim_vclock) {
        s_vt = s_vt + ms;
        if (sim_pace) std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    } else {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }
}
void sim_set_millis(uint32_t t) { s_vt = t; }

static std::mt19937 s_rng(0xC1D5u);
uint32_t esp_random(void) { return (uint32_t)s_rng(); }

/* ---------- Serial ---------- */
bool sim_serial_echo = false;
SimSerial Serial;
int SimSerial::printf(const char *fmt, ...)
{
    if (!sim_serial_echo) return 0;
    va_list ap; va_start(ap, fmt);
    int n = vfprintf(stderr, fmt, ap);
    va_end(ap);
    return n;
}
void SimSerial::println(const char *s) { if (sim_serial_echo) fprintf(stderr, "%s\n", s); }
void SimSerial::print(const char *s) { if (sim_serial_echo) fprintf(stderr, "%s", s); }

/* ---------- heap ----------
 * Canned numbers for an ESP32-WROOM (no PSRAM) after boot. Only used for logs, the frame diag
 * (diagnostic) and the probe's NimBLE gate; the portrait UI forces the direct paint path anyway. */
size_t heap_caps_get_free_size(uint32_t) { return 180u * 1024u; }
size_t heap_caps_get_largest_free_block(uint32_t) { return 110u * 1024u; }
size_t heap_caps_get_minimum_free_size(uint32_t) { return 160u * 1024u; }
SimESP ESP;
uint32_t SimESP::getFreeHeap(void) { return 180u * 1024u; }
uint32_t SimESP::getMaxAllocHeap(void) { return 110u * 1024u; }

SimWiFi WiFi;

/* ---------- Preferences / NVS ---------- */
std::map<std::string, uint32_t> sim_nvs;   /* key = "ns/key" */
const char *sim_nvs_path = nullptr;
void sim_nvs_save(void)
{
    if (!sim_nvs_path) return;
    FILE *f = fopen(sim_nvs_path, "w");
    if (!f) return;
    for (auto &kv : sim_nvs) fprintf(f, "%s %u\n", kv.first.c_str(), (unsigned)kv.second);
    fclose(f);
}
void sim_nvs_load(void)
{
    if (!sim_nvs_path) return;
    FILE *f = fopen(sim_nvs_path, "r");
    if (!f) return;
    char k[128]; unsigned v;
    while (fscanf(f, "%127s %u", k, &v) == 2) sim_nvs[k] = v;
    fclose(f);
}
static std::string key_of(const char *ns, const char *k) { return std::string(ns) + "/" + k; }
bool Preferences::begin(const char *ns, bool ro) { snprintf(ns_, sizeof ns_, "%s", ns); ro_ = ro; open_ = true; return true; }
void Preferences::end(void) { if (open_ && !ro_) sim_nvs_save(); open_ = false; }
bool Preferences::isKey(const char *k) { return sim_nvs.count(key_of(ns_, k)) != 0; }
bool Preferences::remove(const char *k) { return sim_nvs.erase(key_of(ns_, k)) != 0; }
bool Preferences::clear(void)
{
    std::string p = std::string(ns_) + "/";
    for (auto it = sim_nvs.begin(); it != sim_nvs.end();)
        it = it->first.compare(0, p.size(), p) == 0 ? sim_nvs.erase(it) : std::next(it);
    return true;
}
static uint32_t get_(const char *ns, const char *k, uint32_t d)
{
    auto it = sim_nvs.find(key_of(ns, k));
    return it == sim_nvs.end() ? d : it->second;
}
uint8_t Preferences::getUChar(const char *k, uint8_t d) { return (uint8_t)get_(ns_, k, d); }
uint16_t Preferences::getUShort(const char *k, uint16_t d) { return (uint16_t)get_(ns_, k, d); }
uint32_t Preferences::getUInt(const char *k, uint32_t d) { return get_(ns_, k, d); }
int32_t Preferences::getInt(const char *k, int32_t d) { return (int32_t)get_(ns_, k, (uint32_t)d); }
size_t Preferences::putUChar(const char *k, uint8_t v) { if (ro_) return 0; sim_nvs[key_of(ns_, k)] = v; return 1; }
size_t Preferences::putUShort(const char *k, uint16_t v) { if (ro_) return 0; sim_nvs[key_of(ns_, k)] = v; return 2; }
size_t Preferences::putUInt(const char *k, uint32_t v) { if (ro_) return 0; sim_nvs[key_of(ns_, k)] = v; return 4; }
size_t Preferences::putInt(const char *k, int32_t v) { if (ro_) return 0; sim_nvs[key_of(ns_, k)] = (uint32_t)v; return 4; }

/* hub6: layout recorder for the automated overlap check (OVL_BOX in botcorps_ui.cpp) */
#include <set>
#include <cstdio>
#include <string>
static std::set<std::string> s_ovl;
void sim_ovl_box(const char *kind, const char *name, int x, int y, int w, int h)
{
    char b[256]; snprintf(b, sizeof b, "%s\t%d\t%d\t%d\t%d\t%s", kind, x, y, w, h, name ? name : "");
    s_ovl.insert(b);
}
void sim_ovl_clear(void) { s_ovl.clear(); }
void sim_ovl_dump(const char *path)
{
    FILE *f = fopen(path, "w"); if (!f) return;
    for (auto &l : s_ovl) fprintf(f, "%s\n", l.c_str());
    fclose(f);
}
