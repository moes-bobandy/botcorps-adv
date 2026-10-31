import sys,os,subprocess; sys.path.insert(0,'.')
from sel import *
from PIL import ImageFont
F=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf',26)
os.makedirs('fr',exist_ok=True); [os.remove('fr/'+f) for f in os.listdir('fr')]
n=0; st={}; t=0
def frame(cap,tap=None,hold=6):
    global n
    for i in range(hold):
        s=Image.new('RGB',(1480,1020),(24,24,28)); d=ImageDraw.Draw(s)
        for j,pl in enumerate((False,True)):
            im=compose(t,st,pl).resize((720,960),Image.NEAREST); dd=ImageDraw.Draw(im)
            if tap and i<3:
                x,y=tap; dd.ellipse((x*3-18,y*3-18,x*3+18,y*3+18),outline=(255,255,255),width=3)
            s.paste(im,(20+j*740,56))
        d.text((20,12),('B brightness only'.ljust(40))+'   A tinted plate        '+cap,font=F,fill='white')
        s.save(f'fr/{n:04d}.png'); n+=1
frame('MOCK: chip = selector',hold=10)
seq=[('RAIN',(186,97),2),('RAINCOL',(52,158),3),('RMODE',(187,142),3),('MASHUP',(52,217),3),('MOTION',(190,242),2),('ORIENT',(127,234),2),('MIX',(52,105),3),('MULTI',(186,194),3),('FOCUS',(70,78),4)]
for k,tp,nc in seq:
    for c in range(1,nc+1):
        st[k]=c%nc; frame(f'tap {k} -> {CHOICES[k][st[k]]}',tp)
for c in range(1,13):
    t=c%12; frame(f'tap face -> THEME {TN[t]}',(120,140),hold=4)
subprocess.run(['ffmpeg','-y','-loglevel','error','-framerate','10','-i','fr/%04d.png','-c:v','libx264','-pix_fmt','yuv420p','-movflags','+faststart','/workspace/out/ctl-chip-selector/ctl_chip_selector.mp4'],check=True)
print(n)
