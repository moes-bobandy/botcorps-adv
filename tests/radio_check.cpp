#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
#define IRAM_ATTR
#define LV_MAXF 48
struct LvFind {char name[33];uint8_t mac[6];int8_t rssi;};
using TaskHandle_t=void*; using portMUX_TYPE=int;
#define portMUX_INITIALIZER_UNLOCKED 0
#define portENTER_CRITICAL(p) ((void)0)
#define portEXIT_CRITICAL(p) ((void)0)
#define pdTRUE 1
#define pdMS_TO_TICKS(x) (x)
static uint32_t clock_ms=0;static int core=0,wifi_entries=0,ble_entries=0,wifi_mode=0;
static bool ble_initialized=false;
uint32_t millis(){return clock_ms;}
static void on_ui();
void xTaskNotifyGive(TaskHandle_t) {assert(core==1);}
uint32_t ulTaskNotifyTake(int,uint32_t ms) {clock_ms+=ms;core=1;on_ui();core=0;return 0;}
#define WIFI_OFF 0
#define WIFI_STA 1
#define WIFI_SCAN_FAILED -2
#define WIFI_SCAN_RUNNING -1
#define ESP_OK 0
#define WIFI_SECOND_CHAN_NONE 0
#define WIFI_PROMIS_FILTER_MASK_MGMT 1
#define WIFI_PKT_MGMT 1
using esp_err_t=int; using wifi_promiscuous_pkt_type_t=int;
struct wifi_promiscuous_pkt_t {struct {int sig_len;}rx_ctrl;uint8_t payload[2];};
struct wifi_promiscuous_filter_t {int filter_mask;};
static void driver(){assert(core==0);}
int esp_wifi_set_promiscuous(bool){driver();return 0;}int esp_wifi_scan_stop(){driver();return 0;}
int esp_wifi_set_promiscuous_filter(wifi_promiscuous_filter_t*){driver();return 0;}
void esp_wifi_set_promiscuous_rx_cb(void(*)(void*,int)){driver();}
void esp_wifi_set_channel(uint8_t,int){driver();}
struct WiFiMock {
 uint32_t started=0;
 void persistent(bool){driver();}
 bool mode(int m){driver();if(m==WIFI_STA){assert(!ble_initialized);++wifi_entries;}wifi_mode=m;return true;}
 void disconnect(bool,bool){driver();}
 int scanNetworks(bool,bool){driver();assert(wifi_mode==WIFI_STA);started=millis();return WIFI_SCAN_RUNNING;}
 int scanComplete(){driver();return millis()-started>=40?1:WIFI_SCAN_RUNNING;}
 void scanDelete(){driver();}
} WiFi;
struct NimBLEScan {
 uint32_t end=0;bool active=false;
 void setActiveScan(bool){driver();}void setInterval(int){driver();}void setWindow(int){driver();}void setMaxResults(int){driver();}
 bool start(uint32_t ms,bool,bool){driver();assert(ble_initialized);assert(ms==100);end=millis()+ms;active=true;return true;}
 bool isScanning(){driver();return active && (int32_t)(millis()-end)<0;}
 void stop(){driver();active=false;}void clearResults(){driver();}
};static NimBLEScan scan;
struct NimBLEDevice {
 static bool isInitialized(){driver();return ble_initialized;}
 static bool init(const char*){driver();assert(wifi_mode==WIFI_OFF);ble_initialized=true;++ble_entries;return true;}
 static NimBLEScan *getScan(){driver();return &scan;}
 static void deinit(bool){driver();ble_initialized=false;}
};
struct SerialMock {template<class...T>void printf(const char*,T...){} }Serial;
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
static int radio_wifi_results(LvFind *out, int, int) { memset(out,0,sizeof(*out));out->rssi=-55;return 1; }
static const uint16_t BLE_SVC16[] = {0xFD5A, 0xFEED, 0xFFFA, 0xFD44, 0xFE2C, 0xFD6F, 0xFE9F, 0x180F, 0x1812};
static int radio_ble_results(LvFind *out, int, NimBLEScan *) { memset(out,0,sizeof(*out));out->rssi=-65;return 1; }
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


struct Done{};
static void on_ui(){
 static int step=0;++step;
 if(step==1)r_wifi_start();
 if(step==6){LvFind f;assert(r_wifi_poll(&f,1)==1);assert(f.rssi==-55);}
 if(step==8)r_wifi_start(); // a new epoch must not reinitialize WiFi
 if(step==15)r_ble_start(100);
 if(step==20)r_stop();
 if(step==23)r_deauth_start(1000);
 if(step==28)r_stop();
 if(step==31)throw Done{};
}
int main(){
 s_radio_task=(void*)1;
 try{lv_radio_task(nullptr);}catch(Done&){}
 assert(wifi_entries==2 && ble_entries==1);assert(!ble_initialized && wifi_mode==WIFI_OFF);
 core=1;r_wifi_start();const auto old=radio_request();r_ble_start(100);
 radio_publish(old,1);LvFind f;assert(r_ble_poll(&f,1)==-1);assert(!s_batch_ready);
 radio_publish(radio_request(),-2);assert(r_ble_poll(&f,1)==-2);
 puts("Radio worker PASS: Core 0 driver ownership, exclusive stacks, same-mode warm reuse, STOP teardown, stale epochs dropped, async failure handoff");
}
