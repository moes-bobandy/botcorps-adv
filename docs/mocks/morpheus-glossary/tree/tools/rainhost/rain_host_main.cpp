/* Host harness that runs the REAL src/rain/botcorps_rain.cpp against an 8-bit
 * palette framebuffer. Only the sprite/pixel/glyph layer is stubbed. */
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "ui/botcorps_pal.h"
#include "rain/botcorps_rain.h"
#include "ui/botcorps_band.h"
#include "ui/botcorps_sprite.h"
#include "display/botcorps_display.h"

#define PROGMEM
#include "lgfx/Fonts/glcdfont.h"

/* ---- framebuffer + clip ---- */
uint8_t g_fb[320*240];
int g_clip_x=0, g_clip_y=0, g_clip_w=320, g_clip_h=240;
static inline bool in_clip(int x,int y){ return x>=g_clip_x && x<g_clip_x+g_clip_w && y>=g_clip_y && y<g_clip_y+g_clip_h; }
static inline void px(int x,int y,uint8_t c){ if((unsigned)x<320&&(unsigned)y<240&&in_clip(x,y)) g_fb[y*320+x]=c; }

void lgfx::v1::LovyanGFX::fillRect(int x,int y,int w,int h,uint8_t c){
    for(int j=0;j<h;++j) for(int i=0;i<w;++i) px(x+i,y+j,c);
}
void lgfx::v1::LovyanGFX::print(const char* s){
    if(!s) return;
    for(int k=0; s[k]; ++k){
        unsigned u=(unsigned char)s[k];
        if(u<256){ const uint8_t* bits=font+u*5;
            for(int col=0;col<5;++col){ uint8_t line=bits[col];
                for(int row=0;row<8;++row) if((line>>row)&1) px(cx+col,cy+row,fg); } }
        cx+=6;
    }
}

/* ---- surfaces ---- */
static lgfx::v1::LGFX_Device s_dev;
lgfx::v1::LGFX_Device& botcorps_content(void){ return s_dev; }
lgfx::v1::LGFX_Device& botcorps_menu(void){ return s_dev; }
lgfx::v1::LGFX_Device& botcorps_disp(void){ return s_dev; }

/* ---- time / serial / ESP ---- */
static uint32_t s_ms=0;
uint32_t millis(void){ return s_ms; }
void host_set_ms(uint32_t t){ s_ms=t; }
HostPrintD Serial;
HostESPD ESP;
unsigned HostESPD::getFreeHeap(){ return 200000; }
unsigned HostESPD::getMaxAllocHeap(){ return 180000; }
bool botcorps_dual_ok(void){ return false; }
bool botcorps_band_direct(void){ return false; }

/* ---- fonts ---- */
namespace fonts { const Font Font0{0}; }

/* ---- Tier 0 buffers the rain owns ---- */
uint8_t botcorps_t0_rain_cell[3][1200];
uint8_t botcorps_t0_rain_col[56][8];
uint32_t botcorps_t0_row_hash[240];
uint16_t botcorps_t0_lut565[256];

/* ---- palette tables (REAL) ---- */
#include "ui/botcorps_pal_tables.inc"
#include "ui/botcorps_pal_fit.inc"
#include "ui/botcorps_pal_hub.inc"

uint16_t botcorps_pal_native(uint8_t idx){ return __builtin_bswap16(botcorps_t0_lut565[idx]); }

uint8_t botcorps_pal_ramp(int ink,int age){
    if(ink<0)ink=0; if(ink>=k_ramp_n)ink=k_ramp_n-1; if(age<0)age=0; if(age>13)age=13;
    const int step=age*(PAL_RAMP_N-1)/13; return k_ramp_idx[ink][step];
}
bool botcorps_pal_ramp_for(uint16_t native,int age,uint8_t* out){
    if(!out) return false;
    for(int i=0;i<k_ramp_n;++i){ if(k_ramp_src[i]!=native) continue; *out=botcorps_pal_ramp(i,age); return true; }
    return false;
}
uint8_t botcorps_pal_fit(uint16_t native){
    for(int n=0;n<k_own_n;++n){ const uint8_t i=k_own_idx[n]; if(k_exact_nat[i]==native) return i; }
    const unsigned r=(native>>11)&31,g=(native>>5)&63,b=native&31;
    const unsigned key=((r>>1)<<8)|((g>>2)<<4)|(b>>1); return k_fit4[key];
}

/* ---- hub fade + pal draw helpers ---- */
static bool s_hub=false;
void botcorps_pal_hub_set(bool on){ s_hub=on; }
static uint8_t hub_map(uint8_t idx){ return s_hub?k_hub_fade[idx]:idx; }
void botcorps_pal_rect(lgfx::v1::LovyanGFX& d,int x,int y,int w,int h,uint8_t idx){ d.fillRect(x,y,w,h,hub_map(idx)); }
void botcorps_pal_pixel(lgfx::v1::LovyanGFX& d,int x,int y,uint8_t idx){ d.fillRect(x,y,1,1,hub_map(idx)); }
void botcorps_pal_text(lgfx::v1::LovyanGFX& d,uint8_t idx){ d.setTextColor(hub_map(idx)); }

/* ---- sprite + shapes (same bitmaps as src/ui/botcorps_sprite.cpp) ---- */
static const int SPR_W[4]={7,9,7,5}; static const int SPR_H[4]={7,6,9,5};
static const char* const SPR_ROW[4][9]={
 {"...1...",".11111.","1111111","1221221","1221221","1111111",".11111."},
 {"....1....",".1111111.","112211221","112211221","111111111",".1111111."},
 {"...1...","1111111","1221221","1221221","1111111",".11111.",".11111.",".11.11.",".11.11."},
 {"11111","12121","11111","11111","11111"} };
int botcorps_shape_for(int index){ static const int S[12]={0,1,2,3,0,1,2,3,0,1,2,3}; return S[((index%12)+12)%12]; }
void botcorps_sprite_draw(lgfx::v1::LovyanGFX&,int x,int y,int shape,int scale,uint8_t color,bool blink){
    if(shape<0||shape>3)shape=0; int s=scale<1?1:scale; int w=SPR_W[shape],h=SPR_H[shape];
    int ox=x-(w*s)/2, oy=y-(h*s)/2;
    for(int row=0;row<h;++row){ const char* line=SPR_ROW[shape][row];
        for(int col=0;col<w;++col){ char v=line[col]; if(v!='1'&&!(v=='2'&&!blink))continue;
            uint8_t ink=(v=='2')?(uint8_t)36:color;
            for(int sy=0;sy<s;++sy)for(int sx=0;sx<s;++sx)px(ox+col*s+sx,oy+row*s+sy,ink); } }
}
void botcorps_sprite_draw_inks(lgfx::v1::LovyanGFX& d,int x,int y,int shape,int scale,uint8_t body,uint8_t,bool blink){ botcorps_sprite_draw(d,x,y,shape,scale,body,blink); }

/* ---- bot colours (REAL) ---- */
extern const uint16_t BOTCORPS_BOT_COLOR[12];
const uint16_t BOTCORPS_BOT_COLOR[12]={
 0x26E8,0x269C,0x331F,0xB21F,0xF586,0xFD20,0x07FF,0xF81F,0x07E0,0xFFE0,0xFC9F,0x9CFF };

/* ---- RGB888 out, same panel path as render_main ---- */
static uint16_t pack332(uint8_t i){
    int r3=(i>>5)&7,g3=(i>>2)&7,b2=i&3;
    int r8=(((r3<<3)+r3)<<2)+(r3>>1), g8=(((g3<<3)+g3)<<2)+(g3>>1), b8=b2*0x55;
    return (uint16_t)(((r8>>3)<<11)|((g8>>2)<<5)|(b8>>3));
}
static uint16_t word_of(uint8_t idx){ if(k_is_res[idx]) return k_exact_nat[idx]; return pack332(idx); }
static void rgb_of(uint8_t idx,uint8_t*r,uint8_t*g,uint8_t*b){
    uint16_t c=word_of(idx); int r5=(c>>11)&31,g6=(c>>5)&63,b5=c&31;
    *r=(uint8_t)((r5<<3)|(r5>>2)); *g=(uint8_t)((g6<<2)|(g6>>4)); *b=(uint8_t)((b5<<3)|(b5>>2));
}
static void save_rgb(const char* path){
    FILE* f=fopen(path,"wb"); if(!f)return;
    for(int i=0;i<320*240;++i){ uint8_t r,g,b; rgb_of(g_fb[i],&r,&g,&b); fputc(r,f);fputc(g,f);fputc(b,f); }
    fclose(f);
}

#ifndef NO_MAIN
int main(int argc,char**argv){
    int mode = argc>1?atoi(argv[1]):4;     /* 4=PURPLE 2=CYAN */
    const char* outdir = argc>2?argv[2]:"/tmp/rain";
    int frames = argc>3?atoi(argv[3]):8;
    for(int i=0;i<256;++i) botcorps_t0_lut565[i]=__builtin_bswap16(word_of((uint8_t)i));
    botcorps_rain_set_mode(mode);
    botcorps_rain_set_mix(BOTCORPS_RAIN_MIX_FIELD);
    botcorps_rain_set_covers(nullptr,0,false);
    /* warm up a few steps so drops populate */
    s_ms=0;
    for(int w=0;w<40;++w){ s_ms+=90; botcorps_rain_step(); }
    for(int fr=0; fr<frames; ++fr){
        s_ms+=90; botcorps_rain_step();
        memset(g_fb,0,sizeof(g_fb));
        g_clip_x=0;g_clip_y=0;g_clip_w=320;g_clip_h=240;
        botcorps_rain_draw(s_dev,0,240);
        char p[256]; snprintf(p,sizeof(p),"%s/f%02d.rgb",outdir,fr); save_rgb(p);
    }
    printf("rain harness: mode=%d frames=%d dir=%s\n",mode,frames,outdir);
    return 0;
}
#endif
