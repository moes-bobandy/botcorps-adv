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
static int s_log_stage;
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
    Serial.printf("[botcorps] heap %s free=%u largest=%u min=%u\n",
                  tag ? tag : "?",
                  (unsigned)h.free_b, (unsigned)h.max_b, (unsigned)h.min_b);
}

static int next_stage(void)
{
    if (s_log_stage < 255) ++s_log_stage;
    return s_log_stage;
}

static void take(int id, const char *tag)
{
    if (id < 0 || id >= BOTCORPS_ST_N) return;
    const int n = next_stage();
    s_probe.ord[id] = (uint8_t)n;
    botcorps_heap_sample(&s_probe.st[id]);
    Serial.printf("[botcorps] heap %02d %s free=%u largest=%u min=%u\n",
                  n, tag,
                  (unsigned)s_probe.st[id].free_b,
                  (unsigned)s_probe.st[id].max_b,
                  (unsigned)s_probe.st[id].min_b);
}

static void probe_reset(void)
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
    s_log_stage = 0;
}

void botcorps_probe_before_m5(void)
{
    probe_reset();
    take(BOTCORPS_ST_M0, "m5-before");
}

void botcorps_probe_after_m5(void)
{
    take(BOTCORPS_ST_M5, "m5-after");
}

void botcorps_probe_before_ext(void)
{
    take(BOTCORPS_ST_E0, "ext-before");
}

void botcorps_probe_after_ext(bool ext_ok)
{
    snprintf(s_probe.ext_st, sizeof(s_probe.ext_st), ext_ok ? "ok" : "fail");
    take(BOTCORPS_ST_EXT, "ext");
    Serial.printf("[botcorps] probe %02d EXT=%s\n", s_log_stage, s_probe.ext_st);
}

static int nim_gate(char *st, size_t n, int *rc)
{
    const uint32_t largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
    if (largest < NIM_GATE) {
        snprintf(st, n, "short");
        *rc = -1;
        Serial.printf("[botcorps] probe %02d NIM short largest=%u\n",
                      s_log_stage, (unsigned)largest);
        return 0;
    }
    const bool ok = NimBLEDevice::init("bc");
    *rc = ok ? 0 : 1;
    snprintf(st, n, ok ? "ok" : "fail");
    Serial.printf("[botcorps] probe %02d NIM %s rc=%d\n",
                  s_log_stage, st, *rc);
    NimBLEDevice::deinit(true);
    return ok ? 1 : 0;
}

void botcorps_probe_run(void)
{
    s_probe.tier0 = botcorps_tier0_bytes();
    take(BOTCORPS_ST_T0, "t0");
    Serial.printf("[botcorps] probe %02d tier0 bss=%u cell=%u col=%u term=%u bot=%u scan=%u\n",
                  s_log_stage,
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
    take(BOTCORPS_ST_SC0, "sprite-before");
    botcorps_rain_step();
    s_probe.sprite_held = botcorps_rain_sprite_held() ? 1 : 0;
    if (!s_probe.sprite_held) s_probe.sprite_bytes = 0;
    else if (strcmp(s_probe.ext_st, "ok") == 0) s_probe.sprite_bytes = 320u * 240u * 2u;
    else s_probe.sprite_bytes = 240u * 135u * 2u;
    take(BOTCORPS_ST_SC1, "sprite-after");
    Serial.printf("[botcorps] probe %02d sprite held=%d bytes=%u\n",
                  s_log_stage, s_probe.sprite_held, (unsigned)s_probe.sprite_bytes);
    take(BOTCORPS_ST_B0, "nimB-before");
    nim_gate(s_probe.nim_b, sizeof(s_probe.nim_b), &s_probe.nim_b_rc);
    take(BOTCORPS_ST_B1, "nimB-after");
    /* Later stages must not be measured with the 154 KB sprite held.
     * The rain screen calls ensure_sprite again after this probe. */
    take(BOTCORPS_ST_F0, "free-before");
    botcorps_rain_release_sprite();
    s_probe.sprite_freed = botcorps_rain_sprite_held() ? 0 : 1;
    take(BOTCORPS_ST_SF, "free-after");

    /* LS11: the boot probe does not call scanNetworks and does not delay-wait.
     * NIM_GATE (48 KB) stays in nim_gate(). A live scan starts only at arm. */
    s_probe.wifi_n = -1;
    WiFi.mode(WIFI_OFF);
    take(BOTCORPS_ST_W0, "wifi-skip");
    take(BOTCORPS_ST_W1, "wifi-skip");
    Serial.printf("[botcorps] probe %02d WIFI skipped gate=%u\n",
                  s_log_stage, (unsigned)NIM_GATE);

    take(BOTCORPS_ST_OV0, "ov-before");
    {
        lgfx::LGFX_Sprite spr;
        s_probe.ov_ok = spr.createSprite(128, 114) ? 1 : 0;
        Serial.printf("[botcorps] probe %02d OV alloc=%d bytes=%d\n",
                      s_log_stage, s_probe.ov_ok, OV_BYTES);
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
    const int n = next_stage();
    Serial.printf("[botcorps] heap %02d screen leave=%s enter=%s free=%u largest=%u min=%u\n",
                  n,
                  leave ? leave : "-",
                  enter ? enter : "-",
                  (unsigned)h.free_b, (unsigned)h.max_b, (unsigned)h.min_b);
}

const botcorps_probe_t *botcorps_probe_get(void)
{
    return &s_probe;
}

static void row(lgfx::LovyanGFX &g, int y, int ord, const char *name,
                const botcorps_heap_t *h, const char *tail)
{
    char line[56];
    snprintf(line, sizeof(line), "%02d %s %lu L%lu %lu %s",
             ord,
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
    struct spec {
        int id;
        const char *name;
        int kind;
    };
    static const spec rows[] = {
        { BOTCORPS_ST_M0, "M0", 0 },
        { BOTCORPS_ST_M5, "M5", 0 },
        { BOTCORPS_ST_E0, "E0", 0 },
        { BOTCORPS_ST_EXT, "EXT", 1 },
        { BOTCORPS_ST_T0, "T0", 2 },
        { BOTCORPS_ST_A0, "A0", 0 },
        { BOTCORPS_ST_A1, "A1", 3 },
        { BOTCORPS_ST_SC0, "S0", 0 },
        { BOTCORPS_ST_SC1, "S1", 4 },
        { BOTCORPS_ST_B0, "B0", 0 },
        { BOTCORPS_ST_B1, "B1", 5 },
        { BOTCORPS_ST_F0, "F0", 0 },
        { BOTCORPS_ST_SF, "SF", 6 },
        { BOTCORPS_ST_W0, "W0", 0 },
        { BOTCORPS_ST_W1, "W1", 7 },
        { BOTCORPS_ST_OV0, "O0", 0 },
        { BOTCORPS_ST_OV1, "O1", 8 },
        { BOTCORPS_ST_N2, "N2", 9 },
    };
    g.fillScreen(0x0000);
    g.setTextWrap(false, false);
    for (int i = 0; i < (int)(sizeof(rows) / sizeof(rows[0])); ++i) {
        char tail[16];
        tail[0] = 0;
        switch (rows[i].kind) {
        case 1: snprintf(tail, sizeof(tail), "%s", p->ext_st); break;
        case 2: snprintf(tail, sizeof(tail), "%lu", (unsigned long)p->tier0); break;
        case 3: snprintf(tail, sizeof(tail), "%s %d", p->nim_a, p->nim_a_rc); break;
        case 4: snprintf(tail, sizeof(tail), "spr%d", p->sprite_held); break;
        case 5: snprintf(tail, sizeof(tail), "%s %d", p->nim_b, p->nim_b_rc); break;
        case 6: snprintf(tail, sizeof(tail), "free%d", p->sprite_freed); break;
        case 7: snprintf(tail, sizeof(tail), "n%d", p->wifi_n); break;
        case 8: snprintf(tail, sizeof(tail), "%d", p->ov_ok); break;
        case 9: snprintf(tail, sizeof(tail), "%s %d", p->nim_2, p->nim_2_rc); break;
        default: break;
        }
        row(g, i * 8, p->ord[rows[i].id], rows[i].name, &p->st[rows[i].id], tail);
    }

    const uint16_t c1 = 0x4A69;
    const uint16_t c2 = 0x8CD4;
    const uint16_t c3 = 0xFFFF;
    for (int i = 0; i < BOTCORPS_ICON_N; ++i) {
        botcorps_icon_blit(g, 4 + i * 16, 150, i, c1, c2, c3);
    }
}
