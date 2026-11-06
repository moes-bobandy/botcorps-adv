#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <stdio.h>
namespace lgfx { struct swap565_t { uint16_t v; }; struct rgb565_t { uint16_t v; }; }
static uint16_t actual[320][240], expected[320][240];
static uint16_t sw(uint16_t x) { return (x>>8)|(x<<8); }
struct MockDisplay {
 const lgfx::swap565_t *pending=nullptr; int x,y,w,h; int transfers=0;
 void waitDisplay() {} // LCD's real waitDisplay is a no-op: waitDMA is essential.
 void waitDMA() { if(pending) { auto p=pending;pending=nullptr;pushImage(x,y,w,h,p); } }
 void pushImageDMA(int a,int b,int c,int d,const lgfx::swap565_t *p) { assert(!pending);x=a;y=b;w=c;h=d;pending=p;++transfers; }
 void pushImage(int x,int y,int w,int h,const lgfx::swap565_t *p) { assert(!pending); for(int j=0;j<h;++j)for(int i=0;i<w;++i)actual[y+j][x+i]=sw(p[j*w+i].v); }
 void pushImage(int x,int y,int w,int h,const lgfx::rgb565_t *p) { assert(!pending); for(int j=0;j<h;++j)for(int i=0;i<w;++i)actual[y+j][x+i]=p[j*w+i].v; }
 void endWrite() { assert(!pending); }
} g_cyd;
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


static void draw(int x,int y,int w,int h,int seed,bool wire) {
 uint16_t px[240*70];
 for(int j=0;j<h;++j)for(int i=0;i<w;++i) {uint16_t v=(uint16_t)(seed+j*240+i);expected[y+j][x+i]=v;px[j*w+i]=wire?sw(v):v;}
 ui_dma_draw(x,y,w,h,px,wire);
 memset(px,0,sizeof px); // source lifetime ends before asynchronous output
}
int main() {
 static uint16_t strips[2][240*48];s_dma_buf[0]=strips[0];s_dma_buf[1]=strips[1];s_dma_rows=48;
 for(int y=0;y<100;++y) draw(0,y,240,1,100+y*240,y&1);
 draw(10,100,25,16,500,false);draw(10,116,25,32,600,true);
 draw(50,120,100,70,700,false); // direct large blit, source must be consumed
 draw(30,190,90,2,800,true);draw(30,193,90,2,900,false); // discontinuity
 ui_live_end();assert(memcmp(actual,expected,sizeof actual)==0);
 s_dma_rows=0;draw(0,200,240,5,1000,true);ui_live_end();
 assert(memcmp(actual,expected,sizeof actual)==0);
 printf("DMA coalescer: native/wire, ping-pong, source reuse, large blit, gaps, fallback, idle-before-CS PASS (%d DMA strips)\n",g_cyd.transfers);
}
