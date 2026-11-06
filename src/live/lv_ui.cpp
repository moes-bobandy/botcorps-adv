#if defined(BOTCORPS_CYD) && defined(BOTCORPS_CYD_LIVESCAN) && BOTCORPS_CYD_LIVESCAN
/*
 * CYD glue for the live-scan build (env cyd_livescan). Same clip path as lock_ui.cpp
 * (ROM tinfl, swapped palette, pushImage, release-fired taps, NVS theme) over the
 * non-game clip blob lk_blob_live, plus the real radios behind LvRadio:
 *   WiFi : warm async scanning on Core 0, epoch-tagged result handoff.
 *   BLE  : warm NimBLE async scanning on Core 0; no driver calls on the UI core.
 *   DEAUTH watch: passive promiscuous mgmt filter, channel hop 1-13, counts 0xC0/0xA0
 *          subtypes. Receive only, nothing is transmitted.
 * Only one stack is initialized; same-mode scans reuse it until STOP or a mode switch.
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

/* s_pal mirrors the palette of the clip now playing. lv_go() (also run from inside lv_tick via lv_go_soon)
 * switches clips mid-tick, so the cache is re-validated on every raw draw, not only before lv_tick/lv_tap:
 * a stale cache painted the new clip's keyframe with the old clip's palette (home chips "inverted"). */
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

/* Store wire-order RGB565 so LovyanGFX can DMA without a colour-conversion copy. */
static uint16_t *s_dma_buf[2] = {nullptr, nullptr};
static int s_dma_rows = 0, s_dma_cur = 0, s_dma_x = 0, s_dma_y = 0, s_dma_w = 0, s_dma_h = 0;
static void ui_dma_flush(void)
{
    if (!s_dma_h) return;
    g_cyd.waitDisplay(); g_cyd.waitDMA();
    g_cyd.pushImageDMA(s_dma_x, s_dma_y, s_dma_w, s_dma_h,
                       (const lgfx::swap565_t *)s_dma_buf[s_dma_cur]);
    s_dma_cur ^= 1; s_dma_h = 0;
}
static void ui_live_end(void)
{
    ui_dma_flush();
    g_cyd.waitDisplay(); g_cyd.waitDMA(); // HSPI must be idle before CS15 release and VSPI touch polling.
    g_cyd.endWrite();
}
static void ui_dma_draw(int x, int y, int w, int h, const uint16_t *px, bool wire)
{
    if (!s_dma_rows || h > s_dma_rows || w > 240) {
        ui_dma_flush(); g_cyd.waitDisplay(); g_cyd.waitDMA();
        if (wire) g_cyd.pushImage(x, y, w, h, (const lgfx::swap565_t *)px);
        else g_cyd.pushImage(x, y, w, h, (const lgfx::rgb565_t *)px);
        g_cyd.waitDisplay(); g_cyd.waitDMA(); // the caller may immediately reuse px
        return;
    }
    if (s_dma_h && (x != s_dma_x || w != s_dma_w || y != s_dma_y + s_dma_h || s_dma_h + h > s_dma_rows)) ui_dma_flush();
    if (!s_dma_h) { s_dma_x = x; s_dma_y = y; s_dma_w = w; }
    uint16_t *dst = s_dma_buf[s_dma_cur] + s_dma_h * w;
    if (wire) memcpy(dst, px, w * h * sizeof(uint16_t));
    else for (int i = 0; i < w * h; ++i) dst[i] = (uint16_t)((px[i] >> 8) | (px[i] << 8));
    s_dma_h += h;
    if (s_dma_h == s_dma_rows) ui_dma_flush();
}

static void ui_raw_draw(int y, int x, int n, const uint8_t *idx)
{
    lv_use_pal();
    for (int i = 0; i < n; ++i) s_line[i] = s_pal[idx[i]];
    ui_dma_draw(x, y, n, 1, s_line, true);
}

static void ui_blit(int x, int y, int w, int h, const uint16_t *px)
{
    ui_dma_draw(x, y, w, h, px, false);
}

static const LkIo s_io{lk_blob_live, lv_infl, lv_clip_draw, s_buf};

static bool s_tdown = false; static int s_tx = 0, s_ty = 0;   /* live finger state for lv_touch (radar hold-to-lock) */
static bool lv_read_tap(int *ox, int *oy)
{
    static bool down_prev = false;
    static int n = 0, up = 0, px[2], py[2];
    static uint32_t upt = 0;
    int tx, ty;
    const bool down = g_cyd.getTouch(&tx, &ty);
    s_tdown = down; if (down) { s_tx = tx; s_ty = ty; }
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

/* The UI owns requests; only this Core 0 task owns either radio driver. */
enum { LV_RK_NONE, LV_RK_WIFI, LV_RK_BLE, LV_RK_DEAUTH };
static TaskHandle_t s_radio_task = nullptr;
static portMUX_TYPE s_radio_mux = portMUX_INITIALIZER_UNLOCKED;
struct RadioRequest { int kind; uint32_t epoch, ms; };
static RadioRequest s_request = {LV_RK_NONE, 0, 0};
static LvFind s_radio_batch[LV_MAXF], s_radio_work[LV_MAXF];
static uint32_t s_batch_epoch = 0;
static int s_batch_n = 0;
static bool s_batch_ready = false;
static uint32_t s_deauth = 0, s_mgmt = 0;
static uint8_t s_ch = 1;
static bool s_deauth_done = false;

static RadioRequest radio_request(void)
{
    portENTER_CRITICAL(&s_radio_mux);
    const RadioRequest r = s_request;
    portEXIT_CRITICAL(&s_radio_mux);
    return r;
}
static int r_request(int kind, uint32_t ms)
{
    if (!s_radio_task) return -2;
    portENTER_CRITICAL(&s_radio_mux);
    s_request.kind = kind; ++s_request.epoch; s_request.ms = ms;
    s_batch_ready = false; s_batch_n = 0;
    s_deauth_done = false;
    if (kind == LV_RK_DEAUTH) { s_deauth = s_mgmt = 0; s_ch = 1; }
    portEXIT_CRITICAL(&s_radio_mux);
    xTaskNotifyGive(s_radio_task);
    return 0;
}
static void r_stop(void) { (void)r_request(LV_RK_NONE, 0); }
static int r_wifi_start(void) { return r_request(LV_RK_WIFI, 0); }
static int r_ble_start(uint32_t ms) { return r_request(LV_RK_BLE, ms); }
static int r_deauth_start(uint32_t ms) { return r_request(LV_RK_DEAUTH, ms); }
static int r_poll(int kind, LvFind *out, int max)
{
    int n = -1;
    portENTER_CRITICAL(&s_radio_mux);
    if (s_request.kind == kind && s_batch_ready && s_batch_epoch == s_request.epoch) {
        n = s_batch_n < max ? s_batch_n : max;
        if (n > 0) memcpy(out, s_radio_batch, n * sizeof(LvFind));
        s_batch_ready = false;
    }
    portEXIT_CRITICAL(&s_radio_mux);
    return n;
}
static int r_wifi_poll(LvFind *out, int max) { return r_poll(LV_RK_WIFI, out, max); }
static int r_ble_poll(LvFind *out, int max) { return r_poll(LV_RK_BLE, out, max); }
static int r_deauth_poll(uint32_t *d, uint32_t *m, uint8_t *ch)
{
    portENTER_CRITICAL(&s_radio_mux);
    *d = s_deauth; *m = s_mgmt; *ch = s_ch;
    const bool done = s_deauth_done;
    const bool failed = s_batch_ready && s_batch_epoch == s_request.epoch && s_batch_n < -1;
    portEXIT_CRITICAL(&s_radio_mux);
    return failed ? -2 : done ? 0 : -1;
}
static void radio_publish(const RadioRequest &r, int n)
{
    portENTER_CRITICAL(&s_radio_mux);
    if (r.epoch == s_request.epoch && r.kind == s_request.kind) {
        if (n > 0) memcpy(s_radio_batch, s_radio_work, n * sizeof(LvFind));
        s_batch_n = n; s_batch_epoch = r.epoch; s_batch_ready = true;
    }
    portEXIT_CRITICAL(&s_radio_mux);
}
static void wifi_off(void)
{
    esp_wifi_set_promiscuous(false);
    esp_wifi_scan_stop(); WiFi.scanDelete(); WiFi.mode(WIFI_OFF);
}
static void ble_off(void)
{
    if (!NimBLEDevice::isInitialized()) return;
    NimBLEScan *sc = NimBLEDevice::getScan();
    if (sc->isScanning()) sc->stop();
    sc->clearResults(); NimBLEDevice::deinit(true);
}
static int radio_wifi_results(LvFind *out, int max, int sc)
{
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
    return n;
}
static const uint16_t BLE_SVC16[] = {0xFD5A, 0xFEED, 0xFFFA, 0xFD44, 0xFE2C, 0xFD6F, 0xFE9F, 0x180F, 0x1812};
static int radio_ble_results(LvFind *out, int max, NimBLEScan *sc)
{
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
    return n;
}
static void IRAM_ATTR promisc_cb(void *buf, wifi_promiscuous_pkt_type_t type)
{
    if (type != WIFI_PKT_MGMT) return;
    const wifi_promiscuous_pkt_t *p = (const wifi_promiscuous_pkt_t *)buf;
    if (p->rx_ctrl.sig_len < 2) return;
    portENTER_CRITICAL(&s_radio_mux);
    ++s_mgmt;
    if (p->payload[0] == 0xC0 || p->payload[0] == 0xA0) ++s_deauth;
    portEXIT_CRITICAL(&s_radio_mux);
}
static void lv_radio_task(void *)
{
    int warm = LV_RK_NONE;
    uint32_t epoch = 0, end = 0, hop = 0;
    bool scanning = false, failed = false;
    for (;;) {
        const RadioRequest r = radio_request();
        if (r.kind != warm || r.epoch != epoch) {
            if (warm != r.kind || failed) {
                if (warm == LV_RK_BLE) ble_off();
                else if (warm != LV_RK_NONE) wifi_off();
                warm = LV_RK_NONE; failed = false;
                // The previous stack is fully deinitialized before the other can be initialized.
                if (r.kind == LV_RK_BLE) {
                    failed = !NimBLEDevice::init("");
                    if (!failed) {
                        NimBLEScan *sc = NimBLEDevice::getScan();
                        sc->setActiveScan(true); sc->setInterval(100); sc->setWindow(99); sc->setMaxResults(LV_MAXF);
                    }
                } else if (r.kind != LV_RK_NONE) {
                    WiFi.persistent(false); failed = !WiFi.mode(WIFI_STA);
                    if (!failed) WiFi.disconnect(false, false);
                }
                warm = r.kind;
            } else if (scanning) {
                if (warm == LV_RK_WIFI) { esp_wifi_scan_stop(); WiFi.scanDelete(); }
                if (warm == LV_RK_BLE) { NimBLEDevice::getScan()->stop(); NimBLEDevice::getScan()->clearResults(); }
            }
            epoch = r.epoch; scanning = false;
            end = millis() + r.ms; hop = millis();
            if (warm == LV_RK_DEAUTH && !failed) {
                wifi_promiscuous_filter_t flt = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
                esp_wifi_set_promiscuous_filter(&flt); esp_wifi_set_promiscuous_rx_cb(promisc_cb);
                failed = esp_wifi_set_promiscuous(true) != ESP_OK;
            }
            if (failed) { Serial.printf("[live] radio mode %d init failed\n", warm); radio_publish(r, -2); }
        }
        if (warm == LV_RK_NONE || failed) { ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(200)); continue; }
        if (warm == LV_RK_WIFI) {
            if (!scanning) {
                scanning = WiFi.scanNetworks(true, true) != WIFI_SCAN_FAILED;
                if (!scanning) { failed = true; radio_publish(r, -2); }
            } else {
                const int n = WiFi.scanComplete();
                if (n != WIFI_SCAN_RUNNING) {
                    if (n < 0) { failed = true; radio_publish(r, -2); }
                    else radio_publish(r, radio_wifi_results(s_radio_work, LV_MAXF, n));
                    WiFi.scanDelete(); scanning = false;
                }
            }
        } else if (warm == LV_RK_BLE) {
            NimBLEScan *sc = NimBLEDevice::getScan();
            if (!scanning) {
                // Installed NimBLE 2.x: duration is milliseconds; start is non-blocking.
                scanning = sc->start(r.ms ? r.ms : 5000, false, true);
                end = millis() + (r.ms ? r.ms : 5000) + 500;
                if (!scanning) { failed = true; radio_publish(r, -2); }
            } else if (!sc->isScanning() || (int32_t)(millis() - end) >= 0) {
                if (sc->isScanning()) sc->stop();
                radio_publish(r, radio_ble_results(s_radio_work, LV_MAXF, sc));
                sc->clearResults(); scanning = false;
            }
        } else {
            const uint32_t now = millis();
            if ((int32_t)(now - hop) >= 0) {
                portENTER_CRITICAL(&s_radio_mux);
                const uint8_t ch = s_ch = s_ch >= 13 ? 1 : s_ch + 1;
                portEXIT_CRITICAL(&s_radio_mux);
                esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE); hop = now + 120;
            }
            if ((int32_t)(now - end) >= 0) {
                wifi_off(); warm = LV_RK_NONE;
                portENTER_CRITICAL(&s_radio_mux);
                if (r.epoch == s_request.epoch) { s_request.kind = LV_RK_NONE; s_deauth_done = true; }
                portEXIT_CRITICAL(&s_radio_mux);
            }
        }
        // Notification interrupts this wait on STOP/mode changes; never block the UI core.
        ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(20));
    }
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
 * botcorps_cyd_run_touch_cal(): remove cv, tcal=1, c0..c7, cv=3 LAST (a partial write can't pass has_valid_cal). */
static bool r_cal_save(const uint16_t c8[8])
{
    g_cyd.apply_touch_cal8(c8);
    Preferences prefs;
    if (!prefs.begin("botcorps", false)) { Serial.println("[live] CALbot NVS begin failed"); return false; }
    prefs.remove("cv");
    prefs.putUChar("tcal", 1);
    bool ok = true;
    for (int i = 0; i < 8; ++i) { char key[4]; snprintf(key, sizeof key, "c%d", i); if (!prefs.putUShort(key, c8[i])) ok = false; }
    if (ok && !prefs.putUChar("cv", 3)) ok = false;
    prefs.end();
    Serial.printf("[live] CALbot c0..c7 = %u %u %u %u %u %u %u %u -> NVS %s\n", c8[0], c8[1], c8[2], c8[3], c8[4], c8[5], c8[6], c8[7], ok ? "OK" : "FAIL");
    return ok && botcorps_cyd_has_valid_cal();
}

/* CONTROLbot ORIENT: 0 = setRotation(2) (portrait, USB at the bottom, default), 1 = setRotation(0) (flipped 180).
 * Touch follows the panel rotation inside LovyanGFX; the native-order c0..c7 cal stays valid either way. */
static void r_set_rot(int flip)
{
    ui_dma_flush(); g_cyd.waitDisplay(); g_cyd.waitDMA();
    g_cyd.setRotation(flip ? 0 : 2);
    g_cyd.fillScreen(0x0000);
    Preferences prefs;
    if (prefs.begin("botcorps", false)) { prefs.putUChar("lkrot", (uint8_t)(flip ? 1 : 0)); prefs.end(); }
    Serial.printf("[live] ORIENT %s (setRotation %d)\n", flip ? "FLIP" : "PORT", flip ? 0 : 2);
}

/* radar IGNORE / WATCH lists: 6-byte MACs back to back in NVS "botcorps" (keys rdign / rdwat) */
static int r_mac_load(const char *key, uint8_t *out, int max)
{
    Preferences prefs; if (!prefs.begin("botcorps", true)) return 0;
    const size_t n = prefs.isKey(key) ? prefs.getBytes(key, out, (size_t)max * 6) : 0; prefs.end(); return (int)(n / 6);
}
static void r_mac_save(const char *key, const uint8_t *macs, int n)
{
    Preferences prefs; if (!prefs.begin("botcorps", false)) return;
    if (n > 0) prefs.putBytes(key, macs, (size_t)n * 6); else prefs.remove(key);
    prefs.end(); Serial.printf("[live] NVS %s = %d MACs\n", key, n);
}

#if LV_BG_ENGINE
/* background engine: BG STYLE (0..4, 5 = OFF) + BG COLOR (0..5) in NVS "botcorps" lkbgs / lkbgc */
static void r_bg_save(int style, int col)
{
    Preferences prefs;
    if (prefs.begin("botcorps", false)) { prefs.putUChar("lkbgs", (uint8_t)style); prefs.putUChar("lkbgc", (uint8_t)col); prefs.end(); }
    Serial.printf("[live] NVS BG style=%d col=%d\n", style, col);
}
#endif

static const LvRadio s_radio{r_wifi_start, r_wifi_poll, r_ble_start, r_ble_poll, r_deauth_start, r_deauth_poll,
                             r_stop, r_sys, r_save_theme, r_cal_save, r_set_rot, r_mac_load, r_mac_save
#if LV_BG_ENGINE
                             , r_bg_save
#endif
                             };

static void ui_live_init(void)
{
    static bool initialized = false;
    if (initialized) return;
    initialized = true;
    // Account for every persistent UI allocation BEFORE reserving graphics headroom.
    if (!lv_shadow) lv_shadow = (uint8_t *)malloc(LV_SHADOW_CAP);
    if (!lv_port_under) lv_port_under = (uint16_t *)malloc(LV_PORT_MAXGLOW * sizeof(uint16_t));
#if LV_RD_CHARGE
    if (!lv_rd_cgb) {
        lv_rd_cgb = (LvRdCgBuf *)malloc(sizeof(LvRdCgBuf));
        if (lv_rd_cgb) { lv_rd_px = lv_rd_cgb->px; lv_rd_py = lv_rd_cgb->py; }
    }
#endif
    if (!s_radio_task && xTaskCreatePinnedToCore(lv_radio_task, "lv_radio", 6144, nullptr, 1, &s_radio_task, 0) != pdPASS)
        Serial.println("[live] radio worker allocation FAILED");
    const uint32_t caps = MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
    const size_t reserve = 105 * 1024;
    // A 24-row fallback is mandatory. Grow to 64 only while leaving space for two 16-row DMA strips.
    lv_band_rows = 24;
    for (const int rows : {64, 48, 32}) {
        if (heap_caps_get_free_size(caps) >= reserve + (rows + 32) * 240 * 2 + 512) { lv_band_rows = rows; break; }
    }
    lv_bandbuf = (uint16_t *)heap_caps_malloc(LV_BAND_PX * sizeof(uint16_t), caps);
    if (!lv_bandbuf) { Serial.println("[live] render buffer allocation FAILED"); abort(); }
    for (const int rows : {48, 32, 16}) {
        const size_t bytes = 240 * rows * sizeof(uint16_t);
        if (heap_caps_get_free_size(caps) < reserve + 2 * bytes + 512) continue;
        s_dma_buf[0] = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        s_dma_buf[1] = (uint16_t *)heap_caps_malloc(bytes, MALLOC_CAP_DMA | MALLOC_CAP_8BIT);
        if (s_dma_buf[0] && s_dma_buf[1]) { s_dma_rows = rows; break; }
        heap_caps_free(s_dma_buf[0]); heap_caps_free(s_dma_buf[1]);
        s_dma_buf[0] = s_dma_buf[1] = nullptr;
    }
    Serial.printf("[GFX-RAM] dma_rows=%d dma_bytes=%u band_rows=%d band_bytes=%u heap=%u 8bit=%u dma=%u internal8=%u\n",
                  s_dma_rows, (unsigned)(2 * 240 * s_dma_rows * sizeof(uint16_t)), lv_band_rows,
                  (unsigned)(LV_BAND_PX * sizeof(uint16_t)), (unsigned)ESP.getFreeHeap(),
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_8BIT), (unsigned)heap_caps_get_free_size(MALLOC_CAP_DMA),
                  (unsigned)heap_caps_get_free_size(caps));
}

void lv_ui_begin(void)
{
    g_cyd.setBrightness(0);   /* item 3: panel dark until the themed first frame is in GRAM (no black/legacy flash at power-on) */
    botcorps_cyd_orient = 1;
    botcorps_cyd_apply_rotation(); /* rot 2: portrait 240x320, USB at the bottom */
    Preferences prefs;
#if LV_BG_ENGINE
    int th = 0, rot = 0, bgs = 0, bgc = 0;
    if (prefs.begin("botcorps", true)) { th = prefs.getUChar("lktheme", 0); rot = prefs.getUChar("lkrot", 0); bgs = prefs.getUChar("lkbgs", 0); bgc = prefs.getUChar("lkbgc", 0); prefs.end(); }
    lv_rain = bgs < LV_NBGSTYLE; lv_bgstyle = bgs < LV_NBGSTYLE ? bgs : 0; lv_raincol = bgc < LV_NBGCOL ? bgc : 0;   /* background engine (defaults RAIN + GREEN) */
#else
    int th = 0, rot = 0;
    if (prefs.begin("botcorps", true)) { th = prefs.getUChar("lktheme", 0); rot = prefs.getUChar("lkrot", 0); prefs.end(); }
#endif
    lv_flip = rot == 1;
    if (lv_flip) g_cyd.setRotation(0);   /* CONTROLbot ORIENT = FLIP */
    g_cyd.fillScreen(0x0000);
    if (th < 0 || th >= LV_NTHEME) th = 0;
    ui_live_init(); // NVS/display startup allocations are now included in the heap budget.
    Serial.printf("[live] blob=%u (expect %u) theme=%d %dx%d\n", (unsigned)(lk_blob_live_end - lk_blob_live),
                  (unsigned)LK_BLOB_SIZE, th, (int)g_cyd.width(), (int)g_cyd.height());
    heap_log("begin");
    const bool cal_ok = botcorps_cyd_has_valid_cal();   /* if true, display begin already applied it from NVS */
    Serial.printf("[live] CALbot: saved cal %s\n", cal_ok ? "valid (cv=3) - hold 1 s to redo" : "missing - capture 4 corners");
    lv_begin(&s_radio, ui_blit, ui_raw_draw, th, millis(), cal_ok);
    /* item 3: draw frame 0 of the real (themed) first screen, THEN raise the backlight, so the panel turns on
     * already showing the correct themed frame instead of a black fill or a legacy/unthemed intro. */
    lv_use_pal();
    g_cyd.startWrite();
    lv_tick(s_io, millis());
    ui_live_end();
    g_cyd.setBrightness(255);
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
        ui_live_end();
        Serial.printf("[live] tap %d,%d scr=%d -> %s\n", x, y, (int)lv_scr, a ? a : "-");
    }
    if (lv_scr != LV_SC_CAL) lv_touch(s_tdown, s_tx, s_ty);   /* radar: hold a tower ~1 s to lock on */
    const uint32_t now = millis();
    if ((int32_t)(now - next) < 0) { delay(1); return; }
    // Advance the deadline, not completion time; bound catch-up after a slow frame.
    if ((int32_t)(now - next) >= 30) next = now;
    next += 30;
    lv_use_pal();
    g_cyd.startWrite();
    const uint32_t t_us = micros();
    lv_tick(s_io, now);
    ui_live_end();
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
