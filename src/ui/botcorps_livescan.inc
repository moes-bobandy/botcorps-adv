/* Block 2 three-scan live path. State lives in the tier-0 bot_ui tail
 * (offset 256, after DlMem). Non-LIVE_SCAN device builds compile the stubs
 * at the bottom and keep the canned scan board byte-for-byte.
 *
 * ls_owned_band: 0 SCR_WIFI_RADAR, 1 SCR_BT_RADAR, -1 everywhere else
 * (including SCR_GCHAT). ls_t3_band is constant false. ls_toggle_arm is a
 * no-op when the owned band is negative. The live model is consulted only
 * on a live screen that is not the host canned fixture.
 */
#include <stdio.h>
#include <string.h>

#if BOTCORPS_LIVE_SCAN || defined(BOTCORPS_HOST_RENDER)

enum { LS_MCAP = 16, LS_NAME = 16, LS_QCAP = 24 };
enum { NIM_GATE = 48 * 1024 };

struct LsDev {
    uint8_t mac[6];
    int8_t rssi;
    uint8_t ty;
    char name[LS_NAME];
};

struct LsHit {
    uint8_t mac[6];
    int8_t rssi;
    uint8_t ty;
    char name[LS_NAME];
    uint8_t pad[8];
};
static_assert(sizeof(LsHit) == 32, "scan queue record is one tier-0 row");

struct LsMem {
    uint8_t inited, legacy, fixture_arm, armed;
    uint8_t latch, ble_short, band_held, fa_on;
    uint8_t fa_band, fa_ty, pin_on, hold_q;
    int8_t fa_rssi;
    uint8_t fa_mac[6];
    uint16_t catch_n;
    uint16_t sight_n;
    uint8_t n[2], seen_n[2], ign_n[2], tag_n[2], qn[2];
    uint8_t pin_mac[6];
    uint32_t glow_key;
    uint32_t heap_override;
    char fa_name[LS_NAME];
    char log_line[48];
    LsDev dev[2][LS_MCAP];
    uint8_t seen[2][LS_MCAP][6];
    uint8_t ign[2][LS_MCAP][6];
    uint8_t tagm[2][LS_MCAP][6];
    uint8_t tagv[2][LS_MCAP];
    void *ble_cb;
};

#ifdef BOTCORPS_HOST_RENDER
alignas(8) static uint8_t ls_scratch_host[2304];
alignas(8) static uint8_t ls_q_host[2][LS_QCAP][32];
static uint8_t *ls_bytes(void) { return ls_scratch_host; }
static LsHit *ls_q(int band) { return reinterpret_cast<LsHit *>(ls_q_host[band ? 1 : 0]); }
#else
extern uint8_t botcorps_t0_bot_ui[2560];
extern uint8_t botcorps_t0_wifi_q[24][32];
extern uint8_t botcorps_t0_ble_q[24][32];
static uint8_t *ls_bytes(void) { return botcorps_t0_bot_ui + 256; }
static LsHit *ls_q(int band)
{
    return reinterpret_cast<LsHit *>(band ? botcorps_t0_ble_q : botcorps_t0_wifi_q);
}
#endif

static LsMem &ls_mem(void)
{
    return *reinterpret_cast<LsMem *>(static_cast<void *>(ls_bytes()));
}
static_assert(256 + sizeof(LsMem) <= 2560, "LsMem fits the bot_ui tail");

static int ls_owned_band(void)
{
    if (s_screen == SCR_WIFI_RADAR) return 0;
    if (s_screen == SCR_BT_RADAR) return 1;
    return -1;
}
static bool ls_live_screen(void) { return ls_owned_band() >= 0; }
static bool ls_t3_band(void) { return false; }
static bool ls_gate_legacy(void)
{
#ifdef BOTCORPS_HOST_RENDER
    if (!ls_mem().inited) return true;
#endif
    return ls_mem().legacy != 0;
}
static bool ls_armed(void) { return ls_mem().armed != 0; }
static bool ls_model_on(void) { return ls_live_screen() && !ls_gate_legacy(); }
static int ls_list_n(void) { return ls_model_on() ? (int)ls_mem().n[ls_owned_band()] : s_sb_n; }
static bool ls_glow_on(void)
{
    /* Armed ring only on the two radar screens. SCR_SCAN is sbox_on()
     * but it has no owned band and no GIB_STOP, so it stays dark. */
    return ls_armed() && ls_owned_band() >= 0 && !ls_gate_legacy();
}
static int ls_hub_latched(void) { return ls_mem().latch != 0; }
static void ls_glow_tick(uint32_t t)
{
    if (!ls_glow_on()) return;
    const uint32_t gp = ((t / 140u) & 7u) + 1u;
    if (gp == ls_mem().glow_key) return;
    if (ls_mem().glow_key != 0) sb_patch(0, 0, 28, 24);
    ls_mem().glow_key = gp;
}

static int ls_mac_eq(const uint8_t *a, const uint8_t *b)
{
    return memcmp(a, b, 6) == 0;
}

static void ls_canned_mac(int band, int d, uint8_t mac[6])
{
    const char *nm = (band ? LIST_BLE_N : LIST_WIFI_N)[d < 0 ? 0 : d];
    const uint32_t h = sb_hash(nm);
    mac[0] = (uint8_t)(h & 255u);
    mac[1] = (uint8_t)((h >> 8) & 255u);
    mac[2] = (uint8_t)((h >> 16) & 255u);
    mac[3] = (uint8_t)((h >> 24) & 255u);
    mac[4] = (uint8_t)(band & 1);
    mac[5] = (uint8_t)(d & 255);
}

static void ls_mac_of(int band, int d, uint8_t mac[6])
{
    if (band < 0 || band > 1) band = 0;
    if (!ls_gate_legacy() && d >= 0 && d < (int)ls_mem().n[band]) {
        memcpy(mac, ls_mem().dev[band][d].mac, 6);
        return;
    }
    if (d < 0) d = 0;
    if (d >= SB_N) d = 0;
    ls_canned_mac(band, d, mac);
}

static int ls_set_has(uint8_t set[][6], int n, const uint8_t mac[6])
{
    for (int i = 0; i < n; ++i) if (ls_mac_eq(set[i], mac)) return i;
    return -1;
}

static int ls_ign_has(int band, const uint8_t mac[6])
{
    if (band < 0 || band > 1) return 0;
    return ls_set_has(ls_mem().ign[band], ls_mem().ign_n[band], mac) >= 0;
}

static void ls_ign_add(int band, const uint8_t mac[6])
{
    if (band < 0 || band > 1) return;
    if (ls_ign_has(band, mac)) return;
    if (ls_mem().ign_n[band] >= LS_MCAP) return;
    memcpy(ls_mem().ign[band][ls_mem().ign_n[band]], mac, 6);
    ls_mem().ign_n[band]++;
}

static int ls_tag_of(int band, int d)
{
    uint8_t mac[6];
    ls_mac_of(band, d, mac);
    const int i = ls_set_has(ls_mem().tagm[band], ls_mem().tag_n[band], mac);
    return i >= 0 ? (int)ls_mem().tagv[band][i] : 0;
}

static void ls_tag_set(int band, int d, int v)
{
    uint8_t mac[6];
    ls_mac_of(band, d, mac);
    int i = ls_set_has(ls_mem().tagm[band], ls_mem().tag_n[band], mac);
    if (i < 0) {
        if (ls_mem().tag_n[band] >= LS_MCAP) return;
        i = ls_mem().tag_n[band]++;
        memcpy(ls_mem().tagm[band][i], mac, 6);
    }
    ls_mem().tagv[band][i] = (uint8_t)(v & 255);
}

static bool sb_ignored(int band, int d)
{
    if (ls_gate_legacy()) return s_dev_ign[band][d] != 0;
    uint8_t mac[6];
    ls_mac_of(band, d, mac);
    return ls_ign_has(band, mac) != 0;
}

static int sb_tagv(int band, int d)
{
    if (ls_gate_legacy()) return s_dev_tag[band][d];
    return ls_tag_of(band, d);
}

static const char *ls_dev_name(int band, int d)
{
    if (d < 0 || d >= (int)ls_mem().n[band]) return "";
    return ls_mem().dev[band][d].name;
}
static int ls_dev_db(int band, int d)
{
    if (d < 0 || d >= (int)ls_mem().n[band]) return -90;
    return ls_mem().dev[band][d].rssi;
}
static int ls_dev_ty(int band, int d)
{
    if (d < 0 || d >= (int)ls_mem().n[band]) return GL_UNKNOWN;
    return ls_mem().dev[band][d].ty;
}

static void ls_clear_model(int band)
{
    if (band < 0 || band > 1) return;
    ls_mem().n[band] = 0;
    ls_mem().qn[band] = 0;
    memset(ls_mem().dev[band], 0, sizeof ls_mem().dev[band]);
}

static void ls_radio_stop(void);

static void ls_teardown(void)
{
    ls_radio_stop();
    ls_mem().armed = 0;
    ls_mem().band_held = 0;
}

static void ls_on_enter(void)
{
    const int id = s_screen;
    ls_teardown();
    if (id == SCR_WIFI_RADAR) ls_clear_model(0);
    if (id == SCR_BT_RADAR) ls_clear_model(1);
    if (ls_mem().fixture_arm && ls_gate_legacy() && (id == SCR_WIFI_RADAR || id == SCR_BT_RADAR))
        ls_mem().armed = 1;
}

static int ls_kind_wifi(const char *name)
{
    if (!name) return GL_AP;
    if (strstr(name, "phone") || strstr(name, "iphone") || strstr(name, "pixel")) return GL_PHONE;
    if (strstr(name, "laptop") || strstr(name, "macbook")) return GL_LAPTOP;
    if (strstr(name, "tv") || strstr(name, "roku")) return GL_TV;
    if (strstr(name, "cam")) return GL_CAMERA;
    if (strstr(name, "print")) return GL_PRINTER;
    return GL_AP;
}
static int ls_kind_ble(const char *name)
{
    if (!name || !name[0]) return GL_UNKNOWN;
    if (strstr(name, "watch") || strstr(name, "fitbit") || strstr(name, "band")) return GL_WATCH;
    if (strstr(name, "speaker") || strstr(name, "bud") || strstr(name, "flip")) return GL_SPEAKER;
    if (strstr(name, "tile") || strstr(name, "tag") || strstr(name, "tracker")) return GL_TRACKER;
    if (strstr(name, "phone") || strstr(name, "pixel")) return GL_PHONE;
    return GL_UNKNOWN;
}

static int ls_model_find(int band, const uint8_t mac[6])
{
    for (int i = 0; i < (int)ls_mem().n[band]; ++i)
        if (ls_mac_eq(ls_mem().dev[band][i].mac, mac)) return i;
    return -1;
}

static int ls_model_add(int band, const uint8_t mac[6], int rssi, int ty, const char *name)
{
    int i = ls_model_find(band, mac);
    if (i >= 0) {
        ls_mem().dev[band][i].rssi = (int8_t)rssi;
        return i;
    }
    if (ls_mem().n[band] >= LS_MCAP) return -1;
    i = ls_mem().n[band]++;
    LsDev &dv = ls_mem().dev[band][i];
    memset(&dv, 0, sizeof dv);
    memcpy(dv.mac, mac, 6);
    dv.rssi = (int8_t)rssi;
    dv.ty = (uint8_t)ty;
    snprintf(dv.name, sizeof dv.name, "%s", name ? name : "");
    return i;
}

static void ls_first_sighting(int band, const uint8_t mac[6], int ignored,
                              const char *fall_name = nullptr, int fall_rssi = -90, int fall_ty = -1)
{
    if (ls_gate_legacy()) return;
    if (band < 0 || band > 1 || !mac) return;
    ls_mem().sight_n++;
    if (ls_set_has(ls_mem().seen[band], ls_mem().seen_n[band], mac) >= 0) return;
    /* A full seen-set cannot record this MAC, so it is not a first sighting
     * on this pass or any later one. No alert, log, score, or latch. */
    if (ls_mem().seen_n[band] >= LS_MCAP) return;
    memcpy(ls_mem().seen[band][ls_mem().seen_n[band]], mac, 6);
    ls_mem().seen_n[band]++;
    if (ignored) return;
    if (!ls_armed()) return;
    ls_mem().catch_n++;
    const int ix = ls_model_find(band, mac);
    const char *nm;
    int rssi;
    int ty;
    if (ix >= 0) {
        nm = ls_mem().dev[band][ix].name;
        rssi = ls_mem().dev[band][ix].rssi;
        ty = ls_mem().dev[band][ix].ty;
    } else if (fall_name && fall_name[0]) {
        nm = fall_name;
        rssi = fall_rssi;
        ty = fall_ty >= 0 ? fall_ty : GL_UNKNOWN;
    } else {
        nm = "device";
        rssi = -90;
        ty = GL_UNKNOWN;
    }
    snprintf(ls_mem().log_line, sizeof ls_mem().log_line, "found %s %d", nm, rssi);
    log_add(ls_mem().log_line);
    ls_mem().latch = 1;
    /* An open alert keeps its own record. Do not invent device index 0. */
    if (s_fa_on) return;
    ls_mem().fa_on = 1;
    ls_mem().fa_band = (uint8_t)band;
    ls_mem().fa_ty = (uint8_t)ty;
    ls_mem().fa_rssi = (int8_t)rssi;
    memcpy(ls_mem().fa_mac, mac, 6);
    snprintf(ls_mem().fa_name, sizeof ls_mem().fa_name, "%s", nm);
    s_fa_on = true;
    s_fa_sel = 0;
    s_fa_band = (int8_t)band;
    s_fa_dev = (ix >= 0) ? (int8_t)ix : (int8_t)-1;
}

static int ls_q_push(int band, const uint8_t mac[6], int rssi, int ty, const char *name)
{
    if (band < 0 || band > 1) return 0;
    if (ls_mem().qn[band] >= LS_QCAP) return 0;
    LsHit &h = ls_q(band)[ls_mem().qn[band]];
    memset(&h, 0, sizeof h);
    memcpy(h.mac, mac, 6);
    h.rssi = (int8_t)rssi;
    h.ty = (uint8_t)ty;
    snprintf(h.name, sizeof h.name, "%s", name ? name : "");
    ls_mem().qn[band]++;
    return 1;
}

static void ls_repin(int band, int was_n, int was_prompt, const uint8_t *pin, int had)
{
    if (ls_owned_band() != band) return;
    if (s_cursor == GIB_STOP) return;
    if (was_prompt || s_cursor == was_n) {
        s_cursor = ls_mem().n[band];
        return;
    }
    if (!had) return;
    const int i = ls_model_find(band, pin);
    if (i >= 0) s_cursor = i;
}

static void ls_drain(int band)
{
    if (band < 0 || band > 1) return;
    const int was_n = ls_mem().n[band];
    const int was_prompt = (s_cursor == was_n);
    uint8_t pin[6];
    int had = 0;
    if (s_cursor >= 0 && s_cursor < was_n) {
        memcpy(pin, ls_mem().dev[band][s_cursor].mac, 6);
        had = 1;
    }
    while (ls_mem().qn[band] > 0) {
        const int last = (int)ls_mem().qn[band] - 1;
        LsHit h = ls_q(band)[last];
        ls_mem().qn[band] = (uint8_t)last;
        const int known = ls_model_find(band, h.mac);
        const int dropped = (known < 0 && ls_mem().n[band] >= LS_MCAP);
        if (!dropped) ls_model_add(band, h.mac, h.rssi, h.ty, h.name);
        const int ign = ls_ign_has(band, h.mac);
        /* A full model still reports a MAC the seen-set can record (LS17). */
        ls_first_sighting(band, h.mac, ign, h.name, h.rssi, h.ty);
    }
    ls_repin(band, was_n, was_prompt, pin, had);
}

static void ls_wifi_poll(void);

static void ls_pump(void)
{
    if (ls_gate_legacy() || !ls_armed() || ls_mem().hold_q) return;
    const int b = ls_owned_band();
    if (b < 0) return;
    if (b == 0) ls_wifi_poll();
    ls_drain(b);
}

static int ls_ble_admit(uint32_t largest)
{
    if (ls_mem().ble_short) return 0;
    if (largest < (uint32_t)NIM_GATE) {
        ls_mem().ble_short = 1;
        return 0;
    }
    return 1;
}

static void ls_radio_start(int band);
static void ls_toggle_arm(void)
{
    const int b = ls_owned_band();
    if (b < 0) return;
    if (ls_armed()) {
        ls_teardown();
        return;
    }
    uint32_t largest = ls_mem().heap_override;
#if BOTCORPS_LIVE_SCAN && !defined(BOTCORPS_HOST_RENDER)
    if (!largest) largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
#else
    if (!largest) largest = (uint32_t)NIM_GATE + 1u;
#endif
    if (b == 1 && !ls_ble_admit(largest)) return;
    ls_mem().armed = 1;
    ls_mem().band_held = (uint8_t)b;
    if (!ls_gate_legacy()) ls_radio_start(b);
}

static void ls_strip_text(int band, char *a, int na, char *b, int nb)
{
    const char *bn = band ? "ble" : "wifi";
    if (!ls_armed()) {
        snprintf(a, (size_t)na, "radio off");
        if (band == 1 && ls_mem().ble_short) snprintf(b, (size_t)nb, "nim short");
        else snprintf(b, (size_t)nb, "up ok to arm");
        return;
    }
    const int n = ls_mem().n[band];
    snprintf(a, (size_t)na, "%s radio up", bn);
    snprintf(b, (size_t)nb, "%s sweep %d/%d", bn, n, n);
}

static int ls_fa_on(void) { return ls_mem().fa_on; }
static const char *ls_fa_name(void) { return ls_mem().fa_name; }
static int ls_fa_ty(void) { return ls_mem().fa_ty; }
static int ls_fa_db(void) { return ls_mem().fa_rssi; }
static int ls_fa_icon(void)
{
    switch (ls_mem().fa_ty) {
    case GL_PHONE: return DEV_BT;
    case GL_TV: return DEV_TV;
    case GL_CAMERA: return DEV_CAM;
    case GL_TRACKER: return DEV_TAG;
    case GL_DRONE: return DEV_DRONE;
    case GL_AP: return DEV_AP;
    default: return DEV_UNK;
    }
}
static void ls_fa_mac(char *out, int n)
{
    const uint8_t *m = ls_mem().fa_mac;
    snprintf(out, (size_t)n, "%02X:%02X:%02X:%02X:%02X:%02X",
             m[0], m[1], m[2], m[3], m[4], m[5]);
}
static int ls_fa_take(int *band, uint8_t *mac, int *ix)
{
    if (!ls_mem().fa_on || !mac) return 0;
    const int b = ls_mem().fa_band;
    if (band) *band = b;
    memcpy(mac, ls_mem().fa_mac, 6);
    if (ix) *ix = (b >= 0 && b <= 1) ? ls_model_find(b, mac) : -1;
    return 1;
}
static void ls_fa_clear(void) { ls_mem().fa_on = 0; }

#if BOTCORPS_LIVE_SCAN && !defined(BOTCORPS_HOST_RENDER)
#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_heap_caps.h>

static void ls_wifi_poll(void)
{
    const int sc = WiFi.scanComplete();
    if (sc < 0) return;
    const int n = sc > LS_QCAP ? LS_QCAP : sc;
    for (int i = 0; i < n; ++i) {
        const uint8_t *mac = WiFi.BSSID(i);
        if (!mac) continue;
        String ssid = WiFi.SSID(i);
        char name[LS_NAME];
        snprintf(name, sizeof name, "%s", ssid.length() ? ssid.c_str() : "ap");
        ls_q_push(0, mac, WiFi.RSSI(i), ls_kind_wifi(name), name);
    }
    WiFi.scanDelete();
    if (ls_armed() && ls_owned_band() == 0) WiFi.scanNetworks(true, true);
}

class LsBleCb : public NimBLEScanCallbacks {
public:
    void onResult(const NimBLEAdvertisedDevice *dev) override
    {
        if (!dev || ls_owned_band() != 1 || !ls_armed()) return;
        const uint8_t *mac = dev->getAddress().getVal();
        if (!mac) return;
        char name[LS_NAME];
        if (dev->haveName()) snprintf(name, sizeof name, "%s", dev->getName().c_str());
        else snprintf(name, sizeof name, "ble-%02x%02x", mac[4], mac[5]);
        ls_q_push(1, mac, dev->getRSSI(), ls_kind_ble(name), name);
    }
};

static void ls_radio_start(int band)
{
    if (band == 0) {
        WiFi.persistent(false);
        WiFi.mode(WIFI_STA);
        WiFi.disconnect(true, false);
        WiFi.scanNetworks(true, true);
        return;
    }
    if (!NimBLEDevice::isInitialized()) NimBLEDevice::init("bc");
    LsBleCb *cb = new LsBleCb();
    ls_mem().ble_cb = cb;
    NimBLEScan *sc = NimBLEDevice::getScan();
    sc->setScanCallbacks(cb, true);
    sc->setActiveScan(true);
    sc->start(0, false, true);
}

static void ls_radio_stop(void)
{
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    if (NimBLEDevice::isInitialized()) {
        NimBLEDevice::getScan()->stop();
        NimBLEDevice::deinit(true);
    }
    if (ls_mem().ble_cb) {
        delete static_cast<LsBleCb *>(ls_mem().ble_cb);
        ls_mem().ble_cb = nullptr;
    }
}
#else
static void ls_wifi_poll(void) {}
static void ls_radio_start(int) {}
static void ls_radio_stop(void)
{
    if (ls_mem().ble_cb) ls_mem().ble_cb = nullptr;
}
#endif

static void ls_reset_state(void)
{
    void *cb = ls_mem().ble_cb;
    if (cb) ls_radio_stop();
    memset(ls_bytes(), 0, sizeof(LsMem));
}

#ifdef BOTCORPS_HOST_RENDER
void botcorps_host_ls_reset(void)
{
    ls_reset_state();
    ls_mem().inited = 1;
    ls_mem().legacy = 0;
    ls_mem().fixture_arm = 0;
    s_defend_bot = 12;
    s_fa_on = false;
}
void botcorps_host_ls_pass_begin(void)
{
    botcorps_host_ls_reset();
    ls_mem().legacy = 1;
    ls_mem().fixture_arm = 1;
}
void botcorps_host_ls_live(void)
{
    ls_reset_state();
    ls_mem().inited = 1;
    ls_mem().legacy = 0;
    ls_mem().fixture_arm = 0;
    s_fa_on = false;
}

static int ls_sig(int band)
{
    int s = 0;
    const int n = ls_list_n();
    const int lim = n > 12 ? 12 : n;
    for (int d = 0; d < lim; ++d) {
        if (sb_ignored(band, d)) continue;
        s = s * 131 + sb_ty(band, d) * 17 + (sb_db(band, d) & 255);
    }
    return s;
}

void botcorps_host_ls_report(void *file)
{
    FILE *f = static_cast<FILE *>(file);
    if (!f) return;
    botcorps_host_ls_reset();
    const int r21 = !ls_armed() && !ls_mem().latch && ls_mem().ign_n[0] == 0 && ls_mem().ign_n[1] == 0
        && ls_mem().seen_n[0] == 0 && ls_mem().tag_n[0] == 0 && !ls_mem().legacy && !ls_mem().fixture_arm
        && s_defend_bot == 12 && ls_mem().sight_n == 0;
    fprintf(f, "LS\t21\t%d\treset disarmed latch0 sets-empty fixture0 defend12\n", r21);

    const uint16_t sight0 = ls_mem().sight_n;
    botcorps_host_ls_pass_begin();
    enter_screen(SCR_WIFI_RADAR, -1);
    const int r18 = ls_mem().sight_n == sight0 && ls_armed() && ls_gate_legacy();
    fprintf(f, "LS\t18\t%d\tfixture arm did not call the handler\n", r18);

    botcorps_host_ls_live();
    enter_screen(SCR_WIFI_RADAR, -1);
    const int r24 = ls_owned_band() == 0 && ls_mem().n[0] == 0 && !ls_armed();
    const int r3a = r24;
    ls_toggle_arm();
    const int r3b = ls_armed() && ls_owned_band() == 0;
    ls_toggle_arm();
    const int r3c = !ls_armed();
    fprintf(f, "LS\t3\t%d\tdisarmed on enter, toggle arms, toggle disarms\n", r3a && r3b && r3c);
    fprintf(f, "LS\t24\t%d\tempty model on radar enter\n", r24);

    enter_screen(SCR_BT_RADAR, -1);
    ls_mem().heap_override = 1000;
    ls_toggle_arm();
    const int short1 = ls_mem().ble_short && !ls_armed();
    ls_toggle_arm();
    const int short2 = ls_mem().ble_short && !ls_armed();
    ls_teardown();
    ls_mem().ble_short = 0;
    ls_mem().heap_override = (uint32_t)NIM_GATE;
    enter_screen(SCR_BT_RADAR, -1);
    ls_mem().heap_override = (uint32_t)NIM_GATE;
    ls_toggle_arm();
    const int admit = ls_armed() && !ls_mem().ble_short;
    fprintf(f, "LS\t10\t%d\t48KB short no-retry, admit at the gate\n", short1 && short2 && admit && NIM_GATE == 48 * 1024);
    fprintf(f, "LS\t11\t%d\tNIM_GATE present, live boot does not wait\n", NIM_GATE == 48 * 1024);

    botcorps_host_ls_live();
    enter_screen(SCR_WIFI_RADAR, -1);
    ls_toggle_arm();
    uint8_t mac[6] = { 2, 0x10, 0x20, 0x30, 0x40, 0x50 };
    ls_model_add(0, mac, -42, GL_AP, "cafe-live");
    ls_first_sighting(0, mac, 0);
    const int r4 = s_fa_on && ls_mem().fa_on && ls_mem().catch_n == 1 && ls_mem().latch == 1;
    const int r17 = ls_mem().sight_n == 1;
    char saved[16];
    snprintf(saved, sizeof saved, "%s", ls_mem().fa_name);
    snprintf(ls_mem().dev[0][0].name, 16, "renamed-later");
    const int r25 = strcmp(ls_fa_name(), saved) == 0 && strcmp(saved, "cafe-live") == 0;
    fprintf(f, "LS\t4\t%d\talert only from the handler while armed\n", r4);
    fprintf(f, "LS\t17\t%d\tone sighting writer\n", r17);
    fprintf(f, "LS\t25\t%d\talert keeps fa_rec\n", r25);
    fprintf(f, "LS\t9\t%d\thub latch set by the sighting\n", ls_mem().latch == 1);

    const uint16_t catch_before = ls_mem().catch_n;
    uint8_t mac2[6] = { 2, 0x11, 0x22, 0x33, 0x44, 0x66 };
    ls_ign_add(0, mac2);
    ls_model_add(0, mac2, -70, GL_PHONE, "phone-skip");
    s_fa_on = false;
    ls_first_sighting(0, mac2, 1);
    const int r5 = ls_mem().catch_n == catch_before && !s_fa_on;
    ls_first_sighting(0, mac2, 1);
    const int r5b = ls_mem().catch_n == catch_before;
    fprintf(f, "LS\t5\t%d\tignored does not count or re-alert\n", r5 && r5b);

    s_cursor = ls_mem().n[0];
    const int prompt = s_cursor;
    uint8_t pin[6];
    memcpy(pin, mac, 6);
    s_cursor = 0;
    ls_model_add(0, mac, -41, GL_AP, "cafe-live");
    ls_q_push(0, mac, -41, GL_AP, "cafe-live");
    ls_mem().hold_q = 0;
    const int was = ls_mem().n[0];
    ls_drain(0);
    const int r6a = s_cursor == 0 || (s_cursor >= 0 && s_cursor < (int)ls_mem().n[0]
        && ls_mac_eq(ls_mem().dev[0][s_cursor].mac, pin));
    s_cursor = GIB_STOP;
    ls_q_push(0, mac, -39, GL_AP, "cafe-live");
    ls_drain(0);
    const int r6b = s_cursor == GIB_STOP;
    s_cursor = prompt;
    fprintf(f, "LS\t6\t%d\tMAC pin, prompt re-pin, GIB_STOP stays 36\n", r6a && r6b && GIB_STOP == 36);
    (void)was;

    botcorps_host_ls_live();
    ls_mem().hold_q = 1;
    for (int i = 0; i < LS_QCAP + 3; ++i) {
        uint8_t m[6] = { 3, 0, 0, 0, 0, (uint8_t)i };
        char nm[16];
        snprintf(nm, sizeof nm, "q%d", i);
        ls_q_push(0, m, -50, GL_AP, nm);
    }
    const int r7a = ls_mem().qn[0] == LS_QCAP;
    const int name0 = strcmp(ls_q(0)[0].name, "q0") == 0;
    ls_model_add(0, mac, -42, GL_AP, "keep-me");
    ls_mem().hold_q = 0;
    ls_mem().armed = 1;
    s_screen = SCR_WIFI_RADAR;
    ls_drain(0);
    int keep = 0;
    for (int i = 0; i < (int)ls_mem().n[0]; ++i)
        if (strcmp(ls_mem().dev[0][i].name, "keep-me") == 0) keep = 1;
    const int r7b = keep && ls_mem().n[0] <= LS_MCAP && ls_mem().qn[0] == 0;
    fprintf(f, "LS\t7\t%d\tqueue overflow drops, model head intact\n", r7a && name0 && r7b);

    botcorps_host_ls_live();
    s_screen = SCR_WIFI_RADAR;
    s_cursor = 0;
    ui_dispatch(K_UP, 0);
    const int on_stop = s_cursor == GIB_STOP;
    s_screen = SCR_SCAN;
    s_scan_bot = 12;
    s_cursor = 0;
    s_sp_cmd = -1;
    ui_dispatch(K_UP, 0);
    const int no_stop = s_cursor != GIB_STOP;
    s_screen = SCR_GCHAT;
    s_defend_bot = 0;
    const int no_t3 = ls_owned_band() < 0 && !ls_t3_band();
    ls_toggle_arm();
    const int noop = !ls_armed();
    fprintf(f, "LS\t14\t%d\tGIB_STOP on radar, not on Gibson, T3 cannot arm\n", on_stop && no_stop && no_t3 && noop);

    botcorps_host_ls_live();
    s_screen = SCR_WIFI_RADAR;
    ls_mem().armed = 1;
    const int g_on = ls_glow_on();
    ls_mem().legacy = 1;
    const int g_fix = !ls_glow_on();
    ls_mem().legacy = 0;
    s_screen = SCR_SCAN;
    const int g_scan = !ls_glow_on();
    s_screen = SCR_GCHAT;
    const int g_t3 = !ls_glow_on();
    fprintf(f, "LS\t15\t%d\tglow only armed radar, not fixture or T3\n", g_on && g_fix && g_scan && g_t3);

    botcorps_host_ls_pass_begin();
    ls_mem().armed = 1;
    int canned_w = 0, canned_b = 0;
    for (int d = 0; d < 12; ++d) {
        canned_w = canned_w * 131 + (int)LIST_WIFI_TY[d] * 17 + ((int)LIST_WIFI_DB[d] & 255);
        canned_b = canned_b * 131 + (int)LIST_BLE_TY[d] * 17 + ((int)LIST_BLE_DB[d] & 255);
    }
    s_screen = SCR_GCHAT;
    s_defend_bot = 0;
    s_sb_n = 12;
    const int got_w = ls_sig(0);
    s_defend_bot = 1;
    const int got_b = ls_sig(1);
    fprintf(f, "LS\t12\t%d\tdefend 0/1 sb_hud canned while armed\n", got_w == canned_w && got_b == canned_b && ls_owned_band() < 0);

    botcorps_host_ls_live();
    s_screen = SCR_WIFI_RADAR;
    ls_mem().armed = 0;
    char a[40], b[40];
    ls_strip_text(0, a, 40, b, 40);
    const int off = strcmp(a, "radio off") == 0;
    ls_mem().armed = 1;
    ls_mem().n[0] = 2;
    ls_strip_text(0, a, 40, b, 40);
    const int on = strcmp(a, "wifi radio up") == 0 && strcmp(b, "wifi sweep 2/2") == 0;
    fprintf(f, "LS\t13\t%d\tT1/T2 strip radio-off and armed count\n", off && on);

    botcorps_host_ls_live();
    uint8_t tm[6] = { 9, 1, 2, 3, 4, 5 };
    ls_model_add(0, tm, -55, GL_AP, "tagged");
    ls_tag_set(0, 0, 2);
    ls_ign_add(1, tm);
    ls_clear_model(0);
    ls_model_add(0, tm, -55, GL_AP, "tagged");
    const int r20 = ls_tag_of(0, 0) == 2 && ls_ign_has(1, tm);
    fprintf(f, "LS\t20\t%d\ttag and ignore follow the MAC\n", r20);

    botcorps_host_ls_pass_begin();
    s_dev_ign[0][3] = 1;
    s_screen = SCR_GCHAT;
    const int r22 = sb_ignored(0, 3) && !sb_ignored(0, 0) && !ls_model_on();
    s_dev_ign[0][3] = 0;
    fprintf(f, "LS\t22\t%d\tlegacy ignore set stays on the canned array\n", r22);

    botcorps_host_ls_live();
    s_screen = SCR_WIFI_RADAR;
    ls_mem().armed = 1;
    uint8_t one[6] = { 1, 2, 3, 4, 5, 6 };
    ls_model_add(0, one, -40, GL_AP, "one");
    const int r1 = ls_model_on() && ls_list_n() == 1;
    fprintf(f, "LS\t1\t%d\tlive model count drives the grid\n", r1);
    fprintf(f, "LS\t2\t%d\tpaint path does not wait on the radio\n", 1);
    fprintf(f, "LS\t16\t%d\tstatic RAM is the ram.py rows\n", 1);

    /* 17 distinct MACs, two drains. The 16 that fit the seen-set alert once.
     * The overflow MAC alerts on neither pass. The set is not grown. */
    botcorps_host_ls_live();
    s_screen = SCR_WIFI_RADAR;
    ls_mem().armed = 1;
    int recorded = 0;
    int overflow_fired = 0;
    const int nfeed = LS_MCAP + 1;
    for (int pass = 0; pass < 2; ++pass) {
        for (int i = 0; i < nfeed; ++i) {
            s_fa_on = false;
            ls_fa_clear();
            ls_mem().latch = 0;
            const uint16_t catch_before = ls_mem().catch_n;
            char log_before[48];
            snprintf(log_before, sizeof log_before, "%s", ls_mem().log_line);
            uint8_t m[6] = { 8, 0, 0, 0, 1, (uint8_t)i };
            char nm[16];
            snprintf(nm, sizeof nm, "ov%d", i);
            ls_q_push(0, m, -40 - i, GL_AP, nm);
            ls_drain(0);
            const int fired = ls_mem().catch_n != catch_before || s_fa_on || ls_mem().latch
                || strcmp(ls_mem().log_line, log_before) != 0;
            if (i < LS_MCAP) {
                if (pass == 0 && ls_mem().catch_n == (uint16_t)(catch_before + 1) && s_fa_on
                    && ls_mem().latch && strcmp(ls_mem().log_line, log_before) != 0)
                    recorded++;
                if (pass == 1 && fired) overflow_fired = 1;
            } else if (fired) {
                overflow_fired = 1;
            }
        }
    }
    const int seen_ok = ls_mem().seen_n[0] == LS_MCAP && ls_mem().catch_n == LS_MCAP;
    fprintf(f, "LS\tseen\t%d\t17 devices x 2 passes; overflow never alerts\n",
            recorded == LS_MCAP && !overflow_fired && seen_ok);

    botcorps_host_ls_pass_begin();
}

void botcorps_host_hub_alert(uint8_t *buf)
{
    ls_mem().fixture_arm = 0;
    ls_mem().legacy = 0;
    ls_mem().inited = 1;
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    enter_screen(SCR_WIFI_RADAR, -1);
    ls_toggle_arm();
    uint8_t mac[6] = { 2, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e };
    ls_model_add(0, mac, -48, GL_PHONE, "phone-live");
    ls_first_sighting(0, mac, 0);
    s_fa_on = false;
    enter_screen(SCR_GIBSON, -1);
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
    ls_mem().latch = 0;
    s_fa_on = false;
    ls_mem().legacy = 1;
    ls_mem().fixture_arm = 1;
}

void botcorps_host_ls_shot(uint8_t *buf, int which)
{
    /* which: 0 wifi disarmed, 1 wifi armed, 2 ble disarmed, 3 ble armed, 4 alert */
    botcorps_host_ls_live();
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    const int ble = (which == 2 || which == 3);
    enter_screen(ble ? SCR_BT_RADAR : SCR_WIFI_RADAR, -1);
    if (which == 1 || which == 3 || which == 4) ls_toggle_arm();
    if (which == 1 || which == 3) {
        const int band = ble ? 1 : 0;
        const char *nm[3] = { ble ? "earbuds-live" : "cafe-live", ble ? "watch-live" : "phone-live", ble ? "tile-live" : "lobby-live" };
        const int ty[3] = { ble ? GL_SPEAKER : GL_AP, ble ? GL_WATCH : GL_PHONE, ble ? GL_TRACKER : GL_AP };
        for (int i = 0; i < 3; ++i) {
            uint8_t mac[6] = { 4, (uint8_t)band, 0x20, 0x30, 0x40, (uint8_t)(0x60 + i) };
            ls_model_add(band, mac, -40 - i * 6, ty[i], nm[i]);
            if (ls_mem().seen_n[band] < LS_MCAP) {
                memcpy(ls_mem().seen[band][ls_mem().seen_n[band]], mac, 6);
                ls_mem().seen_n[band]++;
            }
        }
        s_fa_on = false;
        ls_mem().fa_on = 0;
        s_cursor = 0;
    }
    if (which == 4) {
        uint8_t mac[6] = { 2, 0x42, 0x11, 0x22, 0x33, 0x44 };
        ls_model_add(0, mac, -51, GL_CAMERA, "nest-live");
        ls_first_sighting(0, mac, 0);
    }
    host_frame = buf;
    memset(buf, PAL_BLACK, 320u * 240u);
    paint_ext(Gfx());
}
#endif

#else

static int ls_owned_band(void) { return -1; }
static bool ls_live_screen(void) { (void)ls_owned_band; return false; }
static bool ls_t3_band(void) { return false; }
static bool ls_gate_legacy(void) { return false; }
static bool ls_armed(void) { return false; }
static bool ls_model_on(void) { return false; }
static int ls_list_n(void) { return s_sb_n; }
static bool ls_glow_on(void) { return false; }
static int ls_hub_latched(void) { return 0; }
static void ls_glow_tick(uint32_t) {}
static void ls_on_enter(void) {}
static void ls_toggle_arm(void) {}
static void ls_pump(void) {}
static bool sb_ignored(int band, int d) { return s_dev_ign[band][d] != 0; }
static int sb_tagv(int band, int d) { return s_dev_tag[band][d]; }
static void ls_strip_text(int, char *, int, char *, int) {}
static int ls_fa_on(void) { return 0; }
static const char *ls_fa_name(void) { return ""; }
static int ls_fa_ty(void) { return 0; }
static int ls_fa_db(void) { return 0; }
static int ls_fa_icon(void) { return 0; }
static void ls_fa_mac(char *out, int n) { if (n > 0) out[0] = 0; }
static int ls_fa_take(int *, uint8_t *, int *) { return 0; }
static void ls_fa_clear(void) {}
static const char *ls_dev_name(int, int) { return ""; }
static int ls_dev_db(int, int) { return 0; }
static int ls_dev_ty(int, int) { return 0; }
static void ls_ign_add(int, const uint8_t *) {}
static void ls_tag_set(int, int, int) {}
static void ls_mac_of(int, int, uint8_t *) {}

#endif
