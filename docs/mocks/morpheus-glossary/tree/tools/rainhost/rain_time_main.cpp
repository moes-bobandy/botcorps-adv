#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "ui/botcorps_pal.h"
#include "rain/botcorps_rain.h"
#include "display/botcorps_display.h"
extern uint8_t g_fb[320*240];
extern int g_clip_x,g_clip_y,g_clip_w,g_clip_h;
extern uint16_t botcorps_t0_lut565[256];
void host_set_ms(uint32_t);
extern "C" {}
#include "ui/botcorps_pal_tables.inc"
static uint16_t pack332b(uint8_t i){int r3=(i>>5)&7,g3=(i>>2)&7,b2=i&3;int r8=(((r3<<3)+r3)<<2)+(r3>>1),g8=(((g3<<3)+g3)<<2)+(g3>>1),b8=b2*0x55;return (uint16_t)(((r8>>3)<<11)|((g8>>2)<<5)|(b8>>3));}
static uint16_t word_of(uint8_t idx){ if(k_is_res[idx]) return k_exact_nat[idx]; return pack332b(idx);} 
static lgfx::v1::LGFX_Device dev;
int main(){
    for(int i=0;i<256;++i) botcorps_t0_lut565[i]=__builtin_bswap16(word_of((uint8_t)i));
    botcorps_rain_set_mode(4); botcorps_rain_set_mix(0); botcorps_rain_set_covers(0,0,false);
    uint32_t ms=0; for(int w=0;w<40;++w){ms+=90;host_set_ms(ms);botcorps_rain_step();}
    const int N=2000; struct timespec a,b;
    clock_gettime(CLOCK_MONOTONIC,&a);
    for(int i=0;i<N;++i){ ms+=90;host_set_ms(ms);botcorps_rain_step(); memset(g_fb,0,sizeof(g_fb)); g_clip_x=0;g_clip_y=0;g_clip_w=320;g_clip_h=240; botcorps_rain_draw(dev,0,240);}
    clock_gettime(CLOCK_MONOTONIC,&b);
    double us=((b.tv_sec-a.tv_sec)*1e6+(b.tv_nsec-a.tv_nsec)/1e3)/N;
    printf("host full-frame draw: %.1f us/frame (1200 cells)\n", us);
    // cells drawn per frame estimate
    return 0;
}
