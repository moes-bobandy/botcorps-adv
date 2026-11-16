/* MOCK ONLY. v3 of the big-bot Color UI screens.
 * Face slot LOCKED: THEME_FACE inclusive (65,88)-(174,197), 110x110.
 * Bottom panel (45,220, 152x71) is a smooth AA instrument per bot role.
 * TERMINALbot silver face: two eyes, no >_ prompt.
 * Unchanged pixels are copied byte-for-byte from the locked still.
 */
#define LV_HOST 1
#define LV_HOST_BAND_ROWS 320
#include "lv_draw.h"
#include "cyd_themes_lv_core_copy.h"
#include "themes_spec_v2.h"
#include "new_themes_v3.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cmath>
#include <algorithm>

static const int W = 240, H = 320;
static const int FACE_X = 65, FACE_Y = 88, FACE_S = 110; /* LOCKED v6 THEME_FACE */
static const int BOT_X0 = 54, BOT_Y0 = 79, BOT_X1 = 185, BOT_Y1 = 210;
static const int NAME_Y = 200;
static const int PX0 = 45, PY0 = 220, PW = 152, PH = 71; /* LV_CB_P* */
static void rgb565_to_888(uint16_t v, uint8_t *q)  /* same expand as the locked v6 stills: <<3 / <<2 */
{
    q[0]=(uint8_t)(((v>>11)&31)<<3);
    q[1]=(uint8_t)(((v>>5)&63)<<2);
    q[2]=(uint8_t)((v&31)<<3);
}

/* ---------- gold / silver faces (verbatim e_* from botcorps_cyd_edge.inc) ---------- */
static uint8_t PAL[256][3];
static void pal_init(void)
{
    memset(PAL, 0, sizeof PAL);
    auto set = [](int i, int r, int g, int b) { PAL[i][0]=(uint8_t)r; PAL[i][1]=(uint8_t)g; PAL[i][2]=(uint8_t)b; };
    set(108,82,69,24); set(104,99,73,16); set(140,148,109,0); set(177,181,146,82);
    set(213,247,182,74); set(249,247,211,74); set(250,255,219,173);
    set(41,41,56,66); set(110,107,121,132); set(146,140,154,165);
    set(186,198,207,214); set(223,231,239,247); set(255,255,255,255);
}
struct FaceBuf { uint8_t *px; int s; };
static FaceBuf *g_fb = nullptr;
struct Gfx {};
static void cbf(Gfx, int x, int y, int w, int h, uint8_t c)
{
    const int s = g_fb->s; uint8_t *p = g_fb->px;
    const uint8_t r = PAL[c][0], g = PAL[c][1], b = PAL[c][2];
    int x0 = x<0?0:x, y0 = y<0?0:y, x1 = x+w>s?s:x+w, y1 = y+h>s?s:y+h;
    for (int yy=y0; yy<y1; ++yy) for (int xx=x0; xx<x1; ++xx) {
        uint8_t *q = p + (yy*s+xx)*3; q[0]=r; q[1]=g; q[2]=b;
    }
}
static void e_gold_face(Gfx g, int x, int y, int s)
{
    static const uint8_t RIM=108, BRZ=104, LO=140, BASE=177, BRT=213, SHN=249, PALE=250;
    const int bw = s>=12?2:1, sh0=(s*70)/100, shw=s/6>2?s/6:2;
    for (int r=0;r<s;++r) {
        int i=0;
        while (i<s) {
            const int d=i+r; uint8_t c; int run=1;
            if (d<(s*45)/100) c=BRT;
            else if (d>=sh0 && d<sh0+shw) c=(d==sh0+shw/2)?PALE:SHN;
            else if (d>=(s*165)/100) c=BRZ;
            else if (d>=(s*135)/100) c=LO;
            else c=BASE;
            while (i+run<s) {
                const int d2=i+run+r; uint8_t c2;
                if (d2<(s*45)/100) c2=BRT;
                else if (d2>=sh0 && d2<sh0+shw) c2=(d2==sh0+shw/2)?PALE:SHN;
                else if (d2>=(s*165)/100) c2=BRZ;
                else if (d2>=(s*135)/100) c2=LO;
                else c2=BASE;
                if (c2!=c) break; ++run;
            }
            cbf(g, x+i, y+r, run, 1, c); i+=run;
        }
    }
    for (int k=0;k<bw;++k) {
        const uint8_t lit=k==0?PALE:BRT, dk=k==0?RIM:BRZ;
        cbf(g,x+k,y+k,s-2*k,1,lit); cbf(g,x+k,y+k,1,s-2*k,lit);
        cbf(g,x+k,y+s-1-k,s-2*k,1,dk); cbf(g,x+s-1-k,y+k,1,s-2*k,dk);
    }
    const int gx=x+s/5, gy=y+s/5, gw=s/4>2?s/4:2;
    cbf(g,gx,gy,gw,1,PALE); cbf(g,gx,gy,1,gw/2>1?gw/2:1,PALE);
}
static void e_silver_shell(Gfx g, int x, int y, int s) /* fill+bevel+glint, no >_ marks */
{
    static const uint8_t RIM=41, LO=110, MID=146, BASE=186, HI=223, GL=255;
    const int bw=s>=12?2:1, sh0=(s*70)/100, shw=s/6>2?s/6:2;
    for (int r=0;r<s;++r) {
        int i=0;
        while (i<s) {
            auto col=[&](int d)->uint8_t{
                if (d<(s*45)/100) return HI;
                if (d>=sh0 && d<sh0+shw) return HI;
                if (d>=(s*165)/100) return LO;
                if (d>=(s*135)/100) return MID;
                return BASE;
            };
            const uint8_t c=col(i+r); int run=1;
            while (i+run<s && col(i+run+r)==c) ++run;
            cbf(g,x+i,y+r,run,1,c); i+=run;
        }
    }
    for (int k=0;k<bw;++k) {
        const uint8_t lit=k==0?GL:HI, dk=k==0?RIM:LO;
        cbf(g,x+k,y+k,s-2*k,1,lit); cbf(g,x+k,y+k,1,s-2*k,lit);
        cbf(g,x+k,y+s-1-k,s-2*k,1,dk); cbf(g,x+s-1-k,y+k,1,s-2*k,dk);
    }
    if (s>=12) cbf(g,x+bw+1,y+bw,s/4,1,GL);
}
static void e_eyes(Gfx g, int x, int y, int s) /* same eye law as e_term_marks / e_metal_face, blink off */
{
    const int c=s/5>2?s/5:2, m=(s-3*c)/2;
    cbf(g,x+m,y+c,c,c,0); cbf(g,x+m+2*c,y+c,c,c,0);
}
static void paint_gold(uint8_t *px, int s){ FaceBuf fb{px,s}; g_fb=&fb; Gfx g{}; e_gold_face(g,0,0,s); e_eyes(g,0,0,s); }
static void paint_silver_eyes(uint8_t *px, int s){ FaceBuf fb{px,s}; g_fb=&fb; Gfx g{}; e_silver_shell(g,0,0,s); e_eyes(g,0,0,s); }

static std::vector<uint8_t> load_rgb(const char *path, size_t expect)
{
    FILE *f=fopen(path,"rb"); if(!f){ fprintf(stderr,"open %s\n",path); exit(1);} 
    std::vector<uint8_t> b(expect);
    if(fread(b.data(),1,expect,f)!=expect){ fprintf(stderr,"short %s\n",path); exit(1);} fclose(f); return b;
}
static void nn_blit_fit(uint8_t *dst, int ds, const uint8_t *src, int sw, int sh)
{
    memset(dst,0,ds*ds*3);
    const int sc=ds*256/(sw>sh?sw:sh);
    const int dw=(sw*sc)/256, dh=(sh*sc)/256, ox=(ds-dw)/2, oy=(ds-dh)/2;
    for(int y=0;y<dh;++y){ const int sy=(int)((int64_t)y*sh/dh);
        for(int x=0;x<dw;++x){ const int sx=(int)((int64_t)x*sw/dw);
            memcpy(dst+((oy+y)*ds+(ox+x))*3, src+(sy*sw+sx)*3, 3);
        }
    }
}
static void blit_face(uint8_t *frame, const uint8_t *face, int s)
{
    for(int y=0;y<s;++y) memcpy(frame+((FACE_Y+y)*W+FACE_X)*3, face+y*s*3, s*3);
}
static void draw_name(uint8_t *frame, const char *name, uint8_t r, uint8_t g, uint8_t b)
{
    if(!name||!name[0]) return;
    const int n=(int)strlen(name), tw=n*6-1, x=(BOT_X0+BOT_X1+1-tw)/2;
    const uint16_t c565=lv_rgb(r,g,b);
    std::vector<uint16_t> band((size_t)W*16,0);
    LvBand B{band.data(),0,NAME_Y,W,16};
    for(int yy=198; yy<=BOT_Y1; ++yy) for(int xx=BOT_X0; xx<=BOT_X1; ++xx){
        uint8_t *q=frame+(yy*W+xx)*3; q[0]=q[1]=q[2]=0;
    }
    b_text(B,x,NAME_Y,name,c565,1);
    for(int yy=0;yy<16;++yy) for(int xx=0;xx<W;++xx){
        uint16_t v=band[yy*W+xx]; if(!v) continue;
        rgb565_to_888(v, frame+((NAME_Y+yy)*W+xx)*3);
    }
}

/* ---------- v6 panel reink (verbatim roles from lv_controlbot_panel.h) ---------- */
static int lv_cb_g_t;
static uint16_t lv_cb_pan_buf[152*71];
static uint16_t *lv_cb_pan = lv_cb_pan_buf;
enum { CB_PLATE=0x080808, CB_KEY=0x9449ad, CB_VALUE=0xde92ff, CB_SEGOFF=0x210c31, CB_CAP=0x8c9aa5, CB_WHITE=0xffffff };
struct LvCbRect { int x0,y0,x1,y1; };
static LvCbRect lv_cb_r7[16]; static int lv_cb_n7=0;
static const uint8_t LV_CB_REINK_T[24]={23,1,2,3,4,5,6,7,8,9,10,11,25,13,14,24,16,17,18,19,20,21,22,26};
static uint32_t lv_cb_th_panel_at(int x,int y){
    return lv_cb_g_t<SEC_NTHEME ? sec_panel_at(lv_cb_g_t,x,y) : ext_theme_at(lv_cb_g_t-SEC_NTHEME,x,y);
}
static uint16_t lv_cb_cyd_sentinel(int role,int x,int y)
{
    const int c=lv_cb_g_t-SEC_NTHEME; const bool multi=ext_multi(c); const uint32_t T=lv_cb_th_panel_at(x,y);
    if(role==3) return lv_hex(sec_legible(multi?sec_mix3(T,0xffffff,2,5):EXT_HDR[c]));
    if(role==5) return lv_hex(multi?sec_mix3(T,0x000000,59,100):EXT_ACCD[c]);
    return lv_hex(sec_legible(T));
}
static uint16_t lv_cb_reink(int role,uint16_t c,int x,int y,int sx,int sy)
{
    const int t=lv_cb_g_t; const bool sec=t<SEC_NTHEME;
    switch(role){
    case 3: return sec?sec_sentinel(t,lv_hex(SEC_HDR[t]),x,y):lv_cb_cyd_sentinel(3,x,y);
    case 4: return sec?sec_sentinel(t,lv_hex(SEC_ACC[t]),x,y):lv_cb_cyd_sentinel(4,x,y);
    case 5: return sec?sec_sentinel(t,lv_hex(SEC_ACCD[t]),x,y):lv_cb_cyd_sentinel(5,x,y);
    case 6: return lv_hex(0x080808);
    case 7: return sec?sec_sentinel(t,lv_hex(SEC_ACC[t]),sx,sy):lv_cb_cyd_sentinel(4,sx,sy);
    default: return c;
    }
}

/* ---- 565 control-panel path: selftest only ---- */
static void lv_cb_put(int x,int y,uint32_t rgb,bool txt)
{
    if(x<PX0||x>=PX0+PW||y<PY0||y>=PY0+PH) return;
    const uint16_t c=lv_hex(rgb); int role=0, sx=x, sy=y;
    if(txt && rgb==CB_WHITE) role=6;
    else if(rgb==CB_KEY) role=3; else if(rgb==CB_VALUE) role=4; else if(rgb==CB_SEGOFF) role=5;
    if(role==4) for(int k=0;k<lv_cb_n7;++k){ const LvCbRect &r=lv_cb_r7[k];
        if(x>=r.x0&&x<r.x1&&y>=r.y0&&y<r.y1){ role=7; sx=(r.x0+r.x1)/2; sy=(r.y0+r.y1)/2; break; } }
    lv_cb_pan[(y-PY0)*PW+(x-PX0)] = lv_cb_reink(role,c,x,y,sx,sy);
}
static void lv_cb_fill(int x,int y,int w,int h,uint32_t c){ for(int j=y;j<y+h;++j) for(int i=x;i<x+w;++i) lv_cb_put(i,j,c,false); }
static void lv_cb_frame(int x,int y,int w,int h,uint32_t c){ lv_cb_fill(x,y,w,1,c); lv_cb_fill(x,y+h-1,w,1,c); lv_cb_fill(x,y,1,h,c); lv_cb_fill(x+w-1,y,1,h,c); }
static int lv_cb_tw(const char *s){ const int n=(int)strlen(s); return n?6*n-1:0; }
static void lv_cb_text(int x,int y,const char *s,uint32_t c)
{
    for(int i=0;s[i];++i){ const int g=s[i]<32||s[i]>126?0:s[i]-32;
        for(int cx=0;cx<5;++cx){ const uint8_t bits=LV_GLCD[g*5+cx];
            for(int cy=0;cy<7;++cy) if(bits>>cy&1) lv_cb_put(x+6*i+cx,y+cy,c,true); } }
}
static void lv_cb_meter(int x,int y,int w,int h,int lit)
{
    lv_cb_fill(x,y,w,h,0x000000); lv_cb_fill(x,y,2,h,CB_CAP); lv_cb_fill(x+w-2,y,2,h,CB_CAP);
    const int ix=x+3, iw=w-6, sw=(iw-7)/8>2?(iw-7)/8:2;
    for(int i=0;i<8;++i) lv_cb_fill(ix+i*(sw+1),y,sw,h, i<lit?CB_VALUE:CB_SEGOFF);
}
static void lv_cb_bar(int x,int y,const char *label,const char *value,int lit,bool box)
{
    const int w=68,h=22;
    lv_cb_fill(x,y,w,h,CB_PLATE); lv_cb_frame(x,y,w,h,CB_KEY);
    lv_cb_text(x+3,y+3,label,CB_KEY);
    const int vw=lv_cb_tw(value), vx=x+w-3-vw, ty=y+3;
    if(box){ lv_cb_r7[lv_cb_n7++]={vx-1,ty-1,vx+vw+1,ty+8}; lv_cb_fill(vx-1,ty-1,vw+2,9,CB_VALUE); lv_cb_text(vx,ty,value,CB_WHITE); }
    else lv_cb_text(vx,ty,value,CB_VALUE);
    lv_cb_meter(x+3,y+12,w-6,5,lit);
}
static void lv_cb_btn(int x,int y,const char *label,const char *value,bool on)
{
    const int w=39,h=34;
    lv_cb_r7[lv_cb_n7++]={x+3,y+12,x+w-3,y+h-3};
    lv_cb_fill(x,y,w,h,CB_PLATE); lv_cb_frame(x,y,w,h,CB_KEY);
    lv_cb_text(x+(w-lv_cb_tw(label))/2,y+3,label,CB_KEY);
    const int by=y+12,bh=h-15,bx=x+3,bw=w-6;
    lv_cb_fill(bx,by,bw,bh,on?0xde92ff:0x202020); lv_cb_frame(bx,by,bw,bh,on?0xffffff:0x606060);
    lv_cb_text(bx+(bw-lv_cb_tw(value))/2, by+(bh-7)/2, value, on?0xffffff:0xb0b0b0);
}
static int lv_cb_lit(int pct){ const int n=(8*pct+50)/100; return n<1?1:n; }
static void panel_control_default(int slot)
{
    lv_cb_g_t=LV_CB_REINK_T[slot]; lv_cb_n7=0;
    memset(lv_cb_pan_buf,0,sizeof lv_cb_pan_buf);
    lv_cb_fill(PX0,PY0,PW,PH,CB_PLATE); lv_cb_frame(PX0,PY0,PW,PH,CB_KEY);
    lv_cb_bar(46,221,"SCAN","B.RAIN",1,true);
    lv_cb_bar(46,244,"VOL","40%",lv_cb_lit(40),false);
    lv_cb_bar(46,267,"BRIGHT","80%",lv_cb_lit(80),false);
    lv_cb_btn(116,221,"CHIME","ON",true);
    lv_cb_btn(157,221,"TIME","RTC",true);
    lv_cb_btn(116,256,"BAR","HAZRD",true);
    lv_cb_btn(157,256,"TONE","BEEP",true);
}

/* ---- 888 AA instrument layer (2x supersample, then crisp GLCD labels) ---- */
static const int SS = 3;
static const int SSW = PW*SS, SSH = PH*SS;
static float ssbuf[SSW*SSH*3];
static uint8_t pan888[PH*PW*3];
static uint8_t TK[3], TV[3], TD[3], TP[3], TW[3], TC[3];

static inline float clampf(float v,float a,float b){ return v<a?a:(v>b?b:v); }
static inline float wrap2pi(float a){ while(a<0) a+=2.f*(float)M_PI; while(a>=2.f*(float)M_PI) a-=2.f*(float)M_PI; return a; }
static inline float deg(float d){ return d*(float)M_PI/180.f; }

static void theme_pick(int slot)
{
    lv_cb_g_t = LV_CB_REINK_T[slot];
    const int mx=PX0+PW/2, my=PY0+PH/2;
    rgb565_to_888(lv_cb_reink(3,lv_hex(CB_KEY),mx,my,mx,my), TK);
    rgb565_to_888(lv_cb_reink(4,lv_hex(CB_VALUE),mx,my,mx,my), TV);
    rgb565_to_888(lv_cb_reink(5,lv_hex(CB_SEGOFF),mx,my,mx,my), TD);
    TP[0]=8; TP[1]=8; TP[2]=8;
    TW[0]=248; TW[1]=252; TW[2]=248;
    TC[0]=136; TC[1]=152; TC[2]=160;
}

static void ss_clear()
{
    for(int i=0,n=SSW*SSH*3;i<n;i+=3){ ssbuf[i]=TP[0]; ssbuf[i+1]=TP[1]; ssbuf[i+2]=TP[2]; }
}
static inline void ss_over(int x,int y,float r,float g,float b,float a)
{
    if(a<=0.f||x<0||y<0||x>=SSW||y>=SSH) return;
    if(a>1.f) a=1.f;
    float *q=ssbuf+(y*SSW+x)*3;
    q[0]=r*a+q[0]*(1.f-a); q[1]=g*a+q[1]*(1.f-a); q[2]=b*a+q[2]*(1.f-a);
}
static inline void ss_add(int x,int y,float r,float g,float b,float a)
{
    if(a<=0.f||x<0||y<0||x>=SSW||y>=SSH) return;
    float *q=ssbuf+(y*SSW+x)*3;
    q[0]=clampf(q[0]+r*a,0.f,255.f); q[1]=clampf(q[1]+g*a,0.f,255.f); q[2]=clampf(q[2]+b*a,0.f,255.f);
}
static void downsample_panel()
{
    const float n=1.f/(SS*SS);
    for(int y=0;y<PH;++y) for(int x=0;x<PW;++x){
        float r=0,g=0,b=0;
        for(int j=0;j<SS;++j) for(int i=0;i<SS;++i){
            const float *q=ssbuf+((y*SS+j)*SSW+(x*SS+i))*3;
            r+=q[0]; g+=q[1]; b+=q[2];
        }
        uint8_t *o=pan888+(y*PW+x)*3;
        o[0]=(uint8_t)(r*n+0.5f); o[1]=(uint8_t)(g*n+0.5f); o[2]=(uint8_t)(b*n+0.5f);
    }
}
static void blit_panel888(uint8_t *frame)
{
    for(int y=0;y<PH;++y)
        memcpy(frame+((PY0+y)*W+PX0)*3, pan888+y*PW*3, PW*3);
}

/* coverage helpers in 1x panel space; loops write SS pixels */
static void fill_disc(float cx,float cy,float rad,const uint8_t *c,float a)
{
    const float cx2=cx*SS, cy2=cy*SS, r2=rad*SS;
    const int x0=(int)floorf(cx2-r2-1.5f), x1=(int)ceilf(cx2+r2+1.5f);
    const int y0=(int)floorf(cy2-r2-1.5f), y1=(int)ceilf(cy2+r2+1.5f);
    for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x){
        const float d=hypotf(x+0.5f-cx2,y+0.5f-cy2)-r2;
        ss_over(x,y,c[0],c[1],c[2], clampf(0.5f-d,0.f,1.f)*a);
    }
}
static void glow_disc(float cx,float cy,float rad,float sigma,const uint8_t *c,float intens)
{
    const float cx2=cx*SS, cy2=cy*SS, r2=rad*SS, sig=sigma*SS;
    const float reach=r2+sig*4.f;
    const int x0=(int)floorf(cx2-reach-1.f), x1=(int)ceilf(cx2+reach+1.f);
    const int y0=(int)floorf(cy2-reach-1.f), y1=(int)ceilf(cy2+reach+1.f);
    const float inv=1.f/(2.f*sig*sig);
    for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x){
        const float d=hypotf(x+0.5f-cx2,y+0.5f-cy2)-r2;
        const float g=(d<0.f)?intens: intens*expf(-d*d*inv);
        ss_add(x,y,c[0],c[1],c[2],g);
    }
}
/* ring/arc: a0, span in radians, math angles (0=east, +ccw, y-up). span>0 ccw, span<0 cw. */
static void stroke_arc(float cx,float cy,float rad,float halfw,float a0,float span,
                       const uint8_t *c,float a,float glow_sig,float glow_i,
                       bool gradient, const uint8_t *c1)
{
    const float cx2=cx*SS, cy2=cy*SS, r2=rad*SS, hw=halfw*SS;
    const float extra=(glow_sig>0?glow_sig*4.f:1.5f)*SS;
    const int x0=(int)floorf(cx2-r2-hw-extra), x1=(int)ceilf(cx2+r2+hw+extra);
    const int y0=(int)floorf(cy2-r2-hw-extra), y1=(int)ceilf(cy2+r2+hw+extra);
    const float sp=fabsf(span);
    const float invg=(glow_sig>0.f)?1.f/(2.f*(glow_sig*SS)*(glow_sig*SS)):0.f;
    for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x){
        const float dx=x+0.5f-cx2, dy=y+0.5f-cy2;
        const float rho=hypotf(dx,dy);
        const float sdf=fabsf(rho-r2)-hw;
        /* math angle: atan2(up, right) = atan2(-dy, dx) because SS y is down */
        const float ang=atan2f(-dy,dx);
        float dlt=(span>=0.f)?wrap2pi(ang-a0):wrap2pi(a0-ang);
        const float past=(dlt>sp)?(dlt-sp):0.f;
        /* angular end AA, in pixels */
        const float endc=(dlt<=sp)?1.f: clampf(0.5f-past*fmaxf(rho,1.f),0.f,1.f);
        if(endc<=0.f && (glow_i<=0.f||dlt>sp+0.4f)) continue;
        float cr=c[0],cg=c[1],cb=c[2];
        if(gradient && c1 && dlt<=sp){
            const float t=clampf(dlt/sp,0.f,1.f);
            cr=c[0]+(c1[0]-c[0])*t; cg=c[1]+(c1[1]-c[1])*t; cb=c[2]+(c1[2]-c[2])*t;
        }
        const float cov=clampf(0.5f-sdf,0.f,1.f)*endc;
        if(cov>0.f) ss_over(x,y,cr,cg,cb,cov*a);
        if(glow_i>0.f && invg>0.f && dlt<=sp+0.15f){
            const float gd=sdf>0.f?sdf:0.f;
            const float g=glow_i*expf(-gd*gd*invg)*((dlt<=sp)?1.f:endc);
            ss_add(x,y,cr,cg,cb,g);
        }
    }
}
static void stroke_ring(float cx,float cy,float rad,float halfw,const uint8_t *c,float a,float gsig,float gi)
{
    stroke_arc(cx,cy,rad,halfw,0.f,2.f*(float)M_PI,c,a,gsig,gi,false,nullptr);
}
/* tapered needle, math angle, y-up. len in 1x px. */
static void needle(float cx,float cy,float ang,float len,float w_hub,float w_tip,const uint8_t *c,float gsig,float gi)
{
    const float x0=cx, y0=cy, x1=cx+len*cosf(ang), y1=cy-len*sinf(ang);
    const float vx=x1-x0, vy=y1-y0, L=hypotf(vx,vy);
    if(L<0.1f) return;
    const float ux=vx/L, uy=vy/L;
    /* pad bbox */
    const float pad=(fmaxf(w_hub,w_tip)+gsig*4.f+2.f);
    const int xa=(int)floorf((fminf(x0,x1)-pad)*SS), xb=(int)ceilf((fmaxf(x0,x1)+pad)*SS);
    const int ya=(int)floorf((fminf(y0,y1)-pad)*SS), yb=(int)ceilf((fmaxf(y0,y1)+pad)*SS);
    const float invg=(gsig>0.f)?1.f/(2.f*(gsig*SS)*(gsig*SS)):0.f;
    for(int y=ya;y<=yb;++y) for(int x=xa;x<=xb;++x){
        const float px=(x+0.5f)/SS, py=(y+0.5f)/SS;
        float t=((px-x0)*ux+(py-y0)*uy)/L;
        const float tclip=clampf(t,0.f,1.f);
        const float qx=x0+ux*tclip*L, qy=y0+uy*tclip*L;
        const float d=hypotf(px-qx,py-qy);
        const float hw=w_hub+(w_tip-w_hub)*clampf(t,0.f,1.f);
        /* cap the tip with a slightly pointed end: hw already small at tip */
        const float sdf=d-hw;
        const float cov=clampf(0.5f-sdf,0.f,1.f);
        if(cov>0.f) ss_over(x,y,c[0],c[1],c[2],cov);
        if(gi>0.f && invg>0.f){
            const float gd=sdf>0.f?sdf:0.f;
            ss_add(x,y,c[0],c[1],c[2], gi*expf(-gd*gd*invg));
        }
    }
}
static void capsule(float x0,float y0,float x1,float y1,float hw,const uint8_t *c,float a,float gsig,float gi)
{
    const float vx=x1-x0, vy=y1-y0, L=hypotf(vx,vy);
    const float ux=L>0.1f?vx/L:1.f, uy=L>0.1f?vy/L:0.f;
    const float pad=hw+gsig*4.f+2.f;
    const int xa=(int)floorf((fminf(x0,x1)-pad)*SS), xb=(int)ceilf((fmaxf(x0,x1)+pad)*SS);
    const int ya=(int)floorf((fminf(y0,y1)-pad)*SS), yb=(int)ceilf((fmaxf(y0,y1)+pad)*SS);
    const float invg=(gsig>0.f)?1.f/(2.f*(gsig*SS)*(gsig*SS)):0.f;
    for(int y=ya;y<=yb;++y) for(int x=xa;x<=xb;++x){
        const float px=(x+0.5f)/SS, py=(y+0.5f)/SS;
        const float t=L>0.1f?clampf(((px-x0)*ux+(py-y0)*uy)/L,0.f,1.f):0.f;
        const float qx=x0+ux*t*L, qy=y0+uy*t*L;
        const float sdf=hypotf(px-qx,py-qy)-hw;
        const float cov=clampf(0.5f-sdf,0.f,1.f);
        if(cov>0.f) ss_over(x,y,c[0],c[1],c[2],cov*a);
        if(gi>0.f && invg>0.f){
            const float gd=sdf>0.f?sdf:0.f;
            ss_add(x,y,c[0],c[1],c[2], gi*expf(-gd*gd*invg));
        }
    }
}
static void polyline(const float *xy,int n,float hw,const uint8_t *c,float a,float gsig,float gi)
{
    for(int i=0;i<n-1;++i) capsule(xy[2*i],xy[2*i+1],xy[2*i+2],xy[2*i+3],hw,c,a,gsig,gi);
}
/* rounded horizontal bar, fill 0..1, track + glow fill */
static void hbar(float x,float y,float w,float h,float fill,const uint8_t *track,const uint8_t *lit,float gsig)
{
    const float hw=h*0.5f, cy=y+hw, x0=x+hw, x1=x+w-hw;
    capsule(x0,cy,x1,cy,hw,track,1.f,0.f,0.f);
    if(fill>0.f){
        const float inner=fmaxf(0.f,w*fill-hw);
        const float xlit1=x0+inner;
        capsule(x0,cy,fminf(xlit1,x1),cy,hw*0.78f,lit,1.f,gsig,0.35f);
    }
}

static void arc_caps(float cx,float cy,float rad,float hw,float a0,float span,const uint8_t *c0,const uint8_t *c1,float gsig,float gi)
{
    const float x0=cx+rad*cosf(a0), y0=cy-rad*sinf(a0);
    const float a1=a0+span;
    const float x1=cx+rad*cosf(a1), y1=cy-rad*sinf(a1);
    if(gsig>0.f){ glow_disc(x0,y0,hw,gsig,c0,gi); glow_disc(x1,y1,hw,gsig,c1,gi); }
    fill_disc(x0,y0,hw,c0,1.f); fill_disc(x1,y1,hw,c1,1.f);
}
/* vertical VU tube, fill 0..1 from the bottom, red danger zone in the top 22%. */
static void vu_column(float x,float y,float w,float h,float fill)
{
    const float hw=w*0.5f, cx=x+hw, y0=y+hw, y1=y+h-hw, travel=h-w;
    fill=clampf(fill,0.f,1.f);
    const float men=y1-travel*fill; /* meniscus y */
    const uint8_t red[3]={248,48,56};
    const float amber[3]={248.f,188.f,32.f};
    const float pad=hw+5.f;
    const int xa=(int)floorf((x-pad)*SS), xb=(int)ceilf((x+w+pad)*SS);
    const int ya=(int)floorf((y-pad)*SS), yb=(int)ceilf((y+h+pad)*SS);
    for(int iy=ya;iy<=yb;++iy) for(int ix=xa;ix<=xb;++ix){
        const float px=(ix+0.5f)/SS, py=(iy+0.5f)/SS;
        const float t=clampf(py-y0,0.f,travel);
        const float qx=cx, qy=y0+t;
        const float d=hypotf(px-qx,py-qy);
        const float sdf=d-hw;
        const float cov=clampf(0.5f-sdf,0.f,1.f);
        /* soft outer glow, kept tight so the tube stays sharp */
        if(sdf>0.f){
            const float g=0.10f*expf(-sdf*sdf/(2.f*1.1f*1.1f));
            ss_add(ix,iy,TK[0],TK[1],TK[2],g);
        }
        if(cov<=0.f) continue;
        /* glass body */
        float r=TD[0], g=TD[1], b=TD[2];
        /* red danger tint in the top 22% of travel */
        if(py<y0+travel*0.22f){
            const float u=clampf(((y0+travel*0.22f)-py)/(travel*0.22f),0.f,1.f);
            r=r+(red[0]-r)*u*0.55f; g=g+(red[1]-g)*u*0.55f; b=b+(red[2]-b)*u*0.55f;
        }
        /* liquid fill below the meniscus */
        if(py>=men-0.35f){
            const float rel=clampf((y1-py)/fmaxf(travel*fill,1.f),0.f,1.f);
            const float gt=clampf(rel*fill/0.85f,0.f,1.f);
            float fr=TV[0]+(amber[0]-TV[0])*gt;
            float fg=TV[1]+(amber[1]-TV[1])*gt;
            float fb=TV[2]+(amber[2]-TV[2])*gt;
            /* meniscus highlight */
            const float md=fabsf(py-men);
            const float mh=clampf(1.2f-md,0.f,1.f);
            fr=fr+(255.f-fr)*mh*0.55f; fg=fg+(255.f-fg)*mh*0.55f; fb=fb+(255.f-fb)*mh*0.55f;
            /* left specular glass line */
            const float spec=clampf(1.0f-fabsf(px-(cx-hw*0.45f))/1.1f,0.f,1.f)*0.35f;
            fr=fr+(255.f-fr)*spec; fg=fg+(255.f-fg)*spec; fb=fb+(255.f-fb)*spec;
            r=fr; g=fg; b=fb;
        }
        /* inner rim (slightly lighter at the edge) */
        const float rim=clampf(1.f-fabsf(sdf+0.6f)/0.7f,0.f,1.f)*0.25f;
        r=r+(TK[0]-r)*rim; g=g+(TK[1]-g)*rim; b=b+(TK[2]-b)*rim;
        ss_over(ix,iy,r,g,b,cov);
    }
    glow_disc(cx,men,hw*0.55f,2.2f,TV,0.28f);
    for(int i=0;i<=4;++i){
        const float yy=y1-travel*(i/4.f);
        const uint8_t *tc=(i>=4)?red:TK;
        capsule(x+w+1.2f,yy,x+w+5.0f,yy,0.65f,tc,0.9f,0.6f,0.10f);
    }
}

static void soft_rim()
{
    /* glowing inner rim, not a hard multi-box frame */
    const float sig=1.6f*SS, inv=1.f/(2.f*sig*sig);
    for(int y=0;y<SSH;++y) for(int x=0;x<SSW;++x){
        const float px=(x+0.5f)/SS, py=(y+0.5f)/SS;
        const float d=fminf(fminf(px,py),fminf((PW-1)-px,(PH-1)-py));
        const float g=0.42f*expf(-d*d*SS*SS*inv);
        ss_add(x,y,TK[0],TK[1],TK[2],g);
    }
    /* hairline edge in key colour */
    capsule(1.2f,1.2f,PW-2.2f,1.2f,0.55f,TK,0.7f,1.1f,0.18f);
    capsule(1.2f,PH-2.2f,PW-2.2f,PH-2.2f,0.55f,TK,0.7f,1.1f,0.18f);
    capsule(1.2f,1.2f,1.2f,PH-2.2f,0.55f,TK,0.7f,1.1f,0.18f);
    capsule(PW-2.2f,1.2f,PW-2.2f,PH-2.2f,0.55f,TK,0.7f,1.1f,0.18f);
}
static void radial_tick(float cx,float cy,float ang,float r0,float r1,float hw,const uint8_t *c,float a,float gsig,float gi)
{
    const float x0=cx+r0*cosf(ang), y0=cy-r0*sinf(ang);
    const float x1=cx+r1*cosf(ang), y1=cy-r1*sinf(ang);
    capsule(x0,y0,x1,y1,hw,c,a,gsig,gi);
}

/* crisp GLCD on the 1x buffer. sc=1 is 7px tall; sc=2 is 14px; sc=3 is 21px. */
static int glcd_w(const char *s,int sc){ const int n=(int)strlen(s); return n?(6*n-1)*sc:0; }
static void glcd_at(int x,int y,const char *s,int sc,const uint8_t *c,float a)
{
    if(a<=0.f) return;
    for(int i=0;s[i];++i){
        const int g=s[i]<32||s[i]>126?0:s[i]-32;
        for(int cx=0;cx<5;++cx){
            const uint8_t bits=LV_GLCD[g*5+cx];
            for(int cy=0;cy<7;++cy) if(bits>>cy&1){
                for(int j=0;j<sc;++j) for(int k=0;k<sc;++k){
                    const int xx=x+sc*(6*i+cx)+k, yy=y+sc*cy+j;
                    if(xx<0||yy<0||xx>=PW||yy>=PH) continue;
                    uint8_t *q=pan888+(yy*PW+xx)*3;
                    q[0]=(uint8_t)(c[0]*a+q[0]*(1.f-a)+0.5f);
                    q[1]=(uint8_t)(c[1]*a+q[1]*(1.f-a)+0.5f);
                    q[2]=(uint8_t)(c[2]*a+q[2]*(1.f-a)+0.5f);
                }
            }
        }
    }
}
static void glcd_center(int cx,int y,const char *s,int sc,const uint8_t *c,float a)
{
    glcd_at(cx-glcd_w(s,sc)/2,y,s,sc,c,a);
}
static void stamp_exdata(int x,int y)
{
    /* 5x7 (the 7px minimum), dim, never #. */
    glcd_at(x,y,"EX.DATA",1,TK,0.55f);
}
static void stamp_exdata_br(){ stamp_exdata(PW-2-glcd_w("EX.DATA",1), PH-10); }
static void stamp_exdata_tr(){ stamp_exdata(PW-2-glcd_w("EX.DATA",1), 2); }
static void stamp_exdata_tl(){ stamp_exdata(3, 2); }

static void gar(float t,uint8_t *o) /* green -> amber -> red */
{
    t=clampf(t,0.f,1.f);
    const float g0[3]={48,220,90}, g1[3]={248,188,32}, g2[3]={248,40,48};
    const float *a,*b; float u;
    if(t<0.5f){ a=g0; b=g1; u=t/0.5f; } else { a=g1; b=g2; u=(t-0.5f)/0.5f; }
    o[0]=(uint8_t)(a[0]+(b[0]-a[0])*u+0.5f);
    o[1]=(uint8_t)(a[1]+(b[1]-a[1])*u+0.5f);
    o[2]=(uint8_t)(a[2]+(b[2]-a[2])*u+0.5f);
}

/* ========== role instruments ========== */

/* GIBSONbot: large RSSI semicircle + needle at -42 dBm, sparkline, WIFI 12 / BLE 8. */
static void panel_gibson(int slot)
{
    theme_pick(slot); ss_clear(); soft_rim();
    /* raised so the arc ends don't sit on the floor; room for a left-bowl sparkline */
    const float cx=64.f, cy=58.f, rad=44.f, hw=3.8f;
    const float a0=deg(180.f), span=deg(-180.f);
    stroke_arc(cx,cy,rad,hw,a0,span, TD, 0.95f, 1.1f, 0.08f, false, nullptr);
    arc_caps(cx,cy,rad,hw,a0,span,TD,TD,0.9f,0.08f);
    const float rssi=-42.f, t=(rssi+90.f)/90.f;
    const float span_lit=span*t;
    stroke_arc(cx,cy,rad,hw,a0,span_lit, TK, 1.f, 1.5f, 0.20f, true, TV);
    arc_caps(cx,cy,rad,hw,a0,span_lit,TK,TV,1.2f,0.16f);
    const float ticks[4]={-90.f,-60.f,-30.f,0.f};
    for(int i=0;i<4;++i){
        const float tt=(ticks[i]+90.f)/90.f;
        const float ang=deg(180.f)-tt*(float)M_PI;
        radial_tick(cx,cy,ang, rad+hw+0.6f, rad+hw+6.5f, 0.8f, TV, 1.f, 1.0f, 0.22f);
    }
    const float nang=deg(180.f)-t*(float)M_PI;
    glow_disc(cx,cy,3.2f,3.5f,TV,0.35f);
    needle(cx,cy,nang, rad-2.0f, 1.7f, 0.55f, TW, 1.6f, 0.4f);
    fill_disc(cx,cy,3.0f,TV,1.f);
    fill_disc(cx,cy,1.3f,TW,1.f);

    /* sparkline in the left bowl, stopping short of the needle; tight glow so it stays a line */
    float sp[36];
    const float raw[18]={0.22f,0.28f,0.35f,0.30f,0.48f,0.72f,0.58f,0.40f,0.66f,0.92f,0.55f,0.33f,0.44f,0.70f,0.50f,0.36f,0.42f,0.31f};
    const float sx0=22.f, sx1=54.f, sy0=34.f, sy1=48.f;
    for(int i=0;i<18;++i){ sp[2*i]=sx0+(sx1-sx0)*i/17.f; sp[2*i+1]=sy1-(sy1-sy0)*raw[i]; }
    polyline(sp,18,1.05f,TV,1.f,0.9f,0.12f);

    downsample_panel();
    stamp_exdata_tl();
    glcd_center((int)cx, 20, "-42 dBm", 1, TW, 1.f);
    glcd_at(114, 10, "WIFI", 1, TK, 0.95f);
    glcd_at(114, 20, "12", 2, TV, 1.f);
    glcd_at(114, 42, "BLE", 1, TK, 0.95f);
    glcd_at(114, 52, "8", 2, TV, 1.f);
}

/* HACKbot: 270° TRACE power ring at 42%, green-amber-red, FILES bar, L2 + time. */
static void panel_hackbot(int slot)
{
    theme_pick(slot); ss_clear(); soft_rim();
    /* slightly higher / tighter so the horseshoe sits cleanly above the FILES bar */
    const float cx=76.f, cy=30.f, rad=24.5f, hw=4.8f;
    const float a0=deg(210.f), span=deg(-270.f);
    stroke_arc(cx,cy,rad,hw,a0,span, TD, 0.95f, 1.2f, 0.10f, false, nullptr);
    arc_caps(cx,cy,rad,hw,a0,span,TD,TD,1.0f,0.10f);
    const float tr=0.42f;
    const int SEG=18;
    uint8_t c0[3], c1[3];
    for(int i=0;i<SEG;++i){
        const float t0=tr*i/SEG, t1=tr*(i+1)/SEG;
        gar(t0,c0); gar(t1,c1);
        stroke_arc(cx,cy,rad,hw, a0+span*t0, span*(t1-t0), c0, 1.f, 2.0f, 0.40f, true, c1);
    }
    uint8_t tip[3]; gar(tr,tip);
    const float tipa=a0+span*tr;
    glow_disc(cx+rad*cosf(tipa), cy-rad*sinf(tipa), 2.2f, 3.4f, tip, 0.55f);
    arc_caps(cx,cy,rad,hw,a0,span*tr,c0,tip,1.6f,0.25f); /* c0 is last seg start; start cap should be green */
    uint8_t g0[3]; gar(0.f,g0);
    fill_disc(cx+rad*cosf(a0), cy-rad*sinf(a0), hw, g0, 1.f);
    for(int i=0;i<=10;++i){
        const float ang=a0+span*(i/10.f);
        radial_tick(cx,cy,ang, rad-hw-0.1f, rad-hw+2.0f, 0.5f, TW, 0.35f, 0.f, 0.f);
    }

    hbar(36.f, 62.f, 80.f, 6.2f, 0.5f, TD, TV, 2.0f);

    downsample_panel();
    stamp_exdata_tl();
    glcd_center((int)cx, (int)cy-11, "42", 2, TW, 1.f);
    glcd_center((int)cx, (int)cy+5, "TRACE", 1, TK, 0.95f);
    glcd_at(6, 62, "L2", 1, TK, 0.95f);
    glcd_at(118, 62, "37.8S", 1, TK, 0.95f);
    glcd_at(36, 53, "FILES  1/2", 1, TV, 0.95f);
}

/* TERMINALbot: vertical threat VU + scrolling alert waveform, ALERTS 2 large. */
static void panel_terminal(int slot)
{
    theme_pick(slot); ss_clear(); soft_rim();
    /* tall smooth VU on the left; red zone at the top; fill modest at 2 alerts (EX.DATA 0.40) */
    vu_column(10.f, 5.f, 18.f, 48.f, 0.40f);

    /* waveform along the bottom, full width, clear of the VU */
    float wf[64];
    const int N=32;
    for(int i=0;i<N;++i){
        const float u=i/(float)(N-1);
        const float x=8.f+u*136.f;
        float s=0.50f+0.22f*sinf(u*18.f)+0.10f*sinf(u*41.f+0.7f);
        s+=0.38f*expf(-powf((u-0.28f)/0.035f,2.f));
        s+=0.48f*expf(-powf((u-0.67f)/0.03f,2.f));
        const float y=66.f - clampf(s,0.f,1.1f)*11.f;
        wf[2*i]=x; wf[2*i+1]=y;
    }
    polyline(wf,N,1.15f,TV,0.95f,1.6f,0.30f);

    downsample_panel();
    stamp_exdata_tr();
    glcd_center(80, 10, "ALERTS", 1, TK, 0.95f);
    glcd_center(80, 22, "2", 3, TV, 1.f);
}

/* BOTpheus: radar orbit of 18 pillar dots, glowing locked one, center 18 SEEN, LAST AIR TAG. */
static void panel_botpheus(int slot)
{
    theme_pick(slot); ss_clear(); soft_rim();
    const float cx=76.f, cy=32.f, Rring=21.0f, Rdot=26.0f;
    stroke_ring(cx,cy,Rring,0.55f, TK, 0.40f, 1.0f, 0.10f);
    stroke_ring(cx,cy,Rring*0.58f,0.45f, TD, 0.50f, 0.7f, 0.05f);
    const int locked=4; /* AIR TAG, itw_mo_sel=4 */
    /* faint beam to the locked pillar */
    {
        const float ang=deg(90.f)-locked*deg(20.f);
        capsule(cx,cy, cx+(Rdot-4.f)*cosf(ang), cy-(Rdot-4.f)*sinf(ang), 1.1f, TV, 0.28f, 2.2f, 0.18f);
    }
    for(int i=0;i<18;++i){
        const float ang=deg(90.f) - i*deg(20.f);
        const float x=cx+Rdot*cosf(ang), y=cy-Rdot*sinf(ang);
        if(i==locked){
            glow_disc(x,y,3.6f,4.6f,TV,0.70f);
            fill_disc(x,y,3.3f,TV,1.f);
            fill_disc(x,y,1.5f,TW,1.f);
        } else {
            glow_disc(x,y,2.3f,2.2f,TK,0.22f);
            fill_disc(x,y,2.35f,TK,0.95f);
        }
    }
    downsample_panel();
    stamp_exdata_tl();
    glcd_center((int)cx, (int)cy-12, "18", 2, TW, 1.f);
    glcd_center((int)cx, (int)cy+4, "SEEN", 1, TK, 0.95f);
    glcd_center((int)cx, 61, "LAST  AIR TAG", 1, TV, 1.f);
}

static void usage(){
    fprintf(stderr,"usage: bigbot_v3_compose base.rgb out.rgb face gold|silver|raw|none [rawW rawH raw.rgb] panel none|botpheus|gibson|hackbot|terminal|control slot name\n");
    exit(2);
}

int main(int argc,char **argv)
{
    pal_init();
    if(argc<2) usage();
    if(!strcmp(argv[1],"selftest")){
        if(argc<3){ fprintf(stderr,"selftest base.rgb\n"); return 2; }
        auto frame=load_rgb(argv[2], (size_t)W*H*3);
        panel_control_default(11);
        int n=0; int x0=999,y0=999,x1=-1,y1=-1;
        for(int y=0;y<PH;++y) for(int x=0;x<PW;++x){
            uint8_t rgb[3]; rgb565_to_888(lv_cb_pan[y*PW+x], rgb);
            uint8_t *q=frame.data()+((PY0+y)*W+(PX0+x))*3;
            if(q[0]!=rgb[0]||q[1]!=rgb[1]||q[2]!=rgb[2]){ ++n; if(PX0+x<x0)x0=PX0+x; if(PY0+y<y0)y0=PY0+y; if(PX0+x>x1)x1=PX0+x; if(PY0+y>y1)y1=PY0+y; }
        }
        printf("control-panel selftest vs still: diff_px=%d bbox=%d,%d,%d,%d\n", n, x0,y0,x1,y1);
        return 0;
    }
    if(argc<7) usage();
    const char *basep=argv[1], *outp=argv[2], *facek=argv[3];
    int ai=4;
    auto frame=load_rgb(basep,(size_t)W*H*3);
    std::vector<uint8_t> face((size_t)FACE_S*FACE_S*3,0);
    if(!strcmp(facek,"gold")) paint_gold(face.data(),FACE_S);
    else if(!strcmp(facek,"silver")) paint_silver_eyes(face.data(),FACE_S);
    else if(!strcmp(facek,"raw")){
        if(argc<7) usage();
        int sw=atoi(argv[4]), sh=atoi(argv[5]);
        auto raw=load_rgb(argv[6],(size_t)sw*sh*3);
        nn_blit_fit(face.data(),FACE_S,raw.data(),sw,sh); ai=7;
    } else if(!strcmp(facek,"none")) { ai=4; }
    else usage();
    if(strcmp(facek,"none")!=0) blit_face(frame.data(),face.data(),FACE_S);
    const char *panelk = argv[ai];
    int slot = atoi(argv[ai+1]);
    const char *name = (ai+2<argc)?argv[ai+2]:"";
    if(!strcmp(panelk,"botpheus")) panel_botpheus(slot);
    else if(!strcmp(panelk,"gibson")) panel_gibson(slot);
    else if(!strcmp(panelk,"hackbot")) panel_hackbot(slot);
    else if(!strcmp(panelk,"terminal")) panel_terminal(slot);
    else if(!strcmp(panelk,"none")||!strcmp(panelk,"control")) { /* leave locked panel */ }
    else usage();
    if(strcmp(panelk,"none")!=0 && strcmp(panelk,"control")!=0) blit_panel888(frame.data());
    if(name[0]) draw_name(frame.data(), name, 231,239,247);
    FILE *o=fopen(outp,"wb"); if(!o){ fprintf(stderr,"write %s\n",outp); return 1; }
    fwrite(frame.data(),1,frame.size(),o); fclose(o);
    return 0;
}
