#!/usr/bin/env python3
"""2026-11-14 (Mobandy 9:06 AM ET): REAL folder content for all 18 Morpheus devices, inside the EXISTING folder UI (text only).
Per device:
  3 folders (SCROLL 1/2/3 = ID / DETECT / ACTION): name (big 5x7 line, <= 20 chars), detail line (3x5, <= 24 chars), right tag (<= 5 chars;
     replaces the old fake "x.yKB" size: threat level / radio / DO)
  8 scrolling line-items inside the tower: KEY (<= 8 chars) + VALUE (<= 8 chars; replaces the old fake file size under each name)
Facts: SquachWatch README "What it detects" table (/workspace/tmp-squach/README.md) + the existing MO_DEV text + public knowledge (Apple Find My,
Tile/Life360, Samsung SmartThings Find, Google Find My Device network, ASTM F3411 Remote ID, 802.11 management frames / 802.11w PMF).
No invented measurements. Writes folders18.inc (C tables, included by mo_mix.inc) and ../folders.md (review dump). Run: python3 folders18.py"""
import os, re, sys
D = os.path.dirname(os.path.abspath(__file__))
TH = {'HIGH': 'HIGH', 'MEDIUM': 'MED', 'LOW': 'LOW'}
# name, threat, radio tag, [ (F1 name, F1 detail), (F2 name, F2 detail), (F3 name, F3 detail) ], [8 x (key, value)], sources
DEV = [
 ("DEAUTH", "HIGH", "WIFI",
  [("DEAUTH FLOOD", "FAKE 802.11 KICK FRAMES"), ("MGMT FRAME BURST", "DEAUTH + DISASSOC RATE"), ("USE WPA3 / PMF", "RISK: KICK + KEY CAPTURE")],
  [("TYPE", "ATTACK"), ("FRAME", "DEAUTH"), ("SUBTYPE", "12"), ("DISASOC", "10"), ("SIGN", "BURST"), ("BAND", "2.4 GHZ"), ("FIX", "PMF"), ("STD", "802.11W")],
  "README: rate-detected burst, not a signature. 802.11 mgmt subtypes 12 deauth / 10 disassoc; 802.11w PMF protects them (mandatory in WPA3)."),
 ("HACK", "HIGH", "WIFI",
  [("WEAK WI-FI AP", "OPEN OR WEP NETWORK"), ("BEACON SAYS OPEN/WEP", "SECURITY FIELD OF BEACON"), ("STAY OFF / USE VPN", "RISK: TRAFFIC READABLE")],
  [("TYPE", "AP"), ("LOCK", "OPEN"), ("OR", "WEP"), ("WEP", "BROKEN"), ("SNIFF", "EASY"), ("SAFE", "WPA3"), ("ALSO OK", "WPA2"), ("DO", "VPN")],
  "MO_DEV: OPEN or WEP access points. WEP is cryptographically broken; open Wi-Fi is unencrypted."),
 ("FLOCK", "LOW", "WI+BT",
  [("FLOCK SAFETY ALPR", "PLATE CAMERA ON A POLE"), ("SSID FLOCK- / BLE ID", "49 OUI, CO ID 09C8"), ("ASK RETENTION RULES", "RISK: CAR MOVES LOGGED")],
  [("MAKER", "FLOCK"), ("TYPE", "ALPR CAM"), ("SSID", "FLOCK-"), ("BLE CO", "09C8"), ("BLE NAME", "PENGUIN"), ("OUIS", "49"), ("RISK", "TRACKED"), ("DO", "ASK CITY")],
  "README: 49 Wi-Fi OUI prefixes, flock- and bare Flock SSIDs, BLE names Flock/Penguin/Pigvision, company ID 0x09C8, Flock BLE service UUID."),
 ("DRONE", "LOW", "WI+BT",
  [("REMOTE ID DRONE", "UAS BROADCASTING ITS ID"), ("RID: BLE FFFA, WIFI", "ASTM F3411 MSG DECODED"), ("NOTE TIME + PLACE", "RISK: AERIAL FILMING")],
  [("TYPE", "UAS"), ("STD", "F3411"), ("BLE SVC", "FFFA"), ("WIFI", "VEND IE"), ("SENDS", "SERIAL"), ("ALSO", "OP LOC"), ("RISK", "FILMING"), ("DO", "REPORT")],
  "README: Remote ID in BLE service data 0xFFFA or a Wi-Fi beacon vendor element; ASTM F3411 decoded: position, altitude, serial, operator location."),
 ("AIR TAG", "MEDIUM", "BLE",
  [("APPLE AIRTAG", "FIND MY BLE TRACKER"), ("APPLE 004C, TYPE 12", "FIND MY OFFLINE ADVERT"), ("SCAN, FIND, DISABLE", "RISK: STALKER TAG IN BAG")],
  [("MAKER", "APPLE"), ("NET", "FIND MY"), ("CO ID", "004C"), ("TYPE", "12"), ("HAS", "UWB+NFC"), ("ALERTS", "IOS+AND"), ("RISK", "STALKING"), ("DO", "PULL BAT")],
  "README: company ID 0x004C + Find My payload check. Find My offline-finding advert type 0x12. AirTag has U1 UWB + NFC; iOS and Android have unknown-tracker alerts; the coin cell can be removed."),
 ("KEYFOB", "MEDIUM", "BLE",
  [("TILE KEYRING TAG", "LIFE360 BLE TRACKER"), ("BLE SVC FEED / FEEC", "TILE SERVICE UUIDS"), ("TILE SCAN AND SECURE", "RISK: FOLLOWS YOUR MOVES")],
  [("MAKER", "TILE"), ("OWNER", "LIFE360"), ("SVC", "FEED"), ("SVC 2", "FEEC"), ("RADIO", "BLE"), ("RISK", "TRACKING"), ("CHECK", "TILE APP"), ("DO", "SCAN")],
  "README: BLE service UUID 0xFEED / 0xFEEC. Tile is owned by Life360; the Tile app has Scan and Secure for unknown Tiles."),
 ("META", "MEDIUM", "BLE",
  [("RAY-BAN META GLASSES", "CAMERA GLASSES, SNAP TOO"), ("BLE SVC FD5F", "META/LUXOTTICA/SNAP IDS"), ("WATCH CAPTURE LED", "RISK: SILENT PHOTO/VIDEO")],
  [("MAKER", "META"), ("BRAND", "RAY-BAN"), ("ALSO", "SNAP"), ("SVC", "FD5F"), ("CAM", "PHO+VID"), ("LED", "CAPTURE"), ("RISK", "FILMING"), ("DO", "ASK")],
  "README: BLE service UUID 0xFD5F + Meta / Luxottica / Snap company IDs (Ray-Ban Meta, Snap Spectacles). Ray-Ban Meta shows a capture LED while recording."),
 ("SKIMMER", "HIGH", "BT",
  [("BT CARD SKIMMER", "HIDDEN IN PUMPS + ATMS"), ("HC-05/06, RN42 NAMES", "SPP UUID 1101 + 3 OUI"), ("TAP PAY, COVER PIN", "RISK: CARD + PIN STOLEN")],
  [("TYPE", "SKIMMER"), ("MOD 1", "HC-05"), ("MOD 2", "HC-06"), ("MOD 3", "RN42"), ("MOD 4", "BT04-A"), ("SPP", "1101"), ("RISK", "CARD"), ("DO", "TAP PAY")],
  "README: BLE/BT name match (HC-05/06/03, RN42, BT04-A) + SPP UUID 0x1101 + 3 OUI. Tap-to-pay avoids the mag stripe; cover the keypad."),
 ("FLIPPER", "HIGH", "BLE",
  [("FLIPPER ZERO", "MULTI-RADIO HACK TOOL"), ("BLE SVC 3081-3083", "CO ID 0E29, OUI 0CFA22"), ("IGNORE POPUPS", "RISK: BLE SPAM, RF CLONE")],
  [("RADIO", "SUB-GHZ"), ("ALSO", "NFC+IR"), ("RFID", "125KHZ"), ("SVC", "3081"), ("CO ID", "0E29"), ("OUI", "0CFA22"), ("RISK", "CLONING"), ("DO", "UPDATE")],
  "README (HACKER type): Flipper service UUIDs 0x3081-0x3083, company ID 0x0E29, OUI 0C:FA:22. Flipper Zero: sub-GHz radio, NFC, 125 kHz RFID, infrared, BLE; BLE pop-up spam was patched in phone updates."),
 ("SAMSUNG", "MEDIUM", "BLE",
  [("GALAXY SMARTTAG", "SAMSUNG BLE TRACKER"), ("BLE SVC FD5A", "SMARTTAG ADVERT UUID"), ("SMARTTHINGS TAG SCAN", "RISK: GALAXY NET LOCATES")],
  [("MAKER", "SAMSUNG"), ("NET", "ST FIND"), ("SVC", "FD5A"), ("ALSO", "FD59"), ("PLUS", "UWB"), ("RISK", "TRACKING"), ("APP", "SMARTTH."), ("DO", "SCAN")],
  "README: BLE service UUID 0xFD5A (MO_DEV also FD59, both Samsung). SmartThings Find network; SmartTag+ adds UWB; SmartThings has an unknown tag search."),
 ("AXON", "MEDIUM", "WIFI",
  [("AXON BODY CAMERA", "POLICE CAMS, TASERS"), ("SSID AB2- AB3- AB4-", "AXON- SSID + 3 WIFI OUI"), ("KNOW YOUR RIGHTS", "RISK: YOU MAY BE FILMED")],
  [("MAKER", "AXON"), ("USER", "POLICE"), ("SSID", "AB3-"), ("ALSO", "AB4-"), ("OR", "AXON-"), ("OUIS", "3"), ("RISK", "FILMED"), ("DO", "RIGHTS")],
  "README: 3 Wi-Fi OUI + SSID prefixes AB2-/AB3-/AB4-/AXON-; Axon body cameras, TASERs, LE equipment."),
 ("RAVEN", "LOW", "BLE",
  [("RAVEN SHOT SENSOR", "POLE AUDIO DETECTOR"), ("BLE SVC 3100-3500", "ANY UUID IN THAT RANGE"), ("ASK AUDIO POLICY", "RISK: LISTENS FOR SOUNDS")],
  [("MAKER", "FLOCK"), ("TYPE", "GUNSHOT"), ("MOUNT", "POLE"), ("SVC LO", "3100"), ("SVC HI", "3500"), ("RADIO", "BLE"), ("RISK", "AUDIO"), ("DO", "ASK")],
  "README: any service UUID from 0x3100 to 0x3500; Raven gunshot detector (Flock Safety product)."),
 ("ALPR", "LOW", "WIFI",
  [("PLATE READER CAMERA", "MOTOROLA OR GENETEC"), ("WIFI MAKER OUI", "7 OUI: 5 MOTO, 2 GENETEC"), ("ASK WHO GETS DATA", "RISK: TRIPS LOGGED")],
  [("TYPE", "ALPR"), ("MAKER", "MOTOROLA"), ("ALSO", "GENETEC"), ("OUIS", "7"), ("MOTO", "5"), ("GENETEC", "2"), ("RISK", "LOGGED"), ("DO", "ASK")],
  "README: 7 Wi-Fi OUI (5 Motorola Solutions, 2 Genetec)."),
 ("CAMERA", "MEDIUM", "WIFI",
  [("WI-FI IP CAMERA", "WYZE TUYA VERKADA AXIS"), ("CAMERA MAKER OUI", "17 KNOWN CAM OUI ON WIFI"), ("CHECK ROOM FOR CAMS", "RISK: HIDDEN RECORDING")],
  [("TYPE", "IP CAM"), ("OUIS", "17"), ("MAKER", "WYZE"), ("MAKER 2", "TUYA"), ("MAKER 3", "VERKADA"), ("RADIO", "WIFI"), ("RISK", "HIDDEN"), ("DO", "LOOK")],
  "README: 17 Wi-Fi OUI (Wyze, Amazon, Tuya, Verkada, Avigilon, Axis, ...); generic / covert IP cameras."),
 ("RING", "LOW", "WIFI",
  [("RING DOORBELL / CAM", "AMAZON HOME CAMERA"), ("RING + AMAZON OUI", "15 WIFI OUI PREFIXES"), ("ASSUME AUDIO + VIDEO", "RISK: PORCH IS RECORDED")],
  [("MAKER", "RING"), ("OWNER", "AMAZON"), ("TYPE", "DOORBELL"), ("OUIS", "15"), ("RADIO", "WIFI"), ("REC", "AUD+VID"), ("RISK", "RECORDED"), ("DO", "ASSUME")],
  "README: 15 Wi-Fi OUI (Ring LLC's registered block + Amazon's). Ring is an Amazon company; doorbells record video and audio."),
 ("GOOGLE TAG", "MEDIUM", "BLE",
  [("FIND MY DEVICE TAG", "CHIPOLO PEBBLEBEE MOTO"), ("BLE SVC FEAA (FMDN)", "GOOGLE FIND MY NETWORK"), ("ANDROID TRACKER SCAN", "RISK: ANDROIDS RELAY IT")],
  [("NET", "GOOGLE"), ("SVC", "FEAA"), ("PROTO", "FMDN"), ("MAKER", "CHIPOLO"), ("MAKER 2", "PEBBLEBE"), ("ALSO", "MOTO TAG"), ("RISK", "TRACKING"), ("DO", "SCAN")],
  "README: BLE service UUID 0xFEAA; Google Find My Device trackers (Chipolo, Pebblebee, Moto Tag). Android has unknown tracker alerts / manual scan."),
 ("EVIL TWIN", "HIGH", "WIFI",
  [("EVIL TWIN HOTSPOT", "FAKE COPY OF A REAL AP"), ("1 SSID, 2 BSSIDS", "ENCRYPTION DISAGREES"), ("FORGET OPEN COPIES", "RISK: PASSWORD PHISHING")],
  [("TYPE", "ROGUE AP"), ("SSID", "SAME"), ("BSSIDS", "TWO"), ("LOCK", "DIFFERS"), ("LURE", "PORTAL"), ("RISK", "PHISHING"), ("USE", "VPN"), ("DO", "FORGET")],
  "README: one SSID beaconing from two BSSIDs that disagree about encryption."),
 ("IBEACON", "LOW", "BLE",
  [("APPLE IBEACON", "SHOP PROXIMITY BEACON"), ("HEADER 4C 00 02 15", "UUID + MAJOR + MINOR"), ("LIMIT APP BT ACCESS", "RISK: IN-STORE TRACKING")],
  [("FORMAT", "APPLE"), ("HEADER", "4C000215"), ("ID", "UUID"), ("PLUS", "MAJ+MIN"), ("USE", "RETAIL"), ("RISK", "LOCATION"), ("DEFAULT", "OFF"), ("DO", "APP PERM")],
  "README: exact Apple header 4C 00 02 15 (UUID + major + minor follow); retail proximity beacons; SquachWatch ships IBEACON off by default (volume)."),
]
NAMES = ["DEAUTH","HACK","FLOCK","DRONE","AIR TAG","KEYFOB","META","SKIMMER","FLIPPER","SAMSUNG","AXON","RAVEN","ALPR","CAMERA","RING","GOOGLE TAG","EVIL TWIN","IBEACON"]
TAGS3 = lambda d: [TH[d[1]], d[2], "DO"]
BAD = re.compile(r'EX\.DATA|LOREM|IPSUM|TODO|XXX|PLACEHOLDER|\d\.\dKB|\.TRK$|\.SIG$|\.DEF$|#')
def check():
    errs = []; assert [d[0] for d in DEV] == NAMES
    seen_f = {}; seen_block = {}
    for d in DEV:
        for j, (nm, act) in enumerate(d[3]):
            tag = TAGS3(d)[j]
            if len(nm) > 20: errs.append((d[0], 'name>20', nm))
            if len(act) > 24: errs.append((d[0], 'detail>24', act))
            if len(tag) > 5: errs.append((d[0], 'tag>5', tag))
            # pixel fit (v13.1 positions, x = 92, w = 143): detail 3x5 at x + 6 must end before the tag (sx - 3)
            sw = len(tag) * 6 - 1; sx = 92 + 143 - 6 - sw
            if 92 + 6 + 4 * len(act) - 1 > sx - 3: errs.append((d[0], 'detail overlaps tag', act))
            if 92 + 17 + 6 * len(nm) - 1 > 92 + 143 - 4: errs.append((d[0], 'name overflows', nm))
            for s in (nm, act, tag):
                if BAD.search(s) or s != s.upper(): errs.append((d[0], 'placeholder/case', s))
            k = (nm, act)
            if k in seen_f: errs.append((d[0], 'folder duplicate of', seen_f[k]))
            seen_f[k] = d[0]
        if len(d[4]) != 8: errs.append((d[0], 'items != 8'))
        for k, v in d[4]:
            if len(k) > 8 or len(v) > 8: errs.append((d[0], 'item > 8', k, v))
            if BAD.search(k) or BAD.search(v): errs.append((d[0], 'placeholder item', k, v))
        blk = repr(d[3]) + repr(d[4])
        if blk in seen_block: errs.append((d[0], 'block duplicate'))
        seen_block[blk] = d[0]
    return errs
def cstr(s): return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'
def emit():
    L = ['/* GENERATED by src/folders18.py (2026-11-14): real per-device folder + line-item text. Do not edit by hand. */']
    L.append('static const char *const MO_FOLD_NAME[MO_N][3] = {'); L += ['    {%s},   /* %s */' % (', '.join(cstr(f[0]) for f in d[3]), d[0]) for d in DEV]; L.append('};')
    L.append('static const char *const MO_FOLD_ACT[MO_N][3] = {'); L += ['    {%s},' % ', '.join(cstr(f[1]) for f in d[3]) for d in DEV]; L.append('};')
    L.append('static const char *const MO_FOLD_TAG[MO_N][3] = {'); L += ['    {%s},' % ', '.join(cstr(t) for t in TAGS3(d)) for d in DEV]; L.append('};')
    L.append('static const char *const MO_ITEM_K[MO_N][8] = {'); L += ['    {%s},' % ', '.join(cstr(k) for k, v in d[4]) for d in DEV]; L.append('};')
    L.append('static const char *const MO_ITEM_V[MO_N][8] = {'); L += ['    {%s},' % ', '.join(cstr(v) for k, v in d[4]) for d in DEV]; L.append('};')
    open(D + '/folders18.inc', 'w').write('\n'.join(L) + '\n')
    M = ['# Morpheus folder content: 18 devices (mock, 2026-11-14)', '',
         'Generated from `src/folders18.py`. Each device has three folders, SCROLL 1/2/3 = **ID / DETECT / ACTION**.',
         'Each folder shows a name (up to 20 characters), a detail line (up to 24) and a right tag (up to 5).',
         'The right tag replaces the old made-up "x.yKB" size: it is the threat level, the radio, and "DO".',
         'Inside the tower, 8 scrolling line-items show KEY / VALUE (each up to 8 characters). The value replaces the made-up file size.', '',
         'Sources: SquachWatch README "What it detects" table (`/workspace/tmp-squach/README.md`) and public knowledge, noted per device.', '']
    for i, d in enumerate(DEV):
        M += ['## %d. %s (threat %s, radio %s)' % (i, d[0], d[1], d[2]), '', '| folder | name | detail | tag |', '|---|---|---|---|']
        for j, f in enumerate(d[3]): M.append('| %s | %s | %s | %s |' % (['ID', 'DETECT', 'ACTION'][j], f[0], f[1], TAGS3(d)[j]))
        M += ['', 'Tower line-items: ' + ' · '.join('%s = %s' % kv for kv in d[4]), '', 'Source: ' + d[5], '']
    open(os.path.join(D, '..', 'folders.md'), 'w').write('\n'.join(M))
if __name__ == '__main__':
    e = check()
    if e: print('ERRORS', *e, sep='\n'); sys.exit(1)
    emit(); print('folders18: 18 devices OK, wrote folders18.inc + folders.md')
