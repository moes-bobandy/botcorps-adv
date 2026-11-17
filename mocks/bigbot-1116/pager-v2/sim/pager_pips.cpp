/* MOCK ONLY. Paint a 5-pip page indicator into the black 5-px gap
 * immediately under the locked v6 ticker (y=25..29), in theme ink.
 * Does not touch THEME_FACE (65,88)-(174,197) or the bottom panel
 * (45,220)-(196,290). Only blends over near-black pixels in that band.
 *
 * usage: pager_pips in.rgb out.rgb index 0..4
 */
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cmath>
#include <vector>

static const int W=240, H=320;
static const int Y0=25, Y1=29;           /* inclusive pip band, under the ticker hatch */
static const int N=5;
static const float CX0=104.f, PITCH=8.f; /* pip centres at 104,112,120,128,136 */
static const float CY=27.f;
static const float R_OFF=1.35f, R_ON=1.75f;

static void sample_ink(const uint8_t *fr, uint8_t *on, uint8_t *dim)
{
    /* median-ish mean of bright ticker pixels (y=5..17, x=16..223) */
    long sr=0,sg=0,sb=0; int n=0;
    for(int y=5;y<=17;++y) for(int x=16;x<=223;++x){
        const uint8_t *q=fr+(y*W+x)*3;
        const int lum=(q[0]*3+q[1]*6+q[2]*1)/10;
        if(lum>=80){ sr+=q[0]; sg+=q[1]; sb+=q[2]; ++n; }
    }
    if(n<8){ on[0]=192; on[1]=76; on[2]=248; } /* grape fallback */
    else { on[0]=(uint8_t)(sr/n); on[1]=(uint8_t)(sg/n); on[2]=(uint8_t)(sb/n); }
    dim[0]=(uint8_t)(on[0]*52/100); dim[1]=(uint8_t)(on[1]*52/100); dim[2]=(uint8_t)(on[2]*52/100);
}

int main(int argc,char **argv)
{
    if(argc<4){ fprintf(stderr,"usage: pager_pips in.rgb out.rgb index\n"); return 2; }
    const int idx=atoi(argv[3]);
    if(idx<0||idx>=N){ fprintf(stderr,"index 0..4\n"); return 2; }
    FILE *f=fopen(argv[1],"rb"); if(!f){ perror(argv[1]); return 1; }
    std::vector<uint8_t> fr((size_t)W*H*3);
    if(fread(fr.data(),1,fr.size(),f)!=fr.size()){ fprintf(stderr,"short %s\n",argv[1]); return 1; }
    fclose(f);
    uint8_t on[3], dim[3]; sample_ink(fr.data(), on, dim);

    /* AA discs, clipped to the band, only over near-black source pixels */
    auto plot=[&](int x,int y,float r,float g,float b,float a){
        if(a<=0.f||x<4||x>235||y<Y0||y>Y1) return;
        uint8_t *q=fr.data()+(y*W+x)*3;
        if(q[0]>18||q[1]>18||q[2]>18) return; /* don't clobber pin accents or hatch */
        if(a>1.f) a=1.f;
        q[0]=(uint8_t)(r*a+q[0]*(1.f-a)+0.5f);
        q[1]=(uint8_t)(g*a+q[1]*(1.f-a)+0.5f);
        q[2]=(uint8_t)(b*a+q[2]*(1.f-a)+0.5f);
    };
    for(int i=0;i<N;++i){
        const bool lit=(i==idx);
        const float cx=CX0+i*PITCH, rad=lit?R_ON:R_OFF;
        const uint8_t *c=lit?on:dim;
        const float a0=lit?1.f:0.85f;
        const int x0=(int)floorf(cx-rad-1.5f), x1=(int)ceilf(cx+rad+1.5f);
        const int y0=(int)floorf(CY-rad-1.5f), y1=(int)ceilf(CY+rad+1.5f);
        for(int y=y0;y<=y1;++y) for(int x=x0;x<=x1;++x){
            const float d=hypotf(x+0.5f-cx,y+0.5f-CY)-rad;
            const float cov=fminf(1.f,fmaxf(0.f,0.5f-d));
            if(cov>0.f) plot(x,y,c[0],c[1],c[2],cov*a0);
        }
        if(lit){
            /* tight glow, still inside the band */
            const float sig=1.1f, inv=1.f/(2.f*sig*sig);
            for(int y=Y0;y<=Y1;++y) for(int x=x0-1;x<=x1+1;++x){
                const float d=hypotf(x+0.5f-cx,y+0.5f-CY)-rad;
                if(d<=0.f) continue;
                plot(x,y,c[0],c[1],c[2], 0.35f*expf(-d*d*inv));
            }
        }
    }
    FILE *o=fopen(argv[2],"wb"); if(!o){ perror(argv[2]); return 1; }
    fwrite(fr.data(),1,fr.size(),o); fclose(o);
    return 0;
}
