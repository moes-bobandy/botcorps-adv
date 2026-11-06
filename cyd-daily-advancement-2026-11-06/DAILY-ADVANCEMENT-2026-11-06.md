# CYDbot Daily Role Advancement — 2026-11-06 (America/New_York)

**Seat:** CYDbot · Cheap Yellow Display / BOTcorps  
**Scope:** GREEN only (box audits, cheat-sheet/standard sharpening, orphan backup). No flash, no RED gates.

## Concrete advancements (with proof)

### 1. Firmware digest — Launcher 3.0.0 alert delivered
- Journal (2026-11-06): pinged **Launcher 3.0.0** (`bmorcelli/Launcher`, published 2026-11-06 ~5:48 AM ET).
- Direct-flash asset called out: **`Launcher-CYD-2432S028.bin`** (also CYD-2-USB / elecrow-28B).
- Major bump from 2.9.1 (2026-09-04): bootscreen timing, deepsleep/stealth start, battery icon, new device ports.
- Other watched stables unchanged: Marauder v1.20.0, BambuHelper v3.9.1, SquachWatch v1.40.0, HaleHound v4.0, GhostESP v2.2.2, Bruce 1.16.1, Fr4nkFletcher v1.4.3.

### 2. `cyd_flash_preflight.ps1` built & verified (replaces ad-hoc flash one-liners)
Path: `/workspace/out/cyd-daily-advancement-2026-11-06/cyd_flash_preflight.ps1`

Sharper standard vs yesterday's manual PowerShell checks:
- **Binary gate:** size 65536..4194304; ESP32 **0x0 merged/factory magic** (PlatformIO `0xE9` @ `0x1000` / partition `0xAA50` @ `0x8000`) so **app-only `0x10000` launcher bins are rejected** for a `0x0` flash; optional case-insensitive `-ExpectedSha256`.
- **USB/Port gate:** `Get-PnpDevice -PresentOnly` + `[System.IO.Ports.SerialPort]::GetPortNames()`; **hard-excludes `VID_303A`** (ESP32-S3 / Cardputer Adv); accepts CH340 `VID_1A86&PID_7523` + CP210x `VID_10C4&PID_EA60` only; diagnoses (a) unplugged dock / power-only cord vs (b) `VID_0000` / Status≠OK driver fault when zero CYD COMs.
- **Silicon/MAC gate:** `py -m esptool --chip esp32 -p <COM> flash-id` — classic **ESP32-D0WD\*** only (abort S3/C3/C6), print MAC (flag match to Mobandy CYD `a4:f0:0f:68:15:00`), confirm **4MB**.
- **Flash gate (`-Flash` only):** `write-flash` at `0x0` via `py -m esptool` (avoids Store `python.exe` stub), **460800→115200** one-shot retry on serial/framing errors, requires **`Hash of data verified`**.
- **Verification on box:** `pwsh` absent → Python structure check (balanced braces, required params/tokens) + magic/size/SHA rules against `/workspace/out/fw-port-1105/cyd-livescan-b29f8ab-merged.bin` (**PASS**, sha256 `09b2ca75…4bfc04c`) and launcher.bin (**REJECT** for 0x0). See `verify-preflight.txt`.

### 3. CYD GitHub backup audit + orphan backup
- Audit file: `CYD-BACKUP-AUDIT-2026-11-06.md`
- Findings: `fw-port-1105` already on `origin/backup/cyd-fw-port-1105`; **`cyd-gbc-own`** and **`radar-current-meter-interact`** were unbacked (<95 MB) → included with today's deliverables on new orphan ref `backup/cyd-daily-advancement-2026-11-06` (no checkout of portrait worktree).

## SHA-256s (key artifacts)

| Artifact | SHA-256 |
|---|---|
| `cyd-livescan-b29f8ab-merged.bin` | `09b2ca752fe8ddc8a386b7ee164e55f233ed74824a684a9268c983ab24bfc04c` |
| `cyd-livescan-b29f8ab-launcher.bin` / `launcher.bin` | `ece72d53a6b4f66d45511da080c0f14e55893c65d28fa3762fb45d046dc3b78a` |
| `gbc-cyd-own-M3.19-factory-0x0.bin` | `08bd609bcf201b951d7ff2fbe8fb3e3e00c236e092db9669a5078c0dfdf2ab81` |
| `cyd_flash_preflight.ps1` | `53c65263f33592f4a3a4449597860159f739942dfd214dd54952994593a2107c` |

## Next GREEN leverage (not done today)
- When CYD data COM appears on BOTcorps: run preflight read-only, then await Mobandy `-Flash` unlock.
- Keep digest baseline current after each Launcher/Marauder/etc. ping.

## Backup proof

`origin/backup/cyd-daily-advancement-2026-11-06` = `27523ab8f0ec9dc2ded7b3cabcfc3d7adb6826ae`
