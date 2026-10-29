LOCKED. HACKbot game mock, locked 2026-10-29 4:49 PM ET under Mobandy's authority (relayed 2026-10-29 3:48 PM ET):
"just build the fucking thing don't wait for me for even fucking locking anything in"
— once a game passes all checks, lock it.

File: hack@3x.mp4  sha256 96688439edcd1bd37d0a7418755499b763aac9fcee98e85d800e8967c8eac33a
Built from the LOCKED AIR TAG v3 game (LOCKED/refs/game_airtag_v3@3x.mp4, 792bb757...) by repainting only the per-device regions
(field targets, chip icon, device rain icons, silk line 3, chip face colour + label, terminal nouns/verbs). Everything else is AIR TAG v3 pixel-for-pixel before encode.
Control: A (ZAP/EMP + 9-cell meter, A_shock gif cff6033e). Icons: laptop (GL_LAPTOP) field x3, chip icon, laptop rain.
Encode: libx264 preset slow crf 16 yuv420p +faststart, 720x960, 100/3 fps, 739 frames.
stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
Checks (full log + zooms + side-by-side in LOCKED/candidates/hack/):
  frames 739 739
  native px changed (>24) outside mask: 548  outside mask+1px: 548  max diff outside mask+1px: 43
  mask native px: 14173  bbox rows [ 57 303] cols [ 33 206]
  residual AIR TAG colours inside per-device mask (candidate): {'5a5510': 790, '524518': 59, '5a654a': 292, 'ffff00': 0, 'ffba4a': 0, '312c10': 425114}
  leftover tracker rain icons: 0 []
  yellow tag px in field (y33-117,x40-205): 0  in chip icon box: 0
  explanations:
    outside-mask diffs are x264 generation loss only (pre-encode: 0 changed px outside mask, asserted per frame in run.py); baseline re-encode of untouched AIR TAG v3 = 2397 px >24, max 46.
    '5a5510'/'524518' hits inside mask in the control-A rect (y184-226,x35-102) are codec blur of control A's own ZAP/EMP button edges: exact +-10 matches in that rect pre-encode = 0 (frames 0,190,250,260,265,300,330,379,400,500,578,600); the A gif itself has 0 pixels within +-10 of those colours.
  control A source: chat attachment /home/box/agent-data/agents/c18300f9-2b0f-42e0-a342-d3bd5c97abe7/attachments/cff6033e20e3c9aac1c05626cce4c62899b6910eb04e22673174e8e45e2e6e3c.gif (A_shock animation: ZAP + EMP chip buttons + 9-cell meter, 22 frames) + stills sheet b56df02e7cf08beb038f4bfaf5a10c00436b53493120747bd2f26bbb2c2f9dd0.png ('A idle' / 'A fire'). Not in git history or firmware source.
  control A placement: rect y184-226 x35-102 replaces AIR TAG's control C rect; gif rows shifted +2 px (gif layout is 2 px higher than the game clips; measured on C gif af9d vs AIR TAG and B gif 4acb vs KEYFOB). Timing: gif frame A11 (fire) anchored to AIR TAG's C fire frames 260/379/578; gif ms timing at 30 ms per video frame (t=2210+(f-E)*30); A0 idle elsewhere.
  zoom: mainchip/devchip/topchip @f250,f265, control A @f330 (EMP pressed), f260 (ZAP fire, meter full), f500 (idle), field targets @f100 reviewed: full inner pin ring + outer pins, theme-coloured, real gl_icon art, no red rings, no stray native pixels.
    '5a654a' (103) and 38 of the '5a5510' hits outside the control rect are LAPTOP rain cells at the 0.42 level (firmware DEV_NEON laptop CFE0 -> 4a6110 / 4a6539), within tolerance only after encode; exact +-10 matches outside the control rect pre-encode = 0.
    '312c10' hits = LAPTOP gl_icon/rain shade (GL_INK laptop b = slot 44 = 312c10) + untouched binary rain digits.
  device: BH_DEV hack = GL_LAPTOP. Chip icon + field = GL_NEW[1] laptop, GL_INK {188,44,255} = b5ff00/312c10/ffffff; rain = DEV_ART laptop + DEV_NEON (CFE0,3A20,CFE3); silk 'LAPTOP U2'; face BOT_PAL 122 = 18d342; terminal p0t OWN/SIG/SUB[3]: laptop / trace / rst.
  ffprobe: stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
  atom order: type:'moov' parent:'root' sz: 9277 40 1010296
  type:'moov' type:'mdat' 
