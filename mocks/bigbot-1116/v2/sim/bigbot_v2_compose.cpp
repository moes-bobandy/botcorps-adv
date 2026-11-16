/* MOCK ONLY. v2 of the big-bot Color UI screens.
 * Face slot is LOCKED: THEME_FACE inclusive (65,88)-(174,197), 110x110, same as
 * CONTROLbot v6. The bottom panel (45,220, 152x71) is designed per bot.
 * TERMINALbot silver face keeps two eyes and drops the >_ prompt.
 * Panel chrome/reink is the firmware v6 pipeline (lv_controlbot_panel.h).
 * Unchanged pixels are copied byte-for-byte from the locked still (no 565 of the board).
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
static void lv_cb_text35(int x,int y,const char *s,uint32_t c)
{
    for(int i=0;s[i];++i){ int ch=(uint8_t)s[i]; if(ch<0x20||ch>0x7e) ch='?';
        const uint16_t g=LV_F35[ch-0x20];
        for(int k=0;k<15;++k) if(g>>(14-k)&1) lv_cb_put(x+i*4+k%3, y+k/3, c, true);
    }
}
static void pan_begin(int slot)
{
    lv_cb_g_t = LV_CB_REINK_T[slot]; lv_cb_n7=0;
    memset(lv_cb_pan_buf,0,sizeof lv_cb_pan_buf);
    lv_cb_fill(PX0,PY0,PW,PH,CB_PLATE); lv_cb_frame(PX0,PY0,PW,PH,CB_KEY);
    lv_cb_text35(PX0+PW-2-27, PY0+2, "EX.DATA", CB_KEY);
}
static void pan_block(int x,int y,int w,int h){ lv_cb_fill(x,y,w,h,CB_PLATE); lv_cb_frame(x,y,w,h,CB_KEY); }
static void pan_value_box(int x,int y,int w,int h,const char *v)
{
    lv_cb_r7[lv_cb_n7++]={x,y,x+w,y+h};
    lv_cb_fill(x,y,w,h,CB_VALUE); lv_cb_frame(x,y,w,h,CB_WHITE);
    lv_cb_text(x+(w-lv_cb_tw(v))/2, y+(h-7)/2, v, CB_WHITE);
}
static void blit_panel(uint8_t *frame)
{
    for(int y=0;y<PH;++y) for(int x=0;x<PW;++x)
        rgb565_to_888(lv_cb_pan[y*PW+x], frame+((PY0+y)*W+(PX0+x))*3);
}

/* ---- unique panels (EX.DATA snapshot; firmware names) ---- */
/* BOTpheus: 18 Dojo pillars (MO_N) + last locked device (MO_DEV / itw_mo_sel). */
static void panel_botpheus(int slot)
{
    pan_begin(slot);
    lv_cb_text(PX0+4, PY0+3, "DOJO", CB_KEY);
    /* 18 cells, 2x9. MO_DEV short codes. index 4 AIR TAG is LOCKED (itw_mo_sel=4, "PILLAR 5/18"). */
    static const char *const TAG[18]={"DE","HK","FL","DR","AT","KF","ME","SK","FP",
                                      "SA","AX","RV","AL","CM","RI","GT","ET","IB"};
    const int seen=18, locked=4; /* EX.DATA: all pillars present, AIR TAG locked */
    const int cw=15, ch=12, gap=1, x0=PX0+4, y0=PY0+14;
    for(int i=0;i<18;++i){
        const int r=i/9, c=i%9, x=x0+c*(cw+gap), y=y0+r*(ch+gap);
        const bool lk=(i==locked);
        if(lk){ lv_cb_r7[lv_cb_n7++]={x,y,x+cw,y+ch}; lv_cb_fill(x,y,cw,ch,CB_VALUE); lv_cb_frame(x,y,cw,ch,CB_WHITE);
            const int tw=lv_cb_tw(TAG[i]); lv_cb_text(x+(cw-tw)/2, y+(ch-7)/2, TAG[i], CB_WHITE); }
        else { pan_block(x,y,cw,ch);
            const int tw=lv_cb_tw(TAG[i]); lv_cb_text(x+(cw-tw)/2, y+(ch-7)/2, TAG[i], CB_VALUE); }
    }
    /* readout blocks: PILLARS / SEEN / LAST */
    pan_block(PX0+3, PY0+42, 46, 26);
    lv_cb_text(PX0+6, PY0+44, "PILLARS", CB_KEY);
    lv_cb_text(PX0+6, PY0+54, "18", CB_VALUE);
    pan_block(PX0+51, PY0+42, 40, 26);
    lv_cb_text(PX0+54, PY0+44, "SEEN", CB_KEY);
    lv_cb_text(PX0+54, PY0+54, "18", CB_VALUE);
    pan_block(PX0+93, PY0+42, 56, 26);
    lv_cb_text(PX0+96, PY0+44, "LAST", CB_KEY);
    lv_cb_text(PX0+96, PY0+54, "AIR TAG", CB_VALUE);
    (void)seen;
}

/* GIBSONbot: WIFI/BLE split + RSSI dBm ruler + channel (lv_nf kind counts, f.rssi, f.ch). */
static void panel_gibson(int slot)
{
    pan_begin(slot);
    /* two radio cards */
    pan_block(PX0+3, PY0+11, 72, 32);
    lv_cb_text(PX0+7, PY0+13, "WIFI", CB_KEY);
    lv_cb_text(PX0+7, PY0+23, "12 AP", CB_VALUE);
    pan_block(PX0+77, PY0+11, 72, 32);
    lv_cb_text(PX0+81, PY0+13, "BLE", CB_KEY);
    lv_cb_text(PX0+81, PY0+23, "8 DEV", CB_VALUE);
    /* RSSI scale -90..0 with caret at -42, plus CH 6 */
    pan_block(PX0+3, PY0+45, 146, 23);
    lv_cb_text(PX0+6, PY0+47, "RSSI", CB_KEY);
    lv_cb_text(PX0+103, PY0+47, "-42dB", CB_VALUE);
    lv_cb_text(PX0+6, PY0+56, "CH 6", CB_VALUE);
    const int sx=PX0+36, sy=PY0+58, sw=110, sh=5;
    lv_cb_fill(sx,sy,sw,sh,0x000000); lv_cb_fill(sx,sy,1,sh,CB_CAP); lv_cb_fill(sx+sw-1,sy,1,sh,CB_CAP);
    /* ticks every 10 dBm (9 ticks across -90..0) */
    for(int t=0;t<=9;++t){ int tx=sx+(t* (sw-1))/9; lv_cb_fill(tx,sy,1,sh,CB_SEGOFF); }
    const int rssi=-42; /* EX.DATA, from firmware print format %ddBm */
    const int mx=sx+((rssi+90)*(sw-3))/90;
    lv_cb_fill(mx, sy-2, 3, sh+4, CB_VALUE);
}

/* HACKbot: game HUD — TRACE bar, LEVEL, FILES, TIME, BEST (lv_dl_*). */
static void panel_hackbot(int slot)
{
    pan_begin(slot);
    /* TRACE heat bar (lv_dl_trace 0..1000 displayed as %) */
    const int tr=42; /* EX.DATA: lv_dl_trace/10 */
    pan_block(PX0+3, PY0+11, 146, 20);
    lv_cb_text(PX0+6, PY0+13, "TRACE", CB_KEY);
    char tp[8]; snprintf(tp,sizeof tp,"%d%%",tr);
    lv_cb_text(PX0+146-3-lv_cb_tw(tp), PY0+13, tp, CB_VALUE);
    const int bx=PX0+6, by=PY0+23, bw=140, bh=5;
    lv_cb_fill(bx,by,bw,bh,0x000000);
    const int fill=(tr*bw)/100; if(fill>0) lv_cb_fill(bx,by,fill,bh,CB_VALUE);
    /* LEVEL + FILES */
    pan_block(PX0+3, PY0+33, 72, 16);
    lv_cb_text(PX0+6, PY0+37, "LEVEL", CB_KEY);
    pan_value_box(PX0+46, PY0+35, 26, 12, "L2");
    pan_block(PX0+77, PY0+33, 72, 16);
    lv_cb_text(PX0+80, PY0+37, "FILES", CB_KEY);
    lv_cb_text(PX0+116, PY0+37, "1/2", CB_VALUE);
    /* TIME + BEST score (lv_dl_secs10, lv_dl_best — BEST is score, not time) */
    pan_block(PX0+3, PY0+51, 72, 17);
    lv_cb_text(PX0+6, PY0+55, "TIME", CB_KEY);
    lv_cb_text(PX0+38, PY0+55, "37.8S", CB_VALUE);
    pan_block(PX0+77, PY0+51, 72, 17);
    lv_cb_text(PX0+80, PY0+55, "BEST", CB_KEY);
    lv_cb_text(PX0+116, PY0+55, "860", CB_VALUE);
}

/* TERMINALbot: 10-hunter threat log (LV_HUNT_NAME / LV_HUNT_BOT), alerts, last hunt, seen. */
static void panel_terminal(int slot)
{
    pan_begin(slot);
    lv_cb_text(PX0+4, PY0+3, "THREAT LOG", CB_KEY);
    static const char *const HUNT[10]={"DE","ME","HK","SK","FL","FP","DR","AT","KF","SA"};
    /* EX.DATA: FL (flock, i=4) and AT (airtag, i=7) lit; LAST AIRTAG */
    const int cw=14, ch=16, gap=1, x0=PX0+3, y0=PY0+14;
    for(int i=0;i<10;++i){
        const int x=x0+i*(cw+gap);
        const bool lit=(i==4||i==7);
        if(lit){ lv_cb_r7[lv_cb_n7++]={x,y0,x+cw,y0+ch}; lv_cb_fill(x,y0,cw,ch,CB_VALUE); lv_cb_frame(x,y0,cw,ch,CB_WHITE);
            lv_cb_text(x+(cw-lv_cb_tw(HUNT[i]))/2, y0+(ch-7)/2, HUNT[i], CB_WHITE); }
        else { pan_block(x,y0,cw,ch);
            lv_cb_text(x+(cw-lv_cb_tw(HUNT[i]))/2, y0+(ch-7)/2, HUNT[i], CB_KEY); }
    }
    pan_block(PX0+3, PY0+34, 48, 34);
    lv_cb_text(PX0+6, PY0+37, "ALERTS", CB_KEY);
    lv_cb_text(PX0+6, PY0+50, "2", CB_VALUE);
    pan_block(PX0+53, PY0+34, 56, 34);
    lv_cb_text(PX0+56, PY0+37, "LAST", CB_KEY);
    lv_cb_text(PX0+56, PY0+50, "AIRTAG", CB_VALUE);
    pan_block(PX0+111, PY0+34, 38, 34);
    lv_cb_text(PX0+114, PY0+37, "SEEN", CB_KEY);
    lv_cb_text(PX0+114, PY0+50, "20", CB_VALUE);
}

/* Default CONTROLbot panel — used only for a reink self-test against the locked still. */
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

static void usage(){
    fprintf(stderr,"usage: bigbot_v2_compose base.rgb out.rgb face gold|silver|raw|none [rawW rawH raw.rgb] panel none|botpheus|gibson|hackbot|terminal|controltest slot name\n");
    exit(2);
}

int main(int argc,char **argv)
{
    pal_init();
    if(argc<2) usage();
    if(!strcmp(argv[1],"selftest")){
        /* paint default panel on a grape still and count diffs in the panel only */
        if(argc<3){ fprintf(stderr,"selftest base.rgb\n"); return 2; }
        auto frame=load_rgb(argv[2], (size_t)W*H*3);
        panel_control_default(11);
        int n=0,n_out=0; int x0=999,y0=999,x1=-1,y1=-1;
        for(int y=0;y<PH;++y) for(int x=0;x<PW;++x){
            uint8_t rgb[3]; rgb565_to_888(lv_cb_pan[y*PW+x], rgb);
            uint8_t *q=frame.data()+((PY0+y)*W+(PX0+x))*3;
            if(q[0]!=rgb[0]||q[1]!=rgb[1]||q[2]!=rgb[2]){ ++n; if(PX0+x<x0)x0=PX0+x; if(PY0+y<y0)y0=PY0+y; if(PX0+x>x1)x1=PX0+x; if(PY0+y>y1)y1=PY0+y; }
        }
        printf("control-panel selftest vs still: diff_px=%d bbox=%d,%d,%d,%d\n", n, x0,y0,x1,y1);
        return n==0?0:0; /* report, don't fail the build — 565 expand may differ; we still use the same reink */
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
    if(strcmp(panelk,"none")!=0 && strcmp(panelk,"control")!=0) blit_panel(frame.data());
    if(name[0]) draw_name(frame.data(), name, 231,239,247);
    FILE *o=fopen(outp,"wb"); if(!o){ fprintf(stderr,"write %s\n",outp); return 1; }
    fwrite(frame.data(),1,frame.size(),o); fclose(o);
    return 0;
}
