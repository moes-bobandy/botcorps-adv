LOCKED. DEAUTHbot game mock, locked 2026-10-29 4:11 PM ET under Mobandy's authority (relayed 2026-10-29 3:48 PM ET):
"just build the fucking thing don't wait for me for even fucking locking anything in"
— once a game passes all checks, lock it.

File: deauth@3x.mp4  sha256 4926d7dfd3aa7bd88ae4f8b662c0dbf344943c9d89e03f7fe79588cdfc750520
Built from the LOCKED AIR TAG v3 game (LOCKED/refs/game_airtag_v3@3x.mp4, 792bb757...) by repainting only the per-device regions
(field targets, chip icon, device rain icons, silk line 3, chip face colour + label, terminal nouns/verbs). Everything else is AIR TAG v3 pixel-for-pixel before encode.
Control: C (dial + FIRE, unchanged from AIR TAG v3). Icons: WI-FI AP gl_icon (DEV_AP 12x12, inks 00ffff/103431) in field+chip; AP 8x8 rain (DEV_NEON 079F/0208/1F9F); silk WI-FI AP U2; kick/flood.
Encode: libx264 preset slow crf 16 yuv420p +faststart, 720x960, 100/3 fps, 739 frames.
stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
Checks (full log + zooms + side-by-side in LOCKED/candidates/deauth/):
  pre-encode: every changed pixel lies inside the per-device mask (asserted on all 739 frames; 0 outside)
  frames 739 739
  native px changed (>24) outside mask: 1052  outside mask+1px: 1035  max diff outside mask+1px: 36
  mask native px: 11733  bbox rows [ 56 303] cols [ 33 206]
  residual AIR TAG colours inside per-device mask (candidate): {'5a5510': 0, '524518': 0, '5a654a': 0, 'ffff00': 0, 'ffba4a': 0, '312c10': 313027}
  leftover tracker rain icons: 0 []
  yellow tag px in field (y33-117,x40-205): 0  in chip icon box: 0
  post-encode outside-mask residue is x264 generation noise: re-encoding the untouched AIR TAG frames with identical settings gives 2397 px >24, max 46 vs ref
  '312c10' hits inside mask = untouched binary 0/1 rain digits that share cells (union mask over all frames)
  zoom-check: zoom4x_mainchip/devchip/topchip f250+f265 reviewed: full inner+outer pin rings, no stray pixels, no red rings
  ffprobe: stream|codec_name=h264|profile=High|width=720|height=960|pix_fmt=yuv420p|r_frame_rate=100/3|nb_frames=739
  atom order: type:'moov' parent:'root' sz: 9293 40 1055111
  type:'moov' type:'mdat' 
