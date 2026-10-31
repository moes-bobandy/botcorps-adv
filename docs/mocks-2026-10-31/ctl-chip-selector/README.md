# CTL chip-selector mock (MOCK ONLY, not in firmware, not approved yet)

Feedback addressed: "No remove those box's you just modified" / "The chip alone is the selector".

- All 10 value rectangles are gone (THEME, FOCUS, MIX, RAIN COL, MASHUP, RAIN, RAIN MODE, MULTI LED+value, ORIENT PORT/FLIP, BG MOTION).
  Their area is filled with board black, and any straight trace that crosses the area is continued.
- The chip's own art is the selector. A tap cycles the choice and repaints the chip face with a distinct picture.
- UI-changing chips (THEME, ORIENT, RAIN, RAIN COL, RAIN MODE, MASHUP, BG MOTION) are full brightness.
  Non-UI chips (MIX, MULTI, FOCUS) have their art dimmed to 60%. No white border anywhere.
- Variant A tints the plate inside the chip with the theme accent at 30%. Variant B uses brightness only.

| chip | choices | own art per choice |
|---|---|---|
| THEME | 12 themes | CONTROLbot face re-inks (shipping). The THEME 12-segment meter lights segment N in the theme colour. |
| ORIENT | PORT/FLIP | Slide switch: the cyan knob sits left (PORT, phone notch up) or right (FLIP, notch down). |
| RAIN | ON/OFF | Green rain streaks / red X over a flat puddle line. |
| BG STYLE (steer A) | RAIN/BOTS/GIBSON/FLYBY/BOUNCE | Streaks / 3 bots / perspective grid / bot with speed trails / ball with dotted arc. |
| RAIN COL | GREEN/CYAN/THEME | The CP die's bar is green / cyan / theme accent. |
| BG COLOR (steer A) | MULTI/GREEN/CYAN/PURPLE/AMBER | CP bar: 4 stripes / green / cyan / purple / amber. |
| RAIN MODE | DRIFT/SWEEP/STILL | The yellow connector becomes a 3-detent slide. The knob sits at position 1, 2 or 3 and shows a diagonal, vertical or dot mark. |
| MASHUP | SLOW/MED/FAST | Green bot with 1 / 2 / 3 chevrons, plus speed lines. |
| BG MOTION | ON/OFF | The label is the art: bright with a cyan wave underline / grey with a flat line. |
| MIX | ALL/WIFI/BLE | WiFi arcs plus BT rune / WiFi arcs / BT rune (dimmed). |
| MULTI | OFF/30S/60S | Dial: slashed / half filled / full (dimmed). |
| FOCUS | ALL/WIFI/BLE/ALERTS | The 12-segment meter: all lit / cyan group / blue group / red group (dimmed). |

Plate verdict: on chips that have a die frame (RAIN, MASHUP, CP, the RAIN MODE slide), the tint reads as the die lighting up.
On BG MOTION, which has no chip body, and on the empty half of the ORIENT track, the plate clearly reads as a box.
Recommendation: B (brightness only).

Flags:
- THEME, FOCUS, BG MOTION and RAIN MODE have no IC chip body in the locked art. Their "chip" is the label/meter or the connector part.
- Firmware would need new overlay masks over locked chip-face art. That adds overlays; the locked frame stays unchanged underneath.
- The "MULTI" label lower row was already clipped by the old LED rect in the locked art.

Tap audit: see tap_audit.txt. 0 overlapping pixels; 0 dead or ambiguous pixels across 22012 chip-art/body pixels.
