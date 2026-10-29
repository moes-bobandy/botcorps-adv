import sys; sys.path.insert(0,'/workspace/cydport/tools'); from lkenc import Enc
L='/workspace/cyd-lock-store/LOCKED/'
CTL='/home/box/agent-data/agents/c18300f9-2b0f-42e0-a342-d3bd5c97abe7/attachments/8cc9946c6003d1ebc97ae20980f9edb2874d4ffe6065c9e6b24b625cb2df952d.mp4'
TH6=lambda pre,ranges:[(a,b,1,'%s_t%d'%(pre,i)) for i,(a,b) in enumerate(ranges)]
CLIPS=[
 ('boot',L+'ui_bootbot/bootbot_bg_v2@3x.mp4',[(0,84,0,'boot')]),
 ('home',L+'ui_gibsonbot/gibsonbot_bg@3x.mp4',[(6,90,0,'home_intro')]+TH6('home',[(420,479),(120,179),(180,239),(240,299),(300,359),(360,419)])),
 ('term',L+'ui_terminalbot/terminalbot_bg@3x.mp4',TH6('term',[(0,59),(120,179),(180,239),(240,299),(300,359),(360,419)])),
 ('scan',L+'ui_scanbot/scanbot_bg_themes_v2@3x.mp4',TH6('scan',[(0,49),(50,99),(100,149),(150,199),(200,249),(250,299)])),
 ('ctl',CTL,[(0,44,1,'ctl')]),
 ('gib',L+'refs/game_gibson_v8@3x.mp4',[(0,629,0,'gib')]),
 ('air',L+'refs/game_airtag_v3@3x.mp4',[(0,738,0,'air')]),
 ('key',L+'refs/game_keyfob_v2@3x.mp4',[(0,658,0,'key')]),
]
import os
only=sys.argv[1:]  # unused
e=Enc(); meta={}
for n,p,s in CLIPS:
    b0=len(e.blob); pal=e.clip(n,p,s); meta[n]={'src':p,'segs':s,'bytes':len(e.blob)-b0,'npal':len(pal)}
    print(n,'pal',len(pal),'bytes',len(e.blob)-b0,flush=True)
e.write('/workspace/cyd-portrait-wt/src/lock/lock_data.h','/workspace/cyd-portrait-wt/src/lock/lock_blob.bin',meta)
print('TOTAL',len(e.blob),'chunks',len(e.chunks),'ops',len(e.ops))
