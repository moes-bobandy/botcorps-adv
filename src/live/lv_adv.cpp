#if defined(BOTCORPS_ADV_LAND) && BOTCORPS_ADV_LAND
/*
 * Adv landscape dual glue. Same livescan UI and radios as cyd_livescan
 * (c4f3274): Wi-Fi scan, NimBLE scan, passive deauth-frame count.
 * Nothing is transmitted. The external panel is the landscape plate;
 * the internal panel is the status strip. Touch cal is not used —
 * Cardputer Adv is keyboard-only. ORIENT does not rotate the glass.
 */
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_heap_caps.h>
#include <esp_system.h>
#include <NimBLEDevice.h>
#include <M5Cardputer.h>
#include "display/botcorps_display.h"
#include "live/lv_core.h"
#include "live/lv_land.h"
extern "C" {
#include "miniz.h"
}
extern "C" const uint8_t lk_blob_live[];
extern "C" const uint8_t lk_blob_live_end[];

static tinfl_decompressor s_infl;
static uint8_t s_buf[LK_INFL_CAP];
static uint16_t s_line[240];
static const LkIo *s_io;

static int lv_infl(const uint8_t *src, uint32_t len, uint8_t *dst, uint32_t cap)
{
    tinfl_init(&s_infl);
    size_t in = len, out = cap;
    tinfl_status st = tinfl_decompress(&s_infl, src, &in, dst, dst, &out, TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    return st == TINFL_STATUS_DONE ? (int)out : -1;
}

static void lv_adv_raw(int y, int x, int n, const uint8_t *idx)
{
    const uint16_t *pal = LK_CLIP[lk_clip_of_seg(lv_play.seg)].pal;
    for (int i = 0; i < n && i < 240; ++i) s_line[i] = pal[idx[i]];
    lv_land_blit_port(x, y, n > 240 ? 240 : n, 1, s_line);
}

static void lv_adv_blit(int x, int y, int w, int h, const uint16_t *px)
{
    lv_land_blit_port(x, y, w, h, px);
}

static void *lv_adv_alloc(size_t n)
{
    void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!p) p = heap_caps_malloc(n, MALLOC_CAP_8BIT);
    return p;
}

/* ---------------- radio jobs (one at a time; receive only) ---------------- */
enum { JOB_NONE, JOB_WIFI, JOB_BLE, JOB_DEAUTH };
static int s_job = JOB_NONE;
static uint32_t s_job_end = 0;

static void heap_log(const char *tag)
{
    Serial.printf("[adv] heap %s free=%u min=%u largest=%u\n", tag, (unsigned)ESP.getFreeHeap(),
                  (unsigned)ESP.getMinFreeHeap(), (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
}

static void ble_off(void);
static bool wifi_up(const char *tag)
{
    if (NimBLEDevice::isInitialized()) ble_off();
    WiFi.persistent(false);
    if (WiFi.getMode() != WIFI_STA && !WiFi.mode(WIFI_STA)) { Serial.printf("[adv] %s: WiFi.mode(STA) failed\n", tag); heap_log(tag); return false; }
    WiFi.disconnect(false, false);
    wifi_mode_t m = WIFI_MODE_NULL;
    const esp_err_t e = esp_wifi_get_mode(&m);
    if (e != ESP_OK || m != WIFI_MODE_STA) {
        Serial.printf("[adv] %s: esp_wifi_get_mode err=0x%x (%s) mode=%d\n", tag, (unsigned)e, esp_err_to_name(e), (int)m);
        heap_log(tag);
        return false;
    }
    return true;
}
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
    if (!wifi_up("wifi-start")) { wifi_off(); return -2; }
    if (WiFi.scanNetworks(true, true) == WIFI_SCAN_FAILED) { Serial.println("[adv] wifi-start: scanNetworks FAILED"); heap_log("wifi-fail"); wifi_off(); return -1; }
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
        f.auth = (uint8_t)WiFi.encryptionType(i);
        f.kind = LV_K_WIFI;
        f.mfg = 0xFFFF;
        ++n;
    }
    Serial.printf("[adv] wifi scan done rc=%d kept=%d\n", sc, n);
    wifi_off();
    s_job = JOB_NONE;
    heap_log("wifi-post");
    return n;
}
static int r_ble_start(uint32_t ms)
{
    if (s_job != JOB_NONE) r_stop();
    heap_log("ble-pre");
    if (WiFi.getMode() != WIFI_OFF) wifi_off();
    if (!NimBLEDevice::isInitialized() && !NimBLEDevice::init("")) { Serial.println("[adv] ble-start: init FAILED"); heap_log("ble-fail"); return -2; }
    NimBLEScan *sc = NimBLEDevice::getScan();
    sc->setActiveScan(true);
    sc->setInterval(100);
    sc->setWindow(99);
    sc->setMaxResults(LV_MAXF);
    if (!sc->start(ms, false, true)) { Serial.println("[adv] ble-start: scan start FAILED"); heap_log("ble-fail"); ble_off(); return -1; }
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
        const uint8_t *a = d->getAddress().getVal();
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
    Serial.printf("[adv] ble scan done seen=%d kept=%d\n", (int)r.getCount(), n);
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
    if (!wifi_up("deauth-start")) { wifi_off(); return -2; }
    s_deauth = 0; s_mgmt = 0; s_ch = 1; s_hop = millis() + 250;
    wifi_promiscuous_filter_t flt = {.filter_mask = WIFI_PROMIS_FILTER_MASK_MGMT};
    esp_wifi_set_promiscuous_filter(&flt);
    esp_wifi_set_promiscuous_rx_cb(promisc_cb);
    const esp_err_t pe = esp_wifi_set_promiscuous(true);
    if (pe != ESP_OK) { Serial.printf("[adv] deauth-start: promiscuous err=0x%x (%s)\n", (unsigned)pe, esp_err_to_name(pe)); heap_log("deauth-fail"); wifi_off(); return -1; }
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
    Serial.printf("[adv] deauth watch done deauth=%u mgmt=%u\n", (unsigned)s_deauth, (unsigned)s_mgmt);
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
        snprintf(out, n, "adv landscape keyboard");
        break;
    }
}
static void r_save_theme(int t)
{
    Preferences prefs;
    if (prefs.begin("botcorps", false)) { prefs.putUChar("lktheme", (uint8_t)t); prefs.end(); }
}
static bool r_cal_save(const uint16_t *)
{
    /* No touch glass on Adv. CALbot is not on the boot path. */
    return false;
}
static void r_set_rot(int flip)
{
    /* Standing rule: Adv stays landscape. The ORIENT chip still toggles its label. */
    Serial.printf("[adv] ORIENT request %s ignored (landscape locked)\n", flip ? "FLIP" : "PORT");
}
static int r_mac_load(const char *key, uint8_t *out, int max)
{
    Preferences prefs;
    if (!prefs.begin("botcorps", true)) return 0;
    const size_t n = prefs.isKey(key) ? prefs.getBytes(key, out, (size_t)max * 6) : 0;
    prefs.end();
    return (int)(n / 6);
}
static void r_mac_save(const char *key, const uint8_t *macs, int n)
{
    Preferences prefs;
    if (!prefs.begin("botcorps", false)) return;
    if (n > 0) prefs.putBytes(key, macs, (size_t)n * 6);
    else prefs.remove(key);
    prefs.end();
    Serial.printf("[adv] NVS %s = %d MACs\n", key, n);
}

static const LvRadio s_radio{r_wifi_start, r_wifi_poll, r_ble_start, r_ble_poll, r_deauth_start, r_deauth_poll,
                             r_stop, r_sys, r_save_theme, r_cal_save, r_set_rot, r_mac_load, r_mac_save};
static LkIo s_io_obj;

static const char *scr_name(int s)
{
    switch (s) {
    case LV_SC_BOOT: return "BOOT";
    case LV_SC_HOME_INTRO: return "INTRO";
    case LV_SC_HOME: return "HOME";
    case LV_SC_TERM: return "TERM";
    case LV_SC_SCAN: return "SCAN";
    case LV_SC_CTL: return "CONTROL";
    case LV_SC_TTY: return "TTY";
    case LV_SC_RADAR: return "RADAR";
    default: return "?";
    }
}

static void lv_adv_push(void)
{
    if (!botcorps_dual_ok() || !lv_land_px) return;
    auto &ext = botcorps_content();
    ext.startWrite();
    ext.pushImage(0, 0, LV_LAND_W, LV_LAND_H, (const lgfx::rgb565_t *)lv_land_px);
    ext.endWrite();
}

static void lv_adv_menu(void)
{
    auto &m = botcorps_menu();
    m.startWrite();
    m.fillScreen(0x0000);
    m.setFont(&fonts::Font0);
    m.setTextSize(1);
    m.setTextColor(0xFFFF, 0x0000);
    m.setCursor(4, 4);
    m.printf("BOTcorps  %s", scr_name(lv_scr));
    m.setCursor(4, 16);
    m.printf("theme %s", LV_THEME_NAME[lv_theme]);
    m.setCursor(4, 32);
    char line[40];
    snprintf(line, sizeof line, "%s", lv_tk_msg);
    m.print(line);
    m.setCursor(4, 56);
    m.printf("%s%s", LV_PROMPT, lv_inb);
    m.setCursor(4, 84);
    m.print("Fn ;. up/dn   Fn ,/ left/right");
    m.setCursor(4, 96);
    m.print("Enter open    Fn ` back");
    m.setCursor(4, 108);
    m.print("Tab help      type opens TTY");
    m.setCursor(4, 122);
    m.printf("focus %d", lv_land_focus());
    m.endWrite();
}

static Point2D_t s_kprev[16];
static int s_kprev_n;

static void lv_adv_keys(void)
{
    const auto &kl = M5Cardputer.Keyboard.keyList();
    int fresh = -1;
    bool fn = false, shift = false;
    for (int i = 0; i < (int)kl.size(); ++i) {
        const uint8_t v = (uint8_t)M5Cardputer.Keyboard.getKeyValue(kl[i]).value_first;
        if (v == KEY_FN) { fn = true; continue; }
        if (v == KEY_LEFT_SHIFT) { shift = true; continue; }
        if (v == KEY_LEFT_CTRL || v == KEY_OPT || v == KEY_LEFT_ALT) continue;
        bool held = false;
        for (int j = 0; j < s_kprev_n; ++j) if (s_kprev[j] == kl[i]) { held = true; break; }
        if (!held) fresh = i;
    }
    s_kprev_n = 0;
    for (const auto &p : kl) if (s_kprev_n < 16) s_kprev[s_kprev_n++] = p;
    if (fresh < 0) return;
    const KeyValue_t kv = M5Cardputer.Keyboard.getKeyValue(kl[fresh]);
    const uint8_t code = (uint8_t)kv.value_first;
    const char *act = "none";
    if (code == KEY_ENTER) act = lv_land_nav(LV_NAV_OK, 0);
    else if (code == KEY_BACKSPACE) act = lv_land_nav(LV_NAV_BACK, 0);
    else if (code == KEY_TAB) act = lv_land_nav(LV_NAV_TAB, 0);
    else if (fn) {
        if (code == ';') act = lv_land_nav(LV_NAV_UP, 0);
        else if (code == '.') act = lv_land_nav(LV_NAV_DOWN, 0);
        else if (code == ',') act = lv_land_nav(LV_NAV_LEFT, 0);
        else if (code == '/') act = lv_land_nav(LV_NAV_RIGHT, 0);
        else if (code == '`') act = lv_land_nav(LV_NAV_BACK, 0);
    } else {
        const char c = shift ? kv.value_second : kv.value_first;
        if (c >= 32 && c < 127) act = lv_land_nav(LV_NAV_CHAR, c);
    }
    Serial.printf("[adv] key %u fn=%d -> %s scr=%d focus=%d\n", (unsigned)code, fn ? 1 : 0, act, lv_scr, lv_land_focus());
}

void lv_adv_begin(void)
{
    if (!lv_land_init(lv_adv_alloc)) {
        Serial.println("[adv] landscape buffers failed");
        return;
    }
    int th = 0;
    Preferences prefs;
    if (prefs.begin("botcorps", true)) { th = prefs.getUChar("lktheme", 0); prefs.end(); }
    s_io_obj = {lk_blob_live, lv_infl, lv_clip_draw, s_buf};
    s_io = &s_io_obj;
    Serial.printf("[adv] landscape dual blob=%u theme=%d\n", (unsigned)(lk_blob_live_end - lk_blob_live), th);
    heap_log("begin");
    lv_begin(&s_radio, lv_adv_blit, lv_adv_raw, th, millis(), true);
    lv_go(LV_SC_BOOT); /* keyboard device: skip CALbot */
    lv_tick(*s_io, millis());
    lv_land_compose(1);
    lv_adv_push();
    lv_adv_menu();
}

void lv_adv_loop(void)
{
    if (!s_io || !lv_land_px) { delay(10); return; }
    lv_adv_keys();
    static uint32_t next = 0;
    const uint32_t now = millis();
    if ((int32_t)(now - next) < 0) { delay(1); return; }
    next = now + 30;
    lv_tick(*s_io, now);
    lv_land_compose(1);
    lv_adv_push();
    lv_adv_menu();
}
#endif
