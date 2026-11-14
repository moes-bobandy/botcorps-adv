## What changed in v5f (Mobandy 3:24 / 3:48 PM ET)

**Folder squares redesigned as digital instruments.** The HACKbot-shaped Morpheus scroll frame is kept: tab, opaque double outline, and the search-then-drop order. Every element maps to a real value:

| Scroll | Element | Value it shows |
|---|---|---|
| All | Header: device emblem + real folder name | Unchanged |
| All | Fact line | MO_FOLD_ACT |
| SCROLL 1 (what it is) | 7-segment readout | The device's dBm (EX.DATA rssi) |
| SCROLL 1 | 10-LED signal meter | Lit to the Gibson level of that dBm: (rssi + 100, 4..60) x 10 / 60 |
| SCROLL 1 | Threat: warning triangle + 3 segments | HIGH = 3, MEDIUM = 2, LOW = 1, in the threat colour |
| SCROLL 2 (how spotted) | Lit RADIO badge | MO_FOLD_TAG radio: WIFI / BLE / WI+BT |
| SCROLL 2 | Lit band badge | From the device's GHZ item (e.g. 2.4G) |
| SCROLL 2 | Up to 2 numbers as 7-segment digits, labelled | The device's numeric items (e.g. DEAUTH SUBTYPE 12, DISASOC 10; FLOCK OUI 49) |
| SCROLL 3 (protect) | Shield-check icon + DO THIS | Marks the header line as the action to take |

**Removed:** the progress bar, LOADED / %, the landing light sweep and the idle glow cycle. None of them showed data.

**Rain on the 5 black towers is brighter.** It is the same lv_radar.h code-streak law (x, length, speed, head), now with:
- denser streams (face width / 2);
- a continuous trail instead of every 2nd pixel;
- a bright 0xd8ffe6 head;
- trail 0x10d858 then 0x00a83c, full strength for the first half, then fading.

The rain is still drawn behind the file rows and the emblem.

**Unchanged from v5e:** idle placement, solid wood, attached emblems (fixed size, inset), and the left tower.

## What changed in v5e (Mobandy 3:23 - 3:25 PM ET)

**Withdrawn.** The v5d open screen was rejected because the HACKbot UI sat on top of the Morpheus screen and the stat bars and strips meant nothing. All of it is gone: the Gibson lock brackets, the HACKbot key bar, the 11-band meter, the folder chips, the meters and the load bars.

**Open screen.** This is v5c's Morpheus open state again, code for code:
- the camera push;
- the charcoal search bar hung from the tower by timber posts;
- three green scrolls on DOWNLOAD's eased stagger, with the index line.

**Search step.** Only the file-search step changed. The scrolls now drop 200 ms after the search bar has finished typing the device name, so the search comes first and the files follow. In v5c they started at 300 ms while the name was still typing.
- Every element of the search bar is real: the typed text is the device name, and "n/3 SCROLLS" counts the scrolls that have landed.
- Gate, from the sim's own state: %(sr)s.

**Solid wood.** The 13 wood towers are opaque again: front grain, sides and cap are drawn at full alpha.

**Rain on the 5 black Matrix towers.** The towers are DEAUTH, HACK, SKIMMER, FLIPPER and EVIL TWIN.
- The old dim glyph or 1 px code inside the glass is replaced by the CYD code-streak rain from `lv_radar.h`, the radar field's "code streaks" (`lv_rd_bg` case 0, RAIN), with the same law:

  | part | value |
  |---|---|
  | x | (s*53+17) mod width |
  | length | 10+(s*7) mod 18 |
  | speed | 40+(s*29) mod 70 px/s |
  | head | (t*speed + s*37) mod (height+length) |
  | pixels lit | every 2nd |
  | head colour | 0x10d858 |
  | tail colour | 0x00a83c |
  | fade | 8/8 at the head, then 6-k*5/len |

- It is mapped into each tower's own face: 18 streams in the close-up, width/3 when idle, offset per tower.
- It is drawn before the file rows and the emblem, so both stay opaque on top.

**Emblems bolted to the face.** Each tower now has one fixed idle emblem size: its narrowest drifting face width over 320 real-sim samples minus 3, so there is at least 1 px inset on each side (`src/emb_fix_v5e.json`).
- The emblem is centred on its own face with the same transform, so it never changes size or jumps while idle. It switches to 40 px only in the open close-up.
- Gate over 320 samples:
  - %(att_v)s rects fall outside their face;
  - centre-to-face-centre x offset varies by at most %(att_dx)s px;
  - top offset varies by at most %(att_dy)s px.

**Unchanged from v5d:** the idle room (shorter near towers, Gibson depth, independent drift, colours, 5 black and 13 wood), the camera, and tw 1.35.

### v5d height factors (kept)

The shortest tower (%(stump_name)s) is %(stump_min)s%% of the tallest on-screen tower. The minimum is %(rows_min)s idle folder rows.

| tower (near -> far) | Z | hk | H | median on-screen px | idle folder rows |
|---|---|---|---|---|---|
%(hk_table)s

The black towers are %(black)s.
