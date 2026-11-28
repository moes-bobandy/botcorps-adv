/* v1.2.26 (Mobandy 2026-11-28 "Version B ... then build all of v1.2.26"): the LOGbot subscreen's REAL log.
 * A small ring of the engine's own log lines: everything lv_print / lv_event already say (TTY + ticker), the
 * per-device scan results, and the lv_ui.cpp boot / SD / save lines. Each entry: millis() (lv_now) at push,
 * kind (WIFI / BLE / SYS) and the text. No sample data anywhere. Compiled in only with LV_CYD27_LOGSUB. */
#pragma once
#if LV_CYD27_LOGSUB
#ifndef LV_LOG_N
#define LV_LOG_N 80
#endif
#define LV_LOG_W 44
enum { LV_LOG_WIFI = 0, LV_LOG_BLE = 1, LV_LOG_SYS = 2 };
struct LvLogE { uint32_t ms; uint8_t kind; char t[LV_LOG_W]; };
static LvLogE lv_log_r[LV_LOG_N];
static uint32_t lv_log_seq = 0;   /* entries ever pushed; entry s is lv_log_r[s % LV_LOG_N] while s + LV_LOG_N > lv_log_seq */
/* lv_ui.cpp hooks (host sim: nullptr -> the cell says so) */
static int (*lv_log_sd_hook)(uint32_t *free_mb, uint32_t *size_mb) = nullptr;   /* 1 card mounted, 0 no card */
static uint32_t (*lv_log_heap_hook)(void) = nullptr;                            /* ESP.getFreeHeap() */
static int (*lv_log_save_hook)(int filt, uint32_t from_seq, uint32_t to_seq, char *path, int n) = nullptr;   /* lines written, -1 fail */
static bool lv_log_ci(const char *h, const char *n)   /* case-insensitive substring */
{
    for (; *h; ++h) {
        const char *a = h, *b = n;
        while (*a && *b && ((*a | 32) == (*b | 32))) { ++a; ++b; }
        if (!*b) return true;
    }
    return false;
}
static int lv_log_kind(const char *s)
{
    if (lv_log_ci(s, "scan done")) return LV_LOG_SYS;   /* summary of both radios */
    if (lv_log_ci(s, "ble") || lv_log_ci(s, "nimble") || lv_log_ci(s, "airtag") || lv_log_ci(s, "tracker")) return LV_LOG_BLE;
    if (lv_log_ci(s, "wifi") || lv_log_ci(s, "ssid") || lv_log_ci(s, "deauth") || lv_log_ci(s, "wlan")) return LV_LOG_WIFI;
    return LV_LOG_SYS;
}
static void lv_log_push(int kind, const char *s)
{
    if (!s) return;
    while (*s == ' ' || *s == '>' || *s == '\t') ++s;   /* TTY prompt / indent */
    if (!*s) return;
    LvLogE &e = lv_log_r[lv_log_seq % LV_LOG_N];
    e.ms = lv_now; e.kind = (uint8_t)(kind < 0 ? lv_log_kind(s) : kind);
    int i = 0; for (; s[i] && i < LV_LOG_W - 1; ++i) e.t[i] = (s[i] == '\n' || s[i] == '\r') ? ' ' : s[i];
    e.t[i] = 0;
    ++lv_log_seq;
}
static void lv_log_pushf(int kind, const char *fmt, ...)
{
    char b[LV_LOG_W]; va_list ap; va_start(ap, fmt); vsnprintf(b, sizeof b, fmt, ap); va_end(ap);
    lv_log_push(kind, b);
}
static uint32_t lv_log_first(void) { return lv_log_seq > LV_LOG_N ? lv_log_seq - LV_LOG_N : 0; }
static const LvLogE *lv_log_at(uint32_t s) { return (s >= lv_log_first() && s < lv_log_seq) ? &lv_log_r[s % LV_LOG_N] : nullptr; }
/* uptime stamp of an entry, 5 chars: MM:SS under 100 min, then HHhMM */
static void lv_log_stamp(uint32_t ms, char o[8])
{
    const uint32_t s = ms / 1000u;
    if (s < 6000u) snprintf(o, 8, "%02u:%02u", (unsigned)(s / 60u), (unsigned)(s % 60u));
    else snprintf(o, 8, "%02uh%02u", (unsigned)((s / 3600u) % 100u), (unsigned)(s / 60u % 60u));
}
#define LV_LOG_PUSH(k, s) lv_log_push((k), (s))
#else
#define LV_LOG_PUSH(k, s) ((void)0)
#endif
