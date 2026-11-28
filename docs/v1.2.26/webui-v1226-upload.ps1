param([string]$base = 'http://10.0.4.138')
$ErrorActionPreference = 'Stop'

$work = 'C:\Users\Brock\cyd-webui-work'
$local = 'C:\Users\Brock\cyd-webui-work\BOTcorps-CYD-v1.2.26.bin'
$remoteName = 'BOTcorps-CYD-v1.2.26-db126416.bin'
$expectSize = 2716336
$expectHash = 'db126416dfaaf2f161d865efd2cc3e04ac7d5af6b4600b56e68dfc7f7e6c3ef3'
$readback = Join-Path $work 'BOTcorps-CYD-v1.2.26-readback.bin'
$stamp = Get-Date -Format 'HHmmss'
$listFile = Join-Path $work ("webui-v1226-firmware-list-" + $stamp + ".txt")
$relistFile = Join-Path $work ("webui-v1226-relist-" + $stamp + ".txt")

if (Test-Path $readback) {
  Write-Output ("READBACK_EXISTS=" + $readback)
  Write-Output 'Refusing to overwrite an existing read-back file.'
  exit 2
}

function Login-WebUI {
  $session = New-Object Microsoft.PowerShell.Commands.WebRequestSession
  $login = Invoke-WebRequest -Uri ($base + '/login') -Method POST -Body @{ username = 'admin'; password = 'launcher' } -WebSession $session -MaximumRedirection 0 -ErrorAction SilentlyContinue -UseBasicParsing -TimeoutSec 20
  $loc = $null
  if ($login) { $loc = [string]$login.Headers['Location'] }
  if ($loc -match 'failed') { throw 'login failed' }
  $cookie = $null
  foreach ($c in $session.Cookies.GetCookies($base)) {
    if ($c.Name -eq 'ESP32SESSION') { $cookie = $c.Value }
  }
  if (-not $cookie) { throw 'no ESP32SESSION cookie' }
  return @{ Session = $session; Cookie = $cookie; Location = $loc; Status = [int]$login.StatusCode }
}

function List-Firmware {
  param($Session)
  return Invoke-WebRequest -Uri ($base + '/listfiles?folder=/Firmware') -WebSession $Session -UseBasicParsing -TimeoutSec 20
}

function Target-Exists {
  param([string]$Content)
  foreach ($line in ($Content -split "`r?`n")) {
    if ($line -match ('^Fi:' + [regex]::Escape($remoteName) + ':')) { return $true }
  }
  return $false
}

Write-Output ("SETTLE_START=" + (Get-Date -Format 'HH:mm:ss'))
Start-Sleep -Seconds 1
Write-Output ("SETTLE_END=" + (Get-Date -Format 'HH:mm:ss'))

$maxTries = 3
$script:Ok = $false
for ($n = 1; $n -le $maxTries; $n++) {
  Write-Output ("UPLOAD_TRY=" + $n + " at=" + (Get-Date -Format 'HH:mm:ss'))
  try {
    $auth = Login-WebUI
    Write-Output ("LOGIN_STATUS=" + $auth.Status + " LOCATION=" + $auth.Location)
  } catch {
    Write-Output ("LOGIN_FAIL=" + $_.Exception.Message)
    if ($n -lt $maxTries) { Start-Sleep -Seconds 25; continue }
    Write-Output 'WEBUI_UNREACHABLE after retries'
    exit 3
  }

  try {
    $list = List-Firmware -Session $auth.Session
  } catch {
    Write-Output ("LIST_FAIL=" + $_.Exception.Message)
    if ($n -lt $maxTries) { Start-Sleep -Seconds 25; continue }
    Write-Output 'WEBUI_UNREACHABLE after retries'
    exit 3
  }
  Write-Output ("LIST_STATUS=" + [int]$list.StatusCode)
  Write-Output '-----FIRMWARE-----'
  Write-Output $list.Content
  Write-Output '-----END_FIRMWARE-----'
  [System.IO.File]::WriteAllText($listFile, $list.Content)

  if (Target-Exists -Content $list.Content) {
    Write-Output ("TARGET_EXISTS=/Firmware/" + $remoteName)
    Write-Output 'Refusing to overwrite. Stop.'
    exit 2
  }
  Write-Output ("TARGET_FREE=/Firmware/" + $remoteName)

  $tryBody = Join-Path $work ("webui-v1226-upload-body-" + $stamp + "-e" + $n + ".txt")
  $tryHdr = Join-Path $work ("webui-v1226-upload-hdr-" + $stamp + "-e" + $n + ".txt")
  $form = "folder=/Firmware"
  $fileField = "file=@" + $local + ";filename=" + $remoteName
  $uploadCode = & curl.exe -sS -H "Expect:" --cookie ("ESP32SESSION=" + $auth.Cookie) -D $tryHdr -o $tryBody -w "%{http_code}" --connect-timeout 20 --max-time 180 -F $form -F $fileField ($base + '/')
  $uploadText = ''
  if (Test-Path $tryBody) { $uploadText = [System.IO.File]::ReadAllText($tryBody).Trim() }
  Write-Output ("UPLOAD_HTTP=" + $uploadCode)
  Write-Output ("UPLOAD_BODY=" + $uploadText)
  if ($uploadCode -eq '200' -and $uploadText -eq 'OK') {
    Write-Output 'UPLOAD_OK'
    $script:Ok = $true
    break
  }
  Write-Output 'UPLOAD_FAILED this attempt'
  if (Test-Path $tryHdr) { Write-Output ([System.IO.File]::ReadAllText($tryHdr)) }
  if ($n -lt $maxTries) {
    Write-Output 'Waiting for WebUI to recover after reset...'
    Start-Sleep -Seconds 45
  }
}

if (-not $script:Ok) {
  Write-Output 'UPLOAD_FAILED after retries'
  exit 1
}

$auth2 = Login-WebUI
Write-Output ("READBACK_LOGIN_STATUS=" + $auth2.Status)
$listMid = List-Firmware -Session $auth2.Session
Write-Output '-----FIRMWARE_AFTER_UPLOAD-----'
Write-Output $listMid.Content
Write-Output '-----END_FIRMWARE_AFTER_UPLOAD-----'
if (-not (Target-Exists -Content $listMid.Content)) {
  Write-Output 'UPLOAD_REPORTED_OK_BUT_FILE_MISSING'
  exit 1
}

$dlUrl = $base + '/file?name=/Firmware/' + $remoteName + '&action=download'
$dlCode = & curl.exe -sS --cookie ("ESP32SESSION=" + $auth2.Cookie) -o $readback -w "%{http_code}" --connect-timeout 20 --max-time 180 $dlUrl
Write-Output ("READBACK_HTTP=" + $dlCode)
Write-Output ("READBACK_PATH=" + $readback)
if ($dlCode -ne '200' -or -not (Test-Path $readback)) {
  Write-Output 'READBACK_FAILED'
  exit 1
}
$info = Get-Item $readback
$sha = (Get-FileHash -Algorithm SHA256 -Path $readback).Hash.ToLower()
Write-Output ("READBACK_SIZE=" + $info.Length)
Write-Output ("READBACK_SHA256=" + $sha)
if ($info.Length -eq $expectSize -and $sha -eq $expectHash) {
  Write-Output 'READBACK_MATCH=YES'
} else {
  Write-Output ('READBACK_MATCH=NO expect size=' + $expectSize + ' sha=' + $expectHash)
  exit 1
}

$list2 = List-Firmware -Session $auth2.Session
Write-Output ("RELIST_STATUS=" + [int]$list2.StatusCode)
Write-Output '-----RELIST-----'
Write-Output $list2.Content
[System.IO.File]::WriteAllText($relistFile, $list2.Content)
