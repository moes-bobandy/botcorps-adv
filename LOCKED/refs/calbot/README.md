# CALbot touch-cal references (locked 2026-10-30)

Source: the user's chat attachments, copied byte-for-byte; file names are the sha256 of the content.
All are 720x960 (3x nearest-neighbour of the 240x320 CYD portrait screen); sample [1::3,1::3] for the exact 1x frame.

| file | sha256 | state / use |
|---|---|---|
| 229cd34455f4d157af647360fe380a2e3ca959c83e2d66a7edd33500c1d9f5b2.gif | 229cd34455f4d157af647360fe380a2e3ca959c83e2d66a7edd33500c1d9f5b2 | LIME full flow (primary). 100 ms/frame: f0 cal 1/4 TOP LEFT, f8-15 first TL attempt released early, f24-39 TL hold+lock, f48 cal 2/4, f49-64 TR hold, f73 cal 3/4, f74-89 BR hold, f98 cal 4/4, f99-114 BL hold, f122 CAL OK, f127+ BOOTbot |
| b0a422d0eecc368ae2dda7e2b025b4190d38d8ab9b71deb0a72e50849d8b8f81.png | b0a422d0eecc368ae2dda7e2b025b4190d38d8ab9b71deb0a72e50849d8b8f81 | APPROVED CAL OK end frame ("CALbot", lime, gold pins). Identical to GIF f122 except the chip label |
| e3a3af780dc5e1fd60182a158ae35390f3a8469a7f17e7805028c0746fd734ea.png | e3a3af780dc5e1fd60182a158ae35390f3a8469a7f17e7805028c0746fd734ea | CAL OK with "CALBOT" caps label - NOT approved wording, kept for history |
| a9baac0bef6e0a63d395554e9546db5681f20641ece901910eef5718f52a7690.gif | a9baac0bef6e0a63d395554e9546db5681f20641ece901910eef5718f52a7690 | PURPLE "CONTROLbot" flow - superseded by lime; layout/timing cross-check only |
| 29212e1ed82e488c5190528a478f4ca2697a3b0d009da15d229299a258ee317a.png | 29212e1ed82e488c5190528a478f4ca2697a3b0d009da15d229299a258ee317a | PURPLE "CONTROLbot" OK frame - superseded by lime |

Notes
- Duplicate attachments not copied: 01M66N8Q5XTG6BZZ9Y8P24X3B3.gif (= a9baac0b...), 01M66N909MBHMS9RCFR62T5BQ5.png (= 29212e1e...). FLIPPERbot files excluded (not CALbot).
- The lime GIF draws the chip label as "CALBOT"; firmware art (tools/livescan/make_calbot_art.py) patches x121-130,y169-173 of every frame with the b0a422d0 "CALbot" pixels. No other change.
- Every state the firmware needs exists in lime, so no purple state was recoloured.
- Rules: centre chip lime + "CALbot" (never CALBOT / CONTROLbot); pins gold; power-on order CALbot -> BOOTbot -> home; 1.5 s hold per corner, clockwise TL, TR, BR, BL.
