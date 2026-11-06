<#
.SYNOPSIS
  Safe CYD (ESP32-2432S028) flash preflight + optional write-flash for BOTcorps Windows.

.DESCRIPTION
  Replaces ad-hoc PowerShell one-liners. Default mode is READ-ONLY preflight.
  Pass -Flash only after gates pass to write the merged image at 0x0.

  Guards:
  - Binary size + ESP magic so app-only 0x10000 launcher bins / corrupt files cannot be
    flashed at 0x0 by mistake.
  - USB/PnP excludes VID_303A (ESP32-S3 / Cardputer Adv native USB) forever.
  - Accepts only CH340 (VID_1A86&PID_7523) or CP210x (VID_10C4&PID_EA60) as CYD candidates.
  - Silicon gate: classic ESP32-D0WD* only (abort S3/C3/C6), prints MAC, confirms 4MB.
  - Flash uses `py -m esptool` (v5.3.1+) to avoid the Microsoft Store python.exe stub.
  - Baud 460800 with one automatic retry at 115200 on serial/framing errors.
  - Requires "Hash of data verified" before exit 0.

.NOTES
  Mobandy confirmed CYD MAC: a4:f0:0f:68:15:00
  Default bin: C:\Users\Brock\Downloads\cyd-livescan-b29f8ab-merged.bin
  SHA-256 (known good): 09b2ca752fe8ddc8a386b7ee164e55f233ed74824a684a9268c983ab24bfc04c

  PlatformIO firmware.factory.bin / merged 0x0 images carry 0xFF pad at file offset 0 and
  bootloader magic 0xE9 at offset 0x1000 (plus partition table 0xAA50 at 0x8000).
  App-only launcher bins have 0xE9 at offset 0 and are REJECTED for a 0x0 flash.
#>
[CmdletBinding()]
param(
    [string]$BinPath = 'C:\Users\Brock\Downloads\cyd-livescan-b29f8ab-merged.bin',
    [string]$ExpectedSha256 = '',
    [string]$Port = '',
    [int]$Baud = 460800,
    [switch]$Flash
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

function Write-Gate([string]$Name, [string]$Status, [string]$Detail = '') {
    $line = "[{0}] {1}" -f $Name, $Status
    if ($Detail) { $line = "$line — $Detail" }
    if ($Status -eq 'FAIL') { Write-Host $line -ForegroundColor Red }
    elseif ($Status -eq 'WARN') { Write-Host $line -ForegroundColor Yellow }
    else { Write-Host $line -ForegroundColor Green }
}

function Get-FileSha256Hex([string]$Path) {
    $hash = Get-FileHash -Algorithm SHA256 -Path $Path
    return $hash.Hash.ToLowerInvariant()
}

function Test-Esp32Merged0x0Magic([byte[]]$Bytes) {
    # Valid 0x0 merged/factory for classic ESP32:
    #   A) bootloader magic 0xE9 at offset 0x0 (packed), OR
    #   B) PlatformIO factory: 0xE9 at offset 0x1000 (typical; leading 0xFF pad)
    # Reject app-only launcher images (0xE9 at 0, no bootloader at 0x1000, no 0xAA50 @ 0x8000).
    if ($null -eq $Bytes -or $Bytes.Length -lt 1) { return $false }

    $hasBootAt0 = ($Bytes[0] -eq 0xE9)
    $hasBootAt1000 = ($Bytes.Length -gt 0x1000 -and $Bytes[0x1000] -eq 0xE9)
    $hasPartTable = ($Bytes.Length -gt 0x8001 -and $Bytes[0x8000] -eq 0xAA -and $Bytes[0x8001] -eq 0x50)

    if ($hasBootAt1000) { return $true }
    if ($hasBootAt0 -and $hasPartTable) { return $true }
    return $false
}

function Get-EspToolCommand {
    # Prefer `py -m esptool` (Windows launcher) over Store stub python.exe / bare esptool.exe
    $py = Get-Command py -ErrorAction SilentlyContinue
    if ($py) {
        return @{ Exe = 'py'; Prefix = @('-m', 'esptool') }
    }
    $esptool = Get-Command esptool -ErrorAction SilentlyContinue
    if ($esptool) {
        return @{ Exe = 'esptool'; Prefix = @() }
    }
    throw "Neither 'py' nor 'esptool' found on PATH. Install Python + esptool 5.3.1 (py -m pip install esptool==5.3.1)."
}

function Invoke-EspTool([string[]]$EspArgs) {
    $tool = Get-EspToolCommand
    $all = @()
    $all += $tool.Prefix
    $all += $EspArgs
    Write-Host ("\$ {0} {1}" -f $tool.Exe, ($all -join ' ')) -ForegroundColor DarkGray
    $output = & $tool.Exe @all 2>&1
    $code = $LASTEXITCODE
    return [pscustomobject]@{ ExitCode = $code; Output = ($output | Out-String) }
}

# -----------------------------------------------------------------------------
# Step 1 — Binary gate
# -----------------------------------------------------------------------------
Write-Host "`n=== Step 1: Binary gate ===" -ForegroundColor Cyan

if (-not (Test-Path -LiteralPath $BinPath)) {
    Write-Gate 'BIN' 'FAIL' "File not found: $BinPath"
    exit 2
}

$item = Get-Item -LiteralPath $BinPath
$size = [int64]$item.Length
if ($size -lt 65536 -or $size -gt 4194304) {
    Write-Gate 'BIN' 'FAIL' ("Size {0} bytes outside 65536..4194304" -f $size)
    exit 2
}
Write-Gate 'BIN' 'OK' ("exists, size={0}" -f $size)

$bytes = [System.IO.File]::ReadAllBytes($BinPath)
if (-not (Test-Esp32Merged0x0Magic $bytes)) {
    $b0 = if ($bytes.Length -gt 0) { '0x{0:X2}' -f $bytes[0] } else { 'n/a' }
    $b1k = if ($bytes.Length -gt 0x1000) { '0x{0:X2}' -f $bytes[0x1000] } else { 'n/a' }
    Write-Gate 'BIN' 'FAIL' ("Not a valid ESP32 0x0 merged/factory image (byte0={0}, byte@0x1000={1}). App-only launcher bins (0xE9 at 0 without bootloader@0x1000 / partition@0x8000) must NOT be flashed at 0x0." -f $b0, $b1k)
    exit 2
}
if ($bytes.Length -gt 0x1000 -and $bytes[0x1000] -eq 0xE9) {
    Write-Gate 'MAGIC' 'OK' '0xE9 at offset 0x1000 (PlatformIO factory / merged 0x0 layout)'
} elseif ($bytes[0] -eq 0xE9) {
    Write-Gate 'MAGIC' 'OK' '0xE9 at offset 0x0 with partition table @ 0x8000'
}

$sha = Get-FileSha256Hex $BinPath
Write-Gate 'SHA256' 'OK' $sha
if ($ExpectedSha256) {
    $want = $ExpectedSha256.Trim().ToLowerInvariant()
    if ($sha -ne $want) {
        Write-Gate 'SHA256' 'FAIL' ("expected {0}" -f $want)
        exit 2
    }
    Write-Gate 'SHA256' 'OK' 'matches -ExpectedSha256'
}

# -----------------------------------------------------------------------------
# Step 2 — USB & Port safety gate
# -----------------------------------------------------------------------------
Write-Host "`n=== Step 2: USB & Port safety gate ===" -ForegroundColor Cyan

$present = @()
try {
    $present = @(Get-PnpDevice -PresentOnly -ErrorAction Stop)
} catch {
    Write-Gate 'PNP' 'WARN' ("Get-PnpDevice failed: {0}" -f $_.Exception.Message)
}

$advHits = @()
$ch340 = @()
$cp210 = @()
$vid0000 = @()
$badStatus = @()

foreach ($dev in $present) {
    $id = [string]$dev.InstanceId
    $stat = [string]$dev.Status
    if ($id -match 'VID_303A') {
        $advHits += $dev
    }
    if ($id -match 'VID_1A86&PID_7523') {
        $ch340 += $dev
    }
    if ($id -match 'VID_10C4&PID_EA60') {
        $cp210 += $dev
    }
    if ($id -match 'VID_0000') {
        $vid0000 += $dev
    }
    if ($stat -and $stat -ne 'OK' -and ($id -match 'USB\\')) {
        $badStatus += $dev
    }
}

if ($advHits.Count -gt 0) {
    Write-Gate 'ADV' 'EXCLUDED' ("VID_303A present ({0} device(s)) — ESP32-S3 / Cardputer Adv native USB will NEVER be selected for CYD flash" -f $advHits.Count)
    foreach ($a in $advHits) {
        Write-Host ("    ADV: {0} | {1} | {2}" -f $a.Status, $a.Class, $a.InstanceId) -ForegroundColor Yellow
    }
} else {
    Write-Gate 'ADV' 'OK' 'no VID_303A (Adv) devices present'
}

function Get-ComFromInstanceId([string]$InstanceId) {
    # Prefer friendly COM extraction from PnP children / device caption
    try {
        $children = Get-PnpDeviceProperty -InstanceId $InstanceId -KeyName 'DEVPKEY_Device_Children' -ErrorAction SilentlyContinue
    } catch { $children = $null }
    return $null
}

function Get-ComPortsForVidPid {
    param([string]$VidPidPattern)
    $ports = New-Object System.Collections.Generic.List[string]
    # Serial devices often expose COM via Win32_SerialPort / registry; also scan Get-CimInstance
    try {
        $serials = Get-CimInstance -ClassName Win32_SerialPort -ErrorAction SilentlyContinue
        foreach ($s in @($serials)) {
            $pnp = [string]$s.PNPDeviceID
            if ($pnp -match $VidPidPattern) {
                [void]$ports.Add([string]$s.DeviceID)
            }
        }
    } catch {}
    # Fallback: parse mode / PnP friendly names containing (COMx)
    foreach ($dev in $present) {
        $id = [string]$dev.InstanceId
        if ($id -notmatch $VidPidPattern) { continue }
        $name = [string]$dev.FriendlyName
        if ($name -match '(COM\d+)') {
            [void]$ports.Add($Matches[1])
        }
    }
    return @($ports | Select-Object -Unique)
}

$cydPorts = New-Object System.Collections.Generic.List[string]
foreach ($p in (Get-ComPortsForVidPid -VidPidPattern 'VID_1A86&PID_7523')) { [void]$cydPorts.Add($p) }
foreach ($p in (Get-ComPortsForVidPid -VidPidPattern 'VID_10C4&PID_EA60')) { [void]$cydPorts.Add($p) }
$cydPorts = @($cydPorts | Select-Object -Unique)

$allCom = @([System.IO.Ports.SerialPort]::GetPortNames() | Sort-Object -Unique)
Write-Gate 'COM' 'INFO' ("system COM ports: {0}" -f ($(if ($allCom) { $allCom -join ', ' } else { '(none)' })))
Write-Gate 'CH340' 'INFO' ("present devices={0}, mapped COM={1}" -f $ch340.Count, (($cydPorts | Where-Object { $_ }) -join ', '))
if ($ch340.Count -gt 0) {
    foreach ($d in $ch340) { Write-Host ("    CH340: {0} | {1}" -f $d.Status, $d.InstanceId) -ForegroundColor DarkGray }
}
if ($cp210.Count -gt 0) {
    Write-Gate 'CP210x' 'INFO' ("present devices={0}" -f $cp210.Count)
    foreach ($d in $cp210) { Write-Host ("    CP210x: {0} | {1}" -f $d.Status, $d.InstanceId) -ForegroundColor DarkGray }
}

# Never allow a VID_303A-mapped COM even if user passes -Port
$advCom = New-Object System.Collections.Generic.List[string]
try {
    $serialsAll = Get-CimInstance -ClassName Win32_SerialPort -ErrorAction SilentlyContinue
    foreach ($s in @($serialsAll)) {
        if ([string]$s.PNPDeviceID -match 'VID_303A') {
            [void]$advCom.Add([string]$s.DeviceID)
            Write-Gate 'ADV-COM' 'EXCLUDED' ("{0} is VID_303A — blocked" -f $s.DeviceID)
        }
    }
} catch {}

$selected = $Port
if ($selected) {
    if ($advCom -contains $selected) {
        Write-Gate 'PORT' 'FAIL' ("Refusing {0}: maps to VID_303A Adv device" -f $selected)
        exit 3
    }
    if ($allCom -notcontains $selected) {
        Write-Gate 'PORT' 'FAIL' ("{0} not in SerialPort.GetPortNames()" -f $selected)
        exit 3
    }
    Write-Gate 'PORT' 'OK' ("using -Port {0}" -f $selected)
} else {
    if ($cydPorts.Count -eq 1) {
        $selected = $cydPorts[0]
        Write-Gate 'PORT' 'OK' ("auto-selected CYD COM {0}" -f $selected)
    } elseif ($cydPorts.Count -gt 1) {
        Write-Gate 'PORT' 'FAIL' ("multiple CYD COM candidates: $($cydPorts -join ', '). Pass -Port COMx")
        exit 3
    } else {
        Write-Gate 'PORT' 'FAIL' 'zero CYD COM ports (CH340/CP210x) present'
        # Diagnosis from live PnP
        $usbLike = @($present | Where-Object {
            $c = [string]$_.Class
            $id = [string]$_.InstanceId
            ($c -match 'USB|Ports|Modem') -or ($id -match 'USB\\')
        })
        $externalUsb = @($usbLike | Where-Object {
            $id = [string]$_.InstanceId
            $name = [string]$_.FriendlyName
            # Exclude root hubs and integrated camera
            -not ($name -match 'Root Hub|USB Root|Integrated Camera|Camera') -and -not ($id -match 'ROOT_HUB')
        })
        if ($externalUsb.Count -eq 0) {
            Write-Gate 'DIAG' 'HINT' '(a) only root hubs / integrated camera present → external USB hub/dock unplugged OR power-only USB cord (no D+/D- data lines). Plug a known data cable straight into a laptop USB port.'
        }
        if ($vid0000.Count -gt 0 -or $badStatus.Count -gt 0) {
            Write-Gate 'DIAG' 'HINT' '(b) VID_0000 and/or Status -ne OK on USB devices → USB descriptor/port reset or CH340 driver fault. Reseat cable, try another port, reinstall CH340 driver.'
            foreach ($d in ($vid0000 + $badStatus | Select-Object -Unique)) {
                Write-Host ("    BAD: Status={0} Class={1} Id={2}" -f $d.Status, $d.Class, $d.InstanceId) -ForegroundColor Yellow
            }
        }
        exit 3
    }
}

# Final Adv exclusion on selected port
if ($advCom -contains $selected) {
    Write-Gate 'PORT' 'FAIL' ("Refusing {0}: Adv VID_303A" -f $selected)
    exit 3
}

# -----------------------------------------------------------------------------
# Step 3 — Silicon & MAC gate
# -----------------------------------------------------------------------------
Write-Host "`n=== Step 3: Silicon & MAC gate ===" -ForegroundColor Cyan

$flashId = Invoke-EspTool @('--chip', 'esp32', '-p', $selected, 'flash-id')
Write-Host $flashId.Output
if ($flashId.ExitCode -ne 0) {
    Write-Gate 'FLASH-ID' 'FAIL' ("esptool flash-id exit {0}" -f $flashId.ExitCode)
    exit 4
}

$text = $flashId.Output
if ($text -match 'ESP32-S3|ESP32S3|ESP32-C3|ESP32C3|ESP32-C6|ESP32C6') {
    Write-Gate 'CHIP' 'FAIL' 'Detected S3/C3/C6 — aborting (CYD is classic ESP32 only)'
    exit 4
}
if ($text -match '(ESP32-D0WD[^\s,]*)') {
    Write-Gate 'CHIP' 'OK' $Matches[1]
} elseif ($text -match 'Chip is (ESP32[^\r\n]*)') {
    $chipLine = $Matches[1].Trim()
    if ($chipLine -match 'ESP32-S|ESP32-C') {
        Write-Gate 'CHIP' 'FAIL' $chipLine
        exit 4
    }
    Write-Gate 'CHIP' 'OK' $chipLine
} else {
    Write-Gate 'CHIP' 'WARN' 'Could not parse chip string; raw flash-id printed above'
}

$mac = $null
if ($text -match 'MAC:\s*([0-9a-fA-F:]{17})') {
    $mac = $Matches[1].ToLowerInvariant()
    $known = 'a4:f0:0f:68:15:00'
    if ($mac -eq $known) {
        Write-Gate 'MAC' 'OK' ("{0} (matches Mobandy confirmed CYD)" -f $mac)
    } else {
        Write-Gate 'MAC' 'WARN' ("{0} (Mobandy confirmed CYD is {1})" -f $mac, $known)
    }
} else {
    Write-Gate 'MAC' 'WARN' 'MAC not parsed from flash-id'
}

if ($text -match 'Detected flash size:\s*(\d+MB)') {
    $fs = $Matches[1]
    if ($fs -eq '4MB') {
        Write-Gate 'FLASH-SIZE' 'OK' '4MB'
    } else {
        Write-Gate 'FLASH-SIZE' 'FAIL' ("expected 4MB, got {0}" -f $fs)
        exit 4
    }
} elseif ($text -match '4MB') {
    Write-Gate 'FLASH-SIZE' 'OK' '4MB (string match)'
} else {
    Write-Gate 'FLASH-SIZE' 'WARN' 'could not confirm 4MB from flash-id'
}

# -----------------------------------------------------------------------------
# Step 4 — Flash gate (only with -Flash)
# -----------------------------------------------------------------------------
Write-Host "`n=== Step 4: Flash gate ===" -ForegroundColor Cyan
if (-not $Flash) {
    Write-Gate 'FLASH' 'SKIP' 'read-only preflight OK. Re-run with -Flash to write 0x0.'
    exit 0
}

function Invoke-WriteFlash([int]$UseBaud) {
    $args = @(
        '--chip', 'esp32',
        '-p', $selected,
        '-b', "$UseBaud",
        '--before', 'default-reset',
        '--after', 'hard-reset',
        'write-flash', '-z',
        '--flash-mode', 'dio',
        '--flash-freq', '40m',
        '--flash-size', '4MB',
        '0x0', $BinPath
    )
    return Invoke-EspTool $args
}

$result = Invoke-WriteFlash -UseBaud $Baud
Write-Host $result.Output
$needRetry = $false
if ($result.ExitCode -ne 0) {
    if ($Baud -ne 115200 -and $result.Output -match '(?i)serial|framing|INVALID_RESPONSE|Failed to connect|timeout|No serial data') {
        $needRetry = $true
        Write-Gate 'FLASH' 'WARN' ("baud {0} failed with serial/framing-class error; retrying once at 115200" -f $Baud)
    } else {
        Write-Gate 'FLASH' 'FAIL' ("write-flash exit {0}" -f $result.ExitCode)
        exit 5
    }
}

if ($needRetry) {
    $result = Invoke-WriteFlash -UseBaud 115200
    Write-Host $result.Output
    if ($result.ExitCode -ne 0) {
        Write-Gate 'FLASH' 'FAIL' ("retry at 115200 exit {0}" -f $result.ExitCode)
        exit 5
    }
}

if ($result.Output -notmatch 'Hash of data verified') {
    Write-Gate 'FLASH' 'FAIL' 'missing required success marker: Hash of data verified'
    exit 5
}

Write-Gate 'FLASH' 'OK' 'Hash of data verified'
exit 0
