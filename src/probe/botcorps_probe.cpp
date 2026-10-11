#include "probe/botcorps_probe.h"
#include "probe/botcorps_icons.h"
#include "rain/botcorps_rain.h"

#include <NimBLEDevice.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <stdio.h>
#include <string.h>
#include <lgfx/v1/LGFX_Sprite.hpp>

static botcorps_probe_t s_probe;
static const uint32_t NIM_GATE = 48u * 1024u;
static const int OV_BYTES = 29184; /* 128 * 114 * 2 */

void botcorps_heap_sample(botcorps_heap_t *out)
{
    if (!out) return;
    out->free_b = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
    out->max_b = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    out->min_b = heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL);
}

void botcorps_heap_log(const char *tag)
{
    botcorps_heap_t h;
    botcorps_heap_sample(&h);
    Serial.printf("[botcorps] heap %s free=%u max=%u min=%u\n",
                  tag ? tag : "?",
                  (unsigned)h.free_b, (unsigned)h.max_b, (unsigned)h.min_b);
}

static void take(int id, const char *tag)
{
    if (id < 0 || id >= BOTCORPS_ST_N) return;
    botcorps_heap_sample(&s_probe.st[id]);
    Serial.printf("[botcorps] heap %s free=%u max=%u min=%u\n",
                  tag,
                  (unsigned)s_probe.st[id].free_b,
                  (unsigned)s_probe.st[id].max_b,
                  (unsigned)s_probe.st[id].min_b);
}

void botcorps_probe_after_m5(void)
{
    memset(&s_probe, 0, sizeof(s_probe));
    s_probe.wifi_n = -1;
    s_probe.nim_a_rc = -1;
    s_probe.nim_b_rc = -1;
    s_probe.nim_2_rc = -1;
    snprintf(s_probe.nim_a, sizeof(s_probe.nim_a), "skip");
    snprintf(s_probe.nim_b, sizeof(s_probe.nim_b), "skip");
    snprintf(s_probe.nim_2, sizeof(s_probe.nim_2), "skip");
    snprintf(s_probe.ext_st, sizeof(s_probe.ext_st), "fail");
    take(BOTCORPS_ST_M5, "m5");
}

void botcorps_probe_after_ext(bool ext_ok)
{
    snprintf(s_probe.ext_st, sizeof(s_probe.ext_st), ext_ok ? "ok" : "fail");
    take(BOTCORPS_ST_EXT, "ext");
    Serial.printf("[botcorps] probe EXT=%s\n", s_probe.ext_st);
}

static int nim_gate(char *st, size_t n, int *rc)
{
    const uint32_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    if (largest < NIM_GATE) {
        snprintf(st, n, "short");
        *rc = -1;
        Serial.printf("[botcorps] probe NIM short max=%u\n", (unsigned)largest);
        return 0;
    }
    const bool ok = NimBLEDevice::init("bc");
    *rc = ok ? 0 : 1;
    snprintf(st, n, ok ? "ok" : "fail");
    Serial.printf("[botcorps] probe NIM %s rc=%d\n", st, *rc);
    NimBLEDevice::deinit(true);
    return ok ? 1 : 0;
}

void botcorps_probe_run(void)
{
    s_probe.tier0 = botcorps_tier0_bytes();
    take(BOTCORPS_ST_T0, "t0");
    Serial.printf("[botcorps] tier0 bss=%u cell=%u col=%u term=%u bot=%u scan=%u\n",
                  (unsigned)s_probe.tier0,
                  (unsigned)botcorps_tier0_part(0),
                  (unsigned)botcorps_tier0_part(1),
                  (unsigned)botcorps_tier0_part(2),
                  (unsigned)botcorps_tier0_part(3),
                  (unsigned)botcorps_tier0_part(4));

    /* A: rain sprite not allocated. */
    take(BOTCORPS_ST_A0, "nimA-before");
    nim_gate(s_probe.nim_a, sizeof(s_probe.nim_a), &s_probe.nim_a_rc);
    take(BOTCORPS_ST_A1, "nimA-after");

    /* B: 154 KB sprite first, same path the UI uses. */
    botcorps_rain_step();
    s_probe.sprite_held = botcorps_rain_sprite_held() ? 1 : 0;
    if (!s_probe.sprite_held) s_probe.sprite_bytes = 0;
    else if (strcmp(s_probe.ext_st, "ok") == 0) s_probe.sprite_bytes = 320u * 240u * 2u;
    else s_probe.sprite_bytes = 240u * 135u * 2u;
    take(BOTCORPS_ST_B0, "nimB-before");
    nim_gate(s_probe.nim_b, sizeof(s_probe.nim_b), &s_probe.nim_b_rc);
    take(BOTCORPS_ST_B1, "nimB-after");
    Serial.printf("[botcorps] probe sprite held=%d bytes=%u\n",
                  s_probe.sprite_held, (unsigned)s_probe.sprite_bytes);

    WiFi.persistent(false);
    WiFi.mode(WIFI_STA);
    WiFi.disconnect(true, false);
    WiFi.scanNetworks(true);
    take(BOTCORPS_ST_W0, "wifi-start");
    int n = WIFI_SCAN_RUNNING;
    const uint32_t t0 = millis();
    while ((n = WiFi.scanComplete()) == WIFI_SCAN_RUNNING &&
           (uint32_t)(millis() - t0) < 8000u) {
        delay(50);
    }
    s_probe.wifi_n = n;
    WiFi.scanDelete();
    WiFi.mode(WIFI_OFF);
    take(BOTCORPS_ST_W1, "wifi-stop");
    Serial.printf("[botcorps] probe WIFI n=%d\n", n);

    take(BOTCORPS_ST_OV0, "ov-before");
    {
        lgfx::LGFX_Sprite spr;
        s_probe.ov_ok = spr.createSprite(128, 114) ? 1 : 0;
        Serial.printf("[botcorps] probe OV alloc=%d bytes=%d\n",
                      s_probe.ov_ok, OV_BYTES);
        spr.deleteSprite();
    }
    take(BOTCORPS_ST_OV1, "ov-after");
    nim_gate(s_probe.nim_2, sizeof(s_probe.nim_2), &s_probe.nim_2_rc);
    take(BOTCORPS_ST_N2, "nim2-after");
}

void botcorps_probe_screen(const char *leave, const char *enter)
{
    botcorps_heap_t h;
    botcorps_heap_sample(&h);
    Serial.printf("[botcorps] heap screen leave=%s enter=%s free=%u max=%u min=%u\n",
                  leave ? leave : "-",
                  enter ? enter : "-",
                  (unsigned)h.free_b, (unsigned)h.max_b, (unsigned)h.min_b);
}

const botcorps_probe_t *botcorps_probe_get(void)
{
    return &s_probe;
}

static void row(lgfx::LovyanGFX &g, int y, const char *name, const botcorps_heap_t *h,
                const char *tail)
{
    char line[48];
    snprintf(line, sizeof(line), "%s %lu %lu %lu %s",
             name,
             (unsigned long)h->free_b,
             (unsigned long)h->max_b,
             (unsigned long)h->min_b,
             tail ? tail : "");
    g.setFont(&fonts::Font0);
    g.setTextSize(1, 1);
    g.setTextColor(0xFFFF);
    g.setCursor(2, y);
    g.print(line);
}

void botcorps_probe_paint(lgfx::LovyanGFX &g)
{
    const botcorps_probe_t *p = &s_probe;
    char tail[24];
    g.fillScreen(0x0000);
    g.setTextWrap(false, false);
    row(g, 2, "M5", &p->st[BOTCORPS_ST_M5], "");
    snprintf(tail, sizeof(tail), "%s", p->ext_st);
    row(g, 12, "EXT", &p->st[BOTCORPS_ST_EXT], tail);
    snprintf(tail, sizeof(tail), "%lu", (unsigned long)p->tier0);
    row(g, 22, "T0", &p->st[BOTCORPS_ST_T0], tail);
    row(g, 34, "A0", &p->st[BOTCORPS_ST_A0], "");
    snprintf(tail, sizeof(tail), "%s %d", p->nim_a, p->nim_a_rc);
    row(g, 44, "A1", &p->st[BOTCORPS_ST_A1], tail);
    snprintf(tail, sizeof(tail), "%d", p->sprite_held);
    row(g, 56, "B0", &p->st[BOTCORPS_ST_B0], tail);
    snprintf(tail, sizeof(tail), "%s %d", p->nim_b, p->nim_b_rc);
    row(g, 66, "B1", &p->st[BOTCORPS_ST_B1], tail);
    row(g, 78, "W0", &p->st[BOTCORPS_ST_W0], "");
    snprintf(tail, sizeof(tail), "n%d", p->wifi_n);
    row(g, 88, "W1", &p->st[BOTCORPS_ST_W1], tail);
    row(g, 100, "OV0", &p->st[BOTCORPS_ST_OV0], "");
    snprintf(tail, sizeof(tail), "%d", p->ov_ok);
    row(g, 110, "OV1", &p->st[BOTCORPS_ST_OV1], tail);
    snprintf(tail, sizeof(tail), "%s %d", p->nim_2, p->nim_2_rc);
    row(g, 120, "N2", &p->st[BOTCORPS_ST_N2], tail);
    char spr[32];
    snprintf(spr, sizeof(spr), "SP %lu %d",
             (unsigned long)p->sprite_bytes, p->sprite_held);
    g.setCursor(2, 132);
    g.print(spr);

    const uint16_t c1 = 0x4A69;
    const uint16_t c2 = 0x8CD4;
    const uint16_t c3 = 0xFFFF;
    for (int i = 0; i < BOTCORPS_ICON_N; ++i) {
        botcorps_icon_blit(g, 4 + i * 16, 148, i, c1, c2, c3);
    }
}
