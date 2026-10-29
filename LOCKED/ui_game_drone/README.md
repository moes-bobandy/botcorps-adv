LOCKED. DRONEbot game mock, locked 2026-10-29 4:49 PM ET under Mobandy's authority (relayed 2026-10-29 3:48 PM ET):
"just build the fucking thing don't wait for me for even fucking locking anything in"
— once a game passes all checks, lock it.

File: drone@3x.mp4  sha256 5805d69e42f86321306b10f7c62bccc4cc3a1a0f020e17cce8cfda51b3310fea
Built from the LOCKED AIR TAG v3 game (LOCKED/refs/game_airtag_v3@3x.mp4, 792bb757...) by repainting only the per-device regions
(field targets, chip icon, device rain icons, silk line 3, chip face colour + label, terminal nouns/verbs). Everything else is AIR TAG v3 pixel-for-pixel before encode.
Control: A (ZAP/EMP + 9-cell meter, A_shock gif cff6033e). Icons: drone (GL_DRONE) field x3, chip icon, drone rain (DEV_EXTRA art).
Encode: libx264 preset slow crf 16 yuv420p +faststart, 720x960, 100/3 fps, 739 frames.
stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
Checks (full log + zooms + side-by-side in LOCKED/candidates/drone/):
  frames 739 739
  native px changed (>24) outside mask: 720  outside mask+1px: 720  max diff outside mask+1px: 43
  mask native px: 13999  bbox rows [ 56 303] cols [ 33 206]
  residual AIR TAG colours inside per-device mask (candidate): {'5a5510': 490, '524518': 52, '5a654a': 0, 'ffff00': 598892, 'ffba4a': 0, '312c10': 353642}
  leftover tracker rain icons: 0 []
  yellow tag px in field (y33-117,x40-205): 0  in chip icon box: 0
  explanations:
    outside-mask diffs are x264 generation loss only (pre-encode: 0 changed px outside mask, asserted per frame in run.py); baseline re-encode of untouched AIR TAG v3 = 2397 px >24, max 46.
    '5a5510'/'524518' hits inside mask in the control-A rect (y184-226,x35-102) are codec blur of control A's own ZAP/EMP button edges: exact +-10 matches in that rect pre-encode = 0 (frames 0,190,250,260,265,300,330,379,400,500,578,600); the A gif itself has 0 pixels within +-10 of those colours.
  control A source: chat attachment /home/box/agent-data/agents/c18300f9-2b0f-42e0-a342-d3bd5c97abe7/attachments/cff6033e20e3c9aac1c05626cce4c62899b6910eb04e22673174e8e45e2e6e3c.gif (A_shock animation: ZAP + EMP chip buttons + 9-cell meter, 22 frames) + stills sheet b56df02e7cf08beb038f4bfaf5a10c00436b53493120747bd2f26bbb2c2f9dd0.png ('A idle' / 'A fire'). Not in git history or firmware source.
  control A placement: rect y184-226 x35-102 replaces AIR TAG's control C rect; gif rows shifted +2 px (gif layout is 2 px higher than the game clips; measured on C gif af9d vs AIR TAG and B gif 4acb vs KEYFOB). Timing: gif frame A11 (fire) anchored to AIR TAG's C fire frames 260/379/578; gif ms timing at 30 ms per video frame (t=2210+(f-E)*30); A0 idle elsewhere.
  zoom: mainchip/devchip/topchip @f250,f265, control A @f330 (EMP pressed), f260 (ZAP fire, meter full), f500 (idle), field targets @f100 reviewed: full inner pin ring + outer pins, theme-coloured, real gl_icon art, no red rings, no stray native pixels.
    'ffff00' hits = DRONEbot's own face colour (BOT_PAL 252 = FFE0 = ffff00), not tracker art: yellow tag px in field = 0 and in chip icon box = 0, leftover tracker rain icons = 0.
    '312c10' hits = untouched binary rain digits.
  device: BH_DEV drone = GL_DRONE. Chip icon + field = DEV_ROW[DEV_DRONE] with gl_icon's 'c'->ink[0] rule, GL_INK {224,64,255} = ff0000/080808; silk 'DRONE U2'; face BOT_PAL 252 = ffff00; terminal SIG/SUB[5]: drone / ground / spoof.
  rain NOTE: firmware rain has no drone row (DEV_N=10; dev_only_icon uses kind % DEV_N, so GL_DRONE=10 would draw the WI-FI AP icon - a firmware bug to fix). Mock uses the firmware's own 8x8 drone art (DEV_EXTRA[0]) with a neon trio from the documented RAIN_DEV_NEON rule (a, a 72% to black, a 12% to white) on the GL_INK drone mock colour #FF2020: (F904,4041,F9C7).
  ffprobe: stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
  atom order: type:'moov' parent:'root' sz: 9309 40 1068743
  type:'moov' type:'mdat' 
