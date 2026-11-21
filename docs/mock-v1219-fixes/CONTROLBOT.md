# CONTROLbot investigation (no code change)

v1.2.12 fb46c13105bc0736c79137e0087a29c2eea72a68 added RECAL TOUCH / ROTATE / EXIT
(LNCHR or OFF) and relabelled SCAN to RADAR. Between fb46c13 and bd6c320 the
CONTROLbot sources never changed:

- lv_controlbot_panel.h  blob 3a67ac48775cbced4c06bf9c0e54b614bd1d9b6f
- lv_controlbot_screen.h blob c15ceb598f389fac23fdeea54d75543b97b079fc

Same blob at 9efd705, 89aef6d, cda0390, dc98ba3, 514df65, 0513879, c70ef04,
e8f6c91, bd6c320, 61158ee. No regression commit exists.

Host sim of bd6c320 (page 3 / go 16) still shows RADAR RAIN, RECAL TOUCH,
ROTATE PORT, EXIT OFF, SLEEP, BAR HAZRD, BRIGHT.
Proof: /workspace/out/fw-bigbot-1117/v1219-fixes/controlbot-bd6c320/
controlbot_v1218@3x.png sha256 9a9cf8c33c88de80... (file SHA256SUMS in that tree).
