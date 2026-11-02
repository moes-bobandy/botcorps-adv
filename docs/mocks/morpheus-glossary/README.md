# MORPHEUSbot GLOSSARY mock (MOCK ONLY)

Educational device glossary for the CYD portrait (240x320, frames at 3x = 720x960). No firmware, lock, or flash changes.
Composited in Python from the home portrait board reference frames (`g200` green, `g380` orange), so the top bar, side pin labels, and board edge are the real reference pixels.
Each device chip is the home CONTROLbot crew chip (meander header, grey body, themed dash pins, black face) with its face cleared, the device's firmware icon centred, and a tiny-font `NAME` / `bot` label.

## Files
- `grid_green_3x.png`, `grid_orange_3x.png`: 3x4 grid, `GLOSSARY` title (GLCD 5x7)
- `overlay/grid_*_tapzones_3x.png`: tap zones drawn over the chips and the crown BACK
- `desc/desc_<device>_<theme>_3x.png`: all 12 description pages in both themes
- `overlay/desc_{wifi,skimmer}_*_backzone_3x.png`: the BACK tap zone on a description page
- `contact_sheet_desc_{green,orange}.png`: all 12 description pages
- `tapthrough_green.mp4`: grid, tap chip, page, tap BACK, grid, for all 12 (10 fps, 44.4 s)
- `tap_rects.json`; `tool/glossary.py`, `tool/sheet_video.py`, `tool/glossary_text.json`

## Tap rects (1x, x y w h)
| target | rect |
|---|---|
| WIFI chip | 42 40 44 61 |
| BLUETOOTH chip | 98 40 44 61 |
| DEAUTH chip | 154 40 44 61 |
| HACK chip | 42 110 44 61 |
| FLOCK chip | 98 110 44 61 |
| DRONE chip | 154 110 44 61 |
| AIR TAG chip | 42 180 44 61 |
| KEYFOB chip | 98 180 44 61 |
| META chip | 154 180 44 61 |
| SKIMMER chip | 42 250 44 61 |
| FLIPPER chip | 98 250 44 61 |
| SAMSUNG chip | 154 250 44 61 |
| BACK (crown plate, top bar; `src/ui/botcorps_cyd_edge.inc:184` crown = BACK) | 133 3 27 14 |

Grid: columns x = 42 / 98 / 154, rows y = 40 / 110 / 180 / 250, chip 44x61 including pins. The order follows the BOTS table (`src/ui/botcorps_ui.cpp` ~190).

## Icons
- WIFI: `LV_IC_AP` (`src/live/lv_icons.h`) in clear orange. BLUETOOTH: `LV_IC_BLE` in clear blue.
- DEAUTH: `DEV_DEAUTH` 12x12 with its firmware `DEV_COL` (`src/ui/botcorps_ui.cpp` ~3122). Firmware `LV_HUNT_ICON` maps DEAUTH to AP, which would duplicate WIFI.
- The others follow `LV_HUNT_ICON` (`src/live/lv_core.h`): HACK LAPTOP, FLOCK CCTV, DRONE DRONE, AIR TAG AIRTAG, KEYFOB KEYFOB, META GLASSES, SKIMMER SKIMMER, FLIPPER FLIPPER, SAMSUNG GALAXYTAG.

## Text sources (all pages are marked DRAFT TEXT)
The only device descriptions in the repo are the one-line `LV_HUNT_DESC` strings (`src/live/lv_core.h:302`). Each 'how it is spotted' line is reworded from them. Every 'what it is' and 'how to protect yourself' line is new draft text. WIFI and BLUETOOTH are fully draft. All text is defensive and educational only.

| device | what it is | how it is spotted | how to protect yourself | source |
|---|---|---|---|---|
| WIFI | Routers and hotspots near you. | Networks broadcast their name and lock type. | Use WPA2/WPA3. Skip open Wi-Fi or use a VPN. | draft (no firmware description for WIFIbot) |
| BLUETOOTH | Short-range radio in earbuds, watches and tags. | Devices send adverts with a name and maker ID. | Turn it off when idle. Refuse surprise pairing. | draft (no firmware description for BLUETOOTHbot) |
| DEAUTH | Fake messages that kick you off Wi-Fi. | Passive watch for deauth and disassoc floods. | Use WPA3 or protected frames (802.11w). | spot: LV_HUNT_DESC 'passive deauth/disassoc frame watch' (src/live/lv_core.h:302); rest draft |
| HACK | Weak Wi-Fi anyone can read. | Access points that are OPEN or use WEP. | Stay off them. Lock yours with WPA2/WPA3. | spot: LV_HUNT_DESC 'WiFi: OPEN or WEP access points' (src/live/lv_core.h:302); rest draft |
| FLOCK | Flock cameras that read license plates. | Flock Wi-Fi names, Penguin/FS Ext Battery. | Know where they are. Ask how long data is kept. | spot: LV_HUNT_DESC 'WiFi+BLE: Flock SSIDs, Penguin/FS Ext Battery' (src/live/lv_core.h:302); rest draft |
| DRONE | DJI, Tello, Skydio and Anafi drones. | Drone Wi-Fi names, Bluetooth and Remote ID. | Note time and place. Report misuse to police. | spot: LV_HUNT_DESC 'WiFi+BLE: DJI/Tello/Skydio/Anafi, Remote ID' (src/live/lv_core.h:302); rest draft |
| AIR TAG | Apple AirTag, a coin-size Find My tracker. | Find My adverts (Apple 0x004C, type 0x12). | Check bags and car. Heed phone tag alerts. | spot: LV_HUNT_DESC 'BLE: Apple Find My adverts (0x004C type 0x12)' (src/live/lv_core.h:302); rest draft |
| KEYFOB | Keyring trackers like Tile. | Tile Bluetooth IDs FEED and FEEC. | Scan for tags you do not own. | spot: LV_HUNT_DESC 'BLE: Tile trackers (FEED/FEEC). No sub-GHz radio' (src/live/lv_core.h:302); rest draft |
| META | Meta smart glasses with a camera. | Bluetooth company ID 0x01AB or 0x058E. | Watch for the capture light. Ask before filming. | spot: LV_HUNT_DESC 'BLE: Meta glasses (company 0x01AB/0x058E)' (src/live/lv_core.h:302); rest draft |
| SKIMMER | Hidden card readers on pumps and ATMs. | Serial modules HC-05/06/08 and RNBT. | Tap to pay. Wiggle the reader. Cover the keypad. | spot: LV_HUNT_DESC 'BLE: HC-05/06/08, RNBT serial modules' (src/live/lv_core.h:302); rest draft |
| FLIPPER | Flipper Zero, a pocket radio and NFC tool. | Bluetooth advert names that say Flipper. | Update your phone. Dismiss surprise pop-ups. | spot: LV_HUNT_DESC 'BLE: Flipper Zero advert names' (src/live/lv_core.h:302); rest draft |
| SAMSUNG | Samsung Galaxy SmartTag trackers. | SmartTag Bluetooth adverts (FD5A/FD59). | Run an unknown tag scan in SmartThings. | spot: LV_HUNT_DESC 'BLE: Samsung SmartTag (FD5A/FD59)' (src/live/lv_core.h:302); rest draft |

## Open questions
- Where MORPHEUSbot sits on this screen (no chip on the grid, as asked), and how the glossary is opened (for example, tapping MORPHEUSbot on home).
- KEYFOB really detects Tile trackers (firmware: 'No sub-GHz radio'). Should the name or the text change?
- Should DEAUTH use the `DEV_DEAUTH` art in firmware too?
- BACK is the existing top-bar crown. Its tap rect is small (27x14), so it may need padding.
- Each description is 5 to 6 lines (three sections of at most 2 lines each) rather than 2 to 4 lines.

Regenerate: `python3 tool/glossary.py [outdir] && python3 tool/sheet_video.py` (needs the reference frames in `/workspace/out/morpheus-rename/frames` and mlib.py).
