LOCKED. FLOCKbot game mock, locked 2026-10-29 4:49 PM ET under Mobandy's authority (relayed 2026-10-29 3:48 PM ET):
"just build the fucking thing don't wait for me for even fucking locking anything in"
— once a game passes all checks, lock it.

File: flock@3x.mp4  sha256 77fd8fa6cae0b15cbd50549faaf7aa521f38a25e08b072d09ddc140d5876fc47
Built from the LOCKED AIR TAG v3 game (LOCKED/refs/game_airtag_v3@3x.mp4, 792bb757...) by repainting only the per-device regions
(field targets, chip icon, device rain icons, silk line 3, chip face colour + label, terminal nouns/verbs). Everything else is AIR TAG v3 pixel-for-pixel before encode.
Control: A (ZAP/EMP + 9-cell meter, A_shock gif cff6033e). Icons: camera (GL_CAMERA) field x3, chip icon, camera rain.
Encode: libx264 preset slow crf 16 yuv420p +faststart, 720x960, 100/3 fps, 739 frames.
stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
Checks (full log + zooms + side-by-side in LOCKED/candidates/flock/):
  frames 739 739
  native px changed (>24) outside mask: 384  outside mask+1px: 384  max diff outside mask+1px: 45
  mask native px: 14205  bbox rows [ 57 303] cols [ 33 206]
  residual AIR TAG colours inside per-device mask (candidate): {'5a5510': 669, '524518': 119, '5a654a': 0, 'ffff00': 0, 'ffba4a': 0, '312c10': 314838}
  leftover tracker rain icons: 0 []
  yellow tag px in field (y33-117,x40-205): 0  in chip icon box: 0
  explanations:
    outside-mask diffs are x264 generation loss only (pre-encode: 0 changed px outside mask, asserted per frame in run.py); baseline re-encode of untouched AIR TAG v3 = 2397 px >24, max 46.
    '5a5510'/'524518' hits inside mask in the control-A rect (y184-226,x35-102) are codec blur of control A's own ZAP/EMP button edges: exact +-10 matches in that rect pre-encode = 0 (frames 0,190,250,260,265,300,330,379,400,500,578,600); the A gif itself has 0 pixels within +-10 of those colours.
  control A source: chat attachment /home/box/agent-data/agents/c18300f9-2b0f-42e0-a342-d3bd5c97abe7/attachments/cff6033e20e3c9aac1c05626cce4c62899b6910eb04e22673174e8e45e2e6e3c.gif (A_shock animation: ZAP + EMP chip buttons + 9-cell meter, 22 frames) + stills sheet b56df02e7cf08beb038f4bfaf5a10c00436b53493120747bd2f26bbb2c2f9dd0.png ('A idle' / 'A fire'). Not in git history or firmware source.
  control A placement: rect y184-226 x35-102 replaces AIR TAG's control C rect; gif rows shifted +2 px (gif layout is 2 px higher than the game clips; measured on C gif af9d vs AIR TAG and B gif 4acb vs KEYFOB). Timing: gif frame A11 (fire) anchored to AIR TAG's C fire frames 260/379/578; gif ms timing at 30 ms per video frame (t=2210+(f-E)*30); A0 idle elsewhere.
  zoom: mainchip/devchip/topchip @f250,f265, control A @f330 (EMP pressed), f260 (ZAP fire, meter full), f500 (idle), field targets @f100 reviewed: full inner pin ring + outer pins, theme-coloured, real gl_icon art, no red rings, no stray native pixels.
    '312c10' hits = untouched binary rain digits (mask is the union over all frames).
  device: BH_DEV flock = GL_CAMERA. Chip icon + field = DEV_ROW[DEV_CAM], GL_INK {167,64,255} = b524ff/080808/ffffff; rain = DEV_ART camera + DEV_NEON (B13F,3048,BA1F); silk 'CAMERA U2'; face BOT_PAL 185 = b5db52; terminal SIG/SUB[4]: camera / blind / plate.
  ffprobe: stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
  atom order: type:'moov' parent:'root' sz: 9285 40 1022995
  type:'moov' type:'mdat' 
