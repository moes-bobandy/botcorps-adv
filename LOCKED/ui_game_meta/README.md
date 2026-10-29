LOCKED. METAbot game mock, locked 2026-10-29 4:19 PM ET under Mobandy's authority (relayed 2026-10-29 3:48 PM ET):
"just build the fucking thing don't wait for me for even fucking locking anything in"
— once a game passes all checks, lock it.

File: meta@3x.mp4  sha256 ec0c4a4b6ca4024e375a4b16add08ae035d4e26fdbf93235b0dd7e0b082d6c59
Built from the LOCKED AIR TAG v3 game (LOCKED/refs/game_airtag_v3@3x.mp4, 792bb757...) by repainting only the per-device regions
(field targets, chip icon, device rain icons, silk line 3, chip face colour + label, terminal nouns/verbs). Everything else is AIR TAG v3 pixel-for-pixel before encode.
Control: B (KEYFOB v2 SHOCK, time-mapped). Icons: WATCH gl_icon (GL_NEW[3], ink de24ff/080808/ffffff) in 3 field targets + chip; WATCH 8x8 rain (DEV_ART[6], DEV_NEON F95A/4067/FA3B).
Encode: libx264 preset slow crf 16 yuv420p +faststart, 720x960, 100/3 fps, 739 frames.
stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
Checks (full log + zooms + side-by-side in LOCKED/candidates/meta/):
  frames 739 739
  native px changed (>24) outside mask: 711  outside mask+1px: 699  max diff outside mask+1px: 43
  mask native px: 13367  bbox rows [ 56 303] cols [ 33 206]
  residual AIR TAG colours inside per-device mask (candidate): {'5a5510': 0, '524518': 0, '5a654a': 366, 'ffff00': 0, 'ffba4a': 0, '312c10': 314514}
  leftover tracker rain icons: 0 []
  yellow tag px in field (y33-117,x40-205): 0  in chip icon box: 0
  explanations:
    outside-mask diffs are x264 generation loss only (pre-encode: 0 changed px outside mask, asserted per frame); baseline re-encode of untouched AIR TAG v3 = 2397 px >24, max 46.
    '312c10' hits inside mask = untouched binary rain digits (mask is the union over all frames).
    '5a654a' 366 hits: all inside the control-B rect (x35-102, y186-226); they are pixels of the LOCKED KEYFOB v2 SHOCK control (kf palette), not AIR TAG residue.
  control B: rect x35-102,y186-226 copied pixel-exact from LOCKED/refs/game_keyfob_v2@3x.mp4 (native), time-mapped so its events land on AIR TAG's: kf=f (f<150), f-48 (<340), f-64 (<520), f-80 (else); zap AT260<-KF212, purge AT379<-KF315, blur AT577<-KF497, end AT738<-KF658; switch points sit on identical KF frames (no jump).
  zoom: mainchip/devchip/topchip @f250,f265 + controlB @f265,f560 reviewed: full inner pin ring + outer pins, theme-coloured, no stray pixels.
  ffprobe: stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
  atom order: type:'moov' parent:'root' sz: 9293 40 1109009
  type:'moov' type:'mdat' 
