#if defined(BOTCORPS_CYD) && defined(BOTCORPS_CYD_LIVESCAN) && BOTCORPS_CYD_LIVESCAN
/*
 * CYD glue for the live-scan build (env cyd_livescan). Same clip path as lock_ui.cpp
 * (ROM tinfl, swapped palette, pushImage, release-fired taps, NVS theme) over the
 * non-game clip blob lk_blob_live, plus the real radios behind LvRadio:
 *   WiFi : esp_wifi async scan (WiFi.scanNetworks), results copied, scanDelete, WIFI_OFF.
 *   BLE  : NimBLE active scan for N ms, results copied, clearResults, deinit(true).
 *   DEAUTH watch: passive promiscuous mgmt filter, channel hop 1-13, counts 0xC0/0xA0
 *          subtypes. Receive only, nothing is transmitted.
 * Only one radio job runs at a time; each job releases its stack before the next starts.
 */
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <NimBLEDevice.h>
#include "display/botcorps_display.h"
#include "live/lv_ui.h"
#include "live/lv_core.h"
extern "C" {
#include "miniz.h" /* esp_rom/include: ROM tinfl */
}

extern "C" const uint8_t lk_blob_live[];
extern "C" const uint8_t lk_blob_live_end[];

static tinfl_decompressor s_infl;
static uint8_t s_buf[LK_INFL_CAP];
static uint16_t s_line[240];
static uint16_t s_pal[256];
static int s_pal_clip = -1;

static int lv_infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    tinfl_init(&s_infl);
    size_t in = len, out = cap;
    tinfl_status st = tinfl_decompress(&s_infl, src, &in, dst, dst, &out, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    return st == TINFL_STATUS_DONE ? (int)out : -1;
}

static void ui_raw_draw(int y, int x, int n, const uint8_t *idx)
{
    for (int i = 0; i < n; ++i) s_line[i] = s_pal[idx[i]];
    g_cyd.pushImage(x, y, n, 1, (const lgfx::swap565_t *)s_line);
}

static void ui_blit(int x, int y, int w, int h, const uint16_t *px)
{
    g_cyd.pushImage(x, y, w, h, (const lgfx::rgb565_t *)px);
}

static void lv_use_pal(void)
{
    const int c = lk_clip_of_seg(lv_play.seg);
    if (c == s_pal_clip) return;
    s_pal_clip = c;
    memset(s_pal, 0, sizeof s_pal);
    for (int i = 0; i < LK_CLIP[c].npal; ++i) {
        const uint16_t v = LK_CLIP[c].pal[i];
        s_pal[i] = (uint16_t)((v >> 8) | (v << 8));
    }
}

static const LkIo s_io{lk_blob_live, lv_infl, lv_clip_draw, s_buf};

static bool lv_read_tap(int *ox, int *oy)
{
    static bool down_prev = false;
    static int n = 0, up = 0, px[2], py[2];
    static uint32_t upt = 0;
    int tx, ty;
    const bool down = g_cyd.getTouch(&tx, &ty);
    if (down) {
        if (!down_prev) { down_prev = true; n = 0; }
        px[0] = px[1]; py[0] = py[1]; px[1] = tx; py[1] = ty; ++n; up = 0;
        return false;
    }
    if (!down_prev) return false;
    if (++up == 1) upt = millis();
    if (up < 2 || (int32_t)(millis() - upt) < 45) return false;
    down_prev = false; up = 0;
    const int use = n >= 3 ? 0 : 1;
    *ox = px[use]; *oy = py[use]; n = 0;
    return true;
}

static void heap_log(const char *tag)
{
    Serial.printf("[live] heap %s free=%u min=%u largest=%u\n", tag, (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getMinFreeHeap(), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

/* ---------------- radio jobs (one at a time) ---------------- */
enum { JOB_NONE, JOB_WIFI, JOB_BLE, JOB_DEAUTH };
static int s_job = JOB_NONE;
static uint32_t s_job_end = 0;

static void wifi_off(void)
{
    esp_wifi_set_promiscuous(false);
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
}

static void ble_off(void)
{
    if (NimBLEDevice::isInitialized()) {
        NimBLEScan *sc = NimBLEDevice::getScan();
        if (sc->isScanning()) sc->stop();
        sc->clearResults();
        NimBLEDevice::deinit(true);
    }
}

static void r_stop(void)
{
    if (s_job == JOB_WIFI || s_job == JOB_DEAUTH) wifi_off();
    if (s_job == JOB_BLE) ble_off();
    s_job = JOB_NONE;
    heap_log("stop");
}

static int r_wifi_start(void)
{
    if (s_job != JOB_NONE) r_stop();
    heap_log("wifi-pre");
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, false);
    if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) { wifi_off(); return -1; }
    s_job = JOB_WIFI;
    return 0;
}

static int r_wifi_poll(LvFind *out, int max)
{
    if (s_job != JOB_WIFI) return 0;
    const int sc = WiFi.scanComplete();
    if (sc == WIFI_SCAN_RUNNING) return -1;
    int n = 0;
    for (int i = 0; i < sc && n < max; ++i) {
        LvFind &f = out[n];
        memset(&f, 0, sizeof f);
        String ssid = WiFi.SSID(i);
        snprintf(f.name, sizeof f.name, "%s", ssid.c_str());
        const uint8_t *b = WiFi.BSSID(i);
        if (b) memcpy(f.mac, b, 6);
        f.rssi = (int8_t)WiFi.RSSI(i);
        f.ch = (uint8_t)WiFi.channel(i);
        f.auth = (uint8_t)WiFi.encryptionType(i); /* raw wifi_auth_mode_t, as lv_auth_name expects */
        f.kind = LV_K_WIFI;
        f.mfg = 0xFFFF;
        ++n;
    }
    Serial.printf("[live] wifi scan done rc=%d kept=%d\n", sc, n);
    wifi_off();
    s_job = JOB_NONE;
    heap_log("wifi-post");
    return n;
}

static int r_ble_start(uint32_t ms)
{
    if (s_job != JOB_NONE) r_stop();
    heap_log("ble-pre");
    NimBLEDevice::init("");
    NimBLEScan *sc = NimBLEDevice::getScan();
    sc->setActiveScan(true);
    sc->setInterval(100);
    sc->setWindow(99);
    sc->setMaxResults(LV_MAXF);
    if (!sc->start(ms, false, true)) { ble_off(); return -1; }
    s_job = JOB_BLE;
    s_job_end = millis() + ms + 1500;
    return 0;
}

static const uint16_t BLE_SVC16[] = {0xFD5A, 0xFEED, 0xFFFA, 0xFD44, 0xFE2C, 0xFD6F, 0xFE9F, 0x180F, 0x1812};

static int r_ble_poll(LvFind *out, int max)
{
    if (s_job != JOB_BLE) return 0;
    NimBLEScan *sc = NimBLEDevice::getScan();
    if (sc->isScanning() && (int32_t)(millis() - s_job_end) < 0) return -1;
    if (sc->isScanning()) sc->stop();
    NimBLEScanResults r = sc->getResults();
    int n = 0;
    for (int i = 0; i < r.getCount() && n < max; ++i) {
        const NimBLEAdvertisedDevice *d = r.getDevice(i);
        if (!d) continue;
        LvFind &f = out[n];
        memset(&f, 0, sizeof f);
        if (d->haveName()) snprintf(f.name, sizeof f.name, "%s", d->getName().c_str());
        const uint8_t *a = d->getAddress().getVal(); /* little-endian */
        for (int k = 0; k < 6; ++k) f.mac[k] = a[5 - k];
        f.rssi = (int8_t)d->getRSSI();
        f.kind = LV_K_BLE;
        f.mfg = 0xFFFF;
        if (d->haveManufacturerData()) {
            std::string m = d->getManufacturerData();
            if (m.size() >= 2) f.mfg = (uint16_t)((uint8_t)m[0] | ((uint8_t)m[1] << 8));
            if (f.mfg == 0x004C && m.size() >= 3) f.apple = (uint8_t)m[2];
        }
        for (const uint16_t u : BLE_SVC16) {
            const NimBLEUUID id(u);
            bool hit = d->isAdvertisingService(id);
            for (int k = 0; !hit && k < (int)d->getServiceDataCount(); ++k) hit = d->getServiceDataUUID(k) == id;
            if (hit) { f.svc = u; break; }
        }
        ++n;
    }
    Serial.printf("[live] ble scan done seen=%d kept=%d\n", (int)r.getCount(), n);
    ble_off();
    s_job = JOB_NONE;
    heap_log("ble-post");
    return n;
}

static volatile uint32_t s_deauth = 0, s_mgmt = 0;
static uint8_t s_ch = 1;
static uint32_t s_hop = 0;

static void IRAM_ATTR promisc_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t *p = (const wifi_promiscuous_pkt_t *)buf;
    s_mgmt = s_mgmt + 1;
    const uint8_t fc = p->payload[0];
    if (fc == 0xC0 || fc == 0xA0) s_deauth = s_deauth + 1;
}

static int r_deauth_start(uint32_t ms)
{
    if (s_job != JOB_NONE) r_stop();
    heap_log("deauth-pre");
    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, false);
    s_deauth = 0; s_mgmt = 0; s_ch = 1; s_hop = millis() + 250;
    wifi_promiscuous_filter_t flt = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&flt);
    esp_wifi_set_promiscuous_rx_cb(promisc_cb);
    if (esp_wifi_set_promiscuous(true) != ESP_OK) { wifi_off(); return -1; }
    esp_wifi_set_channel(s_ch, WIFI_SECOND_CHAN_NONE);
    s_job = JOB_DEAUTH;
    s_job_end = millis() + ms;
    return 0;
}

static int r_deauth_poll(uint32_t *d, uint32_t *m, uint8_t *ch)
{
    if (s_job != JOB_DEAUTH) { *d = s_deauth; *m = s_mgmt; *ch = s_ch; return 0; }
    const uint32_t now = millis();
    if ((int32_t)(now - s_hop) >= 0) {
        s_ch = s_ch >= 13 ? 1 : s_ch + 1;
        esp_wifi_set_channel(s_ch, WIFI_SECOND_CHAN_NONE);
        s_hop = now + 250;
    }
    *d = s_deauth; *m = s_mgmt; *ch = s_ch;
    if ((int32_t)(now - s_job_end) < 0) return -1;
    Serial.printf("[live] deauth watch done deauth=%u mgmt=%u\n", (unsigned)s_deauth, (unsigned)s_mgmt);
    wifi_off();
    s_job = JOB_NONE;
    heap_log("deauth-post");
    return 0;
}

static void r_sys(int what, char *out, int n)
{
    switch (what) {
    case 0:
        snprintf(out, n, "heap %u free, min %u, big %u", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMinFreeHeap(),
                 (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
        break;
    case 1:
        snprintf(out, n, "%s rev%d %uMHz", ESP.getChipModel(), (int)ESP.getChipRevision(), (unsigned)ESP.getCpuFreqMHz());
        break;
    case 2: {
        static const char *const R[] = {"unknown", "power-on", "ext", "sw", "panic", "int-wdt", "task-wdt", "wdt",
                                        "deepsleep", "brownout", "sdio"};
        const int r = (int)esp_reset_reason();
        snprintf(out, n, "reset: %s", r >= 0 && r <= 10 ? R[r] : "other");
        break;
    }
    case 3:
        snprintf(out, n, "flash %uKB app %uKB", (unsigned)(ESP.getFlashChipSize() / 1024), (unsigned)(ESP.getSketchSize() / 1024));
        break;
    default:
        snprintf(out, n, "touch cal %s", botcorps_cyd_has_valid_cal() ? "OK (NVS)" : "MISSING");
        break;
    }
}

static void r_save_theme(int t)
{
    Preferences prefs;
    if (prefs.begin("botcorps", false)) { prefs.putUChar("lktheme", (uint8_t)t); prefs.end(); }
}

/* CALbot result: apply the 8-point matrix now, then persist with the SAME keys and order as
 * botcorps_cyd_run_touch_cal(): remove cv, tcal=1, c0..c7, cv=2 LAST (a partial write can't pass has_valid_cal). */
static bool r_cal_save(const uint16_t c8[8])
{
    g_cyd.apply_touch_cal8(c8);
    Preferences prefs;
    if (!prefs.begin("botcorps", false)) { Serial.println("[live] CALbot NVS begin failed"); return false; }
    prefs.remove("cv");
    prefs.putUChar("tcal", 1);
    bool ok = true;
    for (int i = 0; i < 8; ++i) { char key[4]; snprintf(key, sizeof key, "c%d", i); if (!prefs.putUShort(key, c8[i])) ok = false; }
    if (ok && !prefs.putUChar("cv", 2)) ok = false;
    prefs.end();
    Serial.printf("[live] CALbot c0..c7 = %u %u %u %u %u %u %u %u -> NVS %s\n", c8[0], c8[1], c8[2], c8[3], c8[4], c8[5], c8[6], c8[7], ok ? "OK" : "FAIL");
    return ok && botcorps_cyd_has_valid_cal();
}

/* CONTROLbot ORIENT: 0 = setRotation(2) (portrait, USB at the bottom, default), 1 = setRotation(0) (flipped 180).
 * Touch follows the panel rotation inside LovyanGFX; the native-order c0..c7 cal stays valid either way. */
static void r_set_rot(int flip)
{
    g_cyd.setRotation(flip ? 0 : 2);
    g_cyd.fillScreen(0x0000);
    Preferences prefs;
    if (prefs.begin("botcorps", false)) { prefs.putUChar("lkrot", (uint8_t)(flip ? 1 : 0)); prefs.end(); }
    Serial.printf("[live] ORIENT %s (setRotation %d)\n", flip ? "FLIP" : "PORT", flip ? 0 : 2);
}

static const LvRadio s_radio{r_wifi_start, r_wifi_poll, r_ble_start, r_ble_poll, r_deauth_start, r_deauth_poll,
                             r_stop, r_sys, r_save_theme, r_cal_save, r_set_rot};

void lv_ui_begin(void)
{
    botcorps_cyd_orient = 1;
    botcorps_cyd_apply_rotation(); /* rot 2: portrait 240x320, USB at the bottom */
    WiFi.mode(WIFI_OFF);
    Preferences prefs;
    int th = 0, rot = 0;
    if (prefs.begin("botcorps", true)) { th = prefs.getUChar("lktheme", 0); rot = prefs.getUChar("lkrot", 0); prefs.end(); }
    lv_flip = rot == 1;
    if (lv_flip) g_cyd.setRotation(0);   /* CONTROLbot ORIENT = FLIP */
    g_cyd.fillScreen(0x0000);
    if (th < 0 || th >= LV_NTHEME) th = 0;
    Serial.printf("[live] blob=%u (expect %u) theme=%d %dx%d\n", (unsigned)(lk_blob_live_end - lk_blob_live),
                  (unsigned)LK_BLOB_SIZE, th, (int)g_cyd.width(), (int)g_cyd.height());
    heap_log("begin");
    const bool cal_ok = botcorps_cyd_has_valid_cal();   /* if true, display begin already applied it from NVS */
    Serial.printf("[live] CALbot: saved cal %s\n", cal_ok ? "valid (cv=2) - hold 1 s to redo" : "missing - capture 4 corners");
    lv_begin(&s_radio, ui_blit, ui_raw_draw, th, millis(), cal_ok);
}

void lv_ui_loop(void)
{
    static uint32_t next = 0;
    int x, y;
    if (lv_scr == LV_SC_CAL) {                 /* CALbot reads RAW XPT2046, not the calibrated getTouch() */
        int rx = 0, ry = 0;
        const bool d = g_cyd.get_touch_raw(&rx, &ry);
        lv_cal_raw(d, rx, ry, millis());
    }
    else if (lv_read_tap(&x, &y)) {
        lv_use_pal();
        g_cyd.startWrite();
        const char *a = lv_tap(x, y);
        g_cyd.endWrite();
        Serial.printf("[live] tap %d,%d scr=%d -> %s\n", x, y, (int)lv_scr, a ? a : "-");
    }
    const uint32_t now = millis();
    if ((int32_t)(now - next) < 0) { delay(1); return; }
    next = now + 30;
    lv_use_pal();
    g_cyd.startWrite();
    const uint32_t t_us = micros();
    lv_tick(s_io, now);
    g_cyd.endWrite();
    if (lv_scr == LV_SC_RADAR) {   /* field fps proof for the radar: render + SPI per tick, logged every 5 s */
        static uint32_t acc = 0, n = 0, mx = 0, t_log = 0;
        const uint32_t d = micros() - t_us; acc += d; ++n; if (d > mx) mx = d;
        if ((int32_t)(now - t_log) >= 5000) {
            if (n) Serial.printf("[live] radar %lu ticks avg %.1f ms max %.1f ms (~%.1f fps at 30 ms ticks)\n", (unsigned long)n, acc / 1000.0f / n, mx / 1000.0f,
                                 1000.0f / ((acc / 1000.0f / n) > 30.0f ? (acc / 1000.0f / n) : 30.0f));
            acc = 0; n = 0; mx = 0; t_log = now;
        }
    }
}
#endif
