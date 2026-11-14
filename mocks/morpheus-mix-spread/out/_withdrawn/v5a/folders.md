# Morpheus folder content: 18 devices (mock, 2026-11-14)

Generated from `src/folders18.py`. Each device has three folders, SCROLL 1/2/3 = **ID / DETECT / ACTION**.
Each folder shows a name (up to 20 characters), a detail line (up to 24) and a right tag (up to 5).
The right tag replaces the old made-up "x.yKB" size: it is the threat level, the radio, and "DO".
Inside the tower, 8 scrolling line-items show KEY / VALUE (each up to 8 characters). The value replaces the made-up file size.

Sources: SquachWatch README "What it detects" table (`/workspace/tmp-squach/README.md`) and public knowledge, noted per device.

## 0. DEAUTH (threat HIGH, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | DEAUTH FLOOD | FAKE 802.11 KICK FRAMES | HIGH |
| DETECT | MGMT FRAME BURST | DEAUTH + DISASSOC RATE | WIFI |
| ACTION | USE WPA3 / PMF | RISK: KICK + KEY CAPTURE | DO |

Tower line-items: TYPE = ATTACK · FRAME = DEAUTH · SUBTYPE = 12 · DISASOC = 10 · SIGN = BURST · BAND = 2.4 GHZ · FIX = PMF · STD = 802.11W

Source: README: rate-detected burst, not a signature. 802.11 mgmt subtypes 12 deauth / 10 disassoc; 802.11w PMF protects them (mandatory in WPA3).

## 1. HACK (threat HIGH, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | WEAK WI-FI AP | OPEN OR WEP NETWORK | HIGH |
| DETECT | BEACON SAYS OPEN/WEP | SECURITY FIELD OF BEACON | WIFI |
| ACTION | STAY OFF / USE VPN | RISK: TRAFFIC READABLE | DO |

Tower line-items: TYPE = AP · LOCK = OPEN · OR = WEP · WEP = BROKEN · SNIFF = EASY · SAFE = WPA3 · ALSO OK = WPA2 · DO = VPN

Source: MO_DEV: OPEN or WEP access points. WEP is cryptographically broken; open Wi-Fi is unencrypted.

## 2. FLOCK (threat LOW, radio WI+BT)

| folder | name | detail | tag |
|---|---|---|---|
| ID | FLOCK SAFETY ALPR | PLATE CAMERA ON A POLE | LOW |
| DETECT | SSID FLOCK- / BLE ID | 49 OUI, CO ID 09C8 | WI+BT |
| ACTION | ASK RETENTION RULES | RISK: CAR MOVES LOGGED | DO |

Tower line-items: MAKER = FLOCK · TYPE = ALPR CAM · SSID = FLOCK- · BLE CO = 09C8 · BLE NAME = PENGUIN · OUIS = 49 · RISK = TRACKED · DO = ASK CITY

Source: README: 49 Wi-Fi OUI prefixes, flock- and bare Flock SSIDs, BLE names Flock/Penguin/Pigvision, company ID 0x09C8, Flock BLE service UUID.

## 3. DRONE (threat LOW, radio WI+BT)

| folder | name | detail | tag |
|---|---|---|---|
| ID | REMOTE ID DRONE | UAS BROADCASTING ITS ID | LOW |
| DETECT | RID: BLE FFFA, WIFI | ASTM F3411 MSG DECODED | WI+BT |
| ACTION | NOTE TIME + PLACE | RISK: AERIAL FILMING | DO |

Tower line-items: TYPE = UAS · STD = F3411 · BLE SVC = FFFA · WIFI = VEND IE · SENDS = SERIAL · ALSO = OP LOC · RISK = FILMING · DO = REPORT

Source: README: Remote ID in BLE service data 0xFFFA or a Wi-Fi beacon vendor element; ASTM F3411 decoded: position, altitude, serial, operator location.

## 4. AIR TAG (threat MEDIUM, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | APPLE AIRTAG | FIND MY BLE TRACKER | MED |
| DETECT | APPLE 004C, TYPE 12 | FIND MY OFFLINE ADVERT | BLE |
| ACTION | SCAN, FIND, DISABLE | RISK: STALKER TAG IN BAG | DO |

Tower line-items: MAKER = APPLE · NET = FIND MY · CO ID = 004C · TYPE = 12 · HAS = UWB+NFC · ALERTS = IOS+AND · RISK = STALKING · DO = PULL BAT

Source: README: company ID 0x004C + Find My payload check. Find My offline-finding advert type 0x12. AirTag has U1 UWB + NFC; iOS and Android have unknown-tracker alerts; the coin cell can be removed.

## 5. KEYFOB (threat MEDIUM, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | TILE KEYRING TAG | LIFE360 BLE TRACKER | MED |
| DETECT | BLE SVC FEED / FEEC | TILE SERVICE UUIDS | BLE |
| ACTION | TILE SCAN AND SECURE | RISK: FOLLOWS YOUR MOVES | DO |

Tower line-items: MAKER = TILE · OWNER = LIFE360 · SVC = FEED · SVC 2 = FEEC · RADIO = BLE · RISK = TRACKING · CHECK = TILE APP · DO = SCAN

Source: README: BLE service UUID 0xFEED / 0xFEEC. Tile is owned by Life360; the Tile app has Scan and Secure for unknown Tiles.

## 6. META (threat MEDIUM, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | RAY-BAN META GLASSES | CAMERA GLASSES, SNAP TOO | MED |
| DETECT | BLE SVC FD5F | META/LUXOTTICA/SNAP IDS | BLE |
| ACTION | WATCH CAPTURE LED | RISK: SILENT PHOTO/VIDEO | DO |

Tower line-items: MAKER = META · BRAND = RAY-BAN · ALSO = SNAP · SVC = FD5F · CAM = PHO+VID · LED = CAPTURE · RISK = FILMING · DO = ASK

Source: README: BLE service UUID 0xFD5F + Meta / Luxottica / Snap company IDs (Ray-Ban Meta, Snap Spectacles). Ray-Ban Meta shows a capture LED while recording.

## 7. SKIMMER (threat HIGH, radio BT)

| folder | name | detail | tag |
|---|---|---|---|
| ID | BT CARD SKIMMER | HIDDEN IN PUMPS + ATMS | HIGH |
| DETECT | HC-05/06, RN42 NAMES | SPP UUID 1101 + 3 OUI | BT |
| ACTION | TAP PAY, COVER PIN | RISK: CARD + PIN STOLEN | DO |

Tower line-items: TYPE = SKIMMER · MOD 1 = HC-05 · MOD 2 = HC-06 · MOD 3 = RN42 · MOD 4 = BT04-A · SPP = 1101 · RISK = CARD · DO = TAP PAY

Source: README: BLE/BT name match (HC-05/06/03, RN42, BT04-A) + SPP UUID 0x1101 + 3 OUI. Tap-to-pay avoids the mag stripe; cover the keypad.

## 8. FLIPPER (threat HIGH, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | FLIPPER ZERO | MULTI-RADIO HACK TOOL | HIGH |
| DETECT | BLE SVC 3081-3083 | CO ID 0E29, OUI 0CFA22 | BLE |
| ACTION | IGNORE POPUPS | RISK: BLE SPAM, RF CLONE | DO |

Tower line-items: RADIO = SUB-GHZ · ALSO = NFC+IR · RFID = 125KHZ · SVC = 3081 · CO ID = 0E29 · OUI = 0CFA22 · RISK = CLONING · DO = UPDATE

Source: README (HACKER type): Flipper service UUIDs 0x3081-0x3083, company ID 0x0E29, OUI 0C:FA:22. Flipper Zero: sub-GHz radio, NFC, 125 kHz RFID, infrared, BLE; BLE pop-up spam was patched in phone updates.

## 9. SAMSUNG (threat MEDIUM, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | GALAXY SMARTTAG | SAMSUNG BLE TRACKER | MED |
| DETECT | BLE SVC FD5A | SMARTTAG ADVERT UUID | BLE |
| ACTION | SMARTTHINGS TAG SCAN | RISK: GALAXY NET LOCATES | DO |

Tower line-items: MAKER = SAMSUNG · NET = ST FIND · SVC = FD5A · ALSO = FD59 · PLUS = UWB · RISK = TRACKING · APP = SMARTTH. · DO = SCAN

Source: README: BLE service UUID 0xFD5A (MO_DEV also FD59, both Samsung). SmartThings Find network; SmartTag+ adds UWB; SmartThings has an unknown tag search.

## 10. AXON (threat MEDIUM, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | AXON BODY CAMERA | POLICE CAMS, TASERS | MED |
| DETECT | SSID AB2- AB3- AB4- | AXON- SSID + 3 WIFI OUI | WIFI |
| ACTION | KNOW YOUR RIGHTS | RISK: YOU MAY BE FILMED | DO |

Tower line-items: MAKER = AXON · USER = POLICE · SSID = AB3- · ALSO = AB4- · OR = AXON- · OUIS = 3 · RISK = FILMED · DO = RIGHTS

Source: README: 3 Wi-Fi OUI + SSID prefixes AB2-/AB3-/AB4-/AXON-; Axon body cameras, TASERs, LE equipment.

## 11. RAVEN (threat LOW, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | RAVEN SHOT SENSOR | POLE AUDIO DETECTOR | LOW |
| DETECT | BLE SVC 3100-3500 | ANY UUID IN THAT RANGE | BLE |
| ACTION | ASK AUDIO POLICY | RISK: LISTENS FOR SOUNDS | DO |

Tower line-items: MAKER = FLOCK · TYPE = GUNSHOT · MOUNT = POLE · SVC LO = 3100 · SVC HI = 3500 · RADIO = BLE · RISK = AUDIO · DO = ASK

Source: README: any service UUID from 0x3100 to 0x3500; Raven gunshot detector (Flock Safety product).

## 12. ALPR (threat LOW, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | PLATE READER CAMERA | MOTOROLA OR GENETEC | LOW |
| DETECT | WIFI MAKER OUI | 7 OUI: 5 MOTO, 2 GENETEC | WIFI |
| ACTION | ASK WHO GETS DATA | RISK: TRIPS LOGGED | DO |

Tower line-items: TYPE = ALPR · MAKER = MOTOROLA · ALSO = GENETEC · OUIS = 7 · MOTO = 5 · GENETEC = 2 · RISK = LOGGED · DO = ASK

Source: README: 7 Wi-Fi OUI (5 Motorola Solutions, 2 Genetec).

## 13. CAMERA (threat MEDIUM, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | WI-FI IP CAMERA | WYZE TUYA VERKADA AXIS | MED |
| DETECT | CAMERA MAKER OUI | 17 KNOWN CAM OUI ON WIFI | WIFI |
| ACTION | CHECK ROOM FOR CAMS | RISK: HIDDEN RECORDING | DO |

Tower line-items: TYPE = IP CAM · OUIS = 17 · MAKER = WYZE · MAKER 2 = TUYA · MAKER 3 = VERKADA · RADIO = WIFI · RISK = HIDDEN · DO = LOOK

Source: README: 17 Wi-Fi OUI (Wyze, Amazon, Tuya, Verkada, Avigilon, Axis, ...); generic / covert IP cameras.

## 14. RING (threat LOW, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | RING DOORBELL / CAM | AMAZON HOME CAMERA | LOW |
| DETECT | RING + AMAZON OUI | 15 WIFI OUI PREFIXES | WIFI |
| ACTION | ASSUME AUDIO + VIDEO | RISK: PORCH IS RECORDED | DO |

Tower line-items: MAKER = RING · OWNER = AMAZON · TYPE = DOORBELL · OUIS = 15 · RADIO = WIFI · REC = AUD+VID · RISK = RECORDED · DO = ASSUME

Source: README: 15 Wi-Fi OUI (Ring LLC's registered block + Amazon's). Ring is an Amazon company; doorbells record video and audio.

## 15. GOOGLE TAG (threat MEDIUM, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | FIND MY DEVICE TAG | CHIPOLO PEBBLEBEE MOTO | MED |
| DETECT | BLE SVC FEAA (FMDN) | GOOGLE FIND MY NETWORK | BLE |
| ACTION | ANDROID TRACKER SCAN | RISK: ANDROIDS RELAY IT | DO |

Tower line-items: NET = GOOGLE · SVC = FEAA · PROTO = FMDN · MAKER = CHIPOLO · MAKER 2 = PEBBLEBE · ALSO = MOTO TAG · RISK = TRACKING · DO = SCAN

Source: README: BLE service UUID 0xFEAA; Google Find My Device trackers (Chipolo, Pebblebee, Moto Tag). Android has unknown tracker alerts / manual scan.

## 16. EVIL TWIN (threat HIGH, radio WIFI)

| folder | name | detail | tag |
|---|---|---|---|
| ID | EVIL TWIN HOTSPOT | FAKE COPY OF A REAL AP | HIGH |
| DETECT | 1 SSID, 2 BSSIDS | ENCRYPTION DISAGREES | WIFI |
| ACTION | FORGET OPEN COPIES | RISK: PASSWORD PHISHING | DO |

Tower line-items: TYPE = ROGUE AP · SSID = SAME · BSSIDS = TWO · LOCK = DIFFERS · LURE = PORTAL · RISK = PHISHING · USE = VPN · DO = FORGET

Source: README: one SSID beaconing from two BSSIDs that disagree about encryption.

## 17. IBEACON (threat LOW, radio BLE)

| folder | name | detail | tag |
|---|---|---|---|
| ID | APPLE IBEACON | SHOP PROXIMITY BEACON | LOW |
| DETECT | HEADER 4C 00 02 15 | UUID + MAJOR + MINOR | BLE |
| ACTION | LIMIT APP BT ACCESS | RISK: IN-STORE TRACKING | DO |

Tower line-items: FORMAT = APPLE · HEADER = 4C000215 · ID = UUID · PLUS = MAJ+MIN · USE = RETAIL · RISK = LOCATION · DEFAULT = OFF · DO = APP PERM

Source: README: exact Apple header 4C 00 02 15 (UUID + major + minor follow); retail proximity beacons; SquachWatch ships IBEACON off by default (volume).
