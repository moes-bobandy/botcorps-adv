#!/usr/bin/env bash
# Upload BOTcorps-CYD-v1.2.19.bin to the CYD Launcher SD /Firmware folder.
# Does NOT flash. Run on the BOTcorps PC once it is online and the Launcher WebUI is up.
# IPs tried in order: 10.0.4.132 then 192.168.1.67. Login: admin / launcher (fresh per request).
set -euo pipefail

BIN="${1:-/workspace/out/fw-bigbot-1117/v1.2.19/BOTcorps-CYD-v1.2.19.bin}"
EXPECT_SHA="${EXPECT_SHA:-de45a1202c6a8d27d7347520300d9001f355333c44e59ec3c05083750dbbd4a2}"
USER="${LAUNCHER_USER:-admin}"
PASS="${LAUNCHER_PASS:-launcher}"
FOLDER="${LAUNCHER_FOLDER:-/Firmware}"
NAME="$(basename "$BIN")"
IPS=( "${LAUNCHER_IP:-}" 10.0.4.132 192.168.1.67 )

if [[ ! -f "$BIN" ]]; then
  echo "missing bin: $BIN" >&2
  exit 1
fi
HAVE=$(sha256sum "$BIN" | awk '{print $1}')
if [[ "$HAVE" != "$EXPECT_SHA" ]]; then
  echo "local hash mismatch: $HAVE != $EXPECT_SHA" >&2
  exit 2
fi

# Stage a copy in the PC temp dir when this runs on Windows (Git Bash / MSYS).
if [[ -n "${TEMP:-}${TMP:-}" ]]; then
  STAGE="${TEMP:-${TMP}}"
  STAGE="${STAGE}//${NAME}"
  STAGE="${STAGE//\\//}"
  cp -a "$BIN" "$STAGE"
  echo "staged $STAGE"
  BIN="$STAGE"
fi

curl_auth() {
  # fresh auth on every request, as required
  curl -sS -f --connect-timeout 5 --max-time 120 -u "${USER}:${PASS}" "$@"
}

upload_one() {
  local ip="$1"
  local base="http://${ip}"
  echo "trying Launcher at $base ..."
  curl_auth -F "folder=${FOLDER}" -F "file=@${BIN};filename=${NAME}" \
    "${base}/upload" >/tmp/launcher_upload_resp.txt
  echo "upload posted, fetching ${FOLDER}/${NAME} for readback"
  curl_auth -o /tmp/launcher_readback.bin "${base}${FOLDER}/${NAME}" \
    || curl_auth -o /tmp/launcher_readback.bin "${base}/download?folder=${FOLDER}&file=${NAME}" \
    || curl_auth -o /tmp/launcher_readback.bin "${base}/file?path=${FOLDER}/${NAME}"
  local back
  back=$(sha256sum /tmp/launcher_readback.bin | awk '{print $1}')
  echo "readback sha256 $back"
  if [[ "$back" != "$EXPECT_SHA" ]]; then
    echo "READBACK HASH MISMATCH" >&2
    return 3
  fi
  echo "OK: ${FOLDER}/${NAME} on ${ip} matches $EXPECT_SHA"
  return 0
}

tried=0
for ip in "${IPS[@]}"; do
  [[ -z "$ip" ]] && continue
  tried=1
  if upload_one "$ip"; then
    exit 0
  fi
done
echo "upload pending: no Launcher WebUI answered (tried: ${IPS[*]})" >&2
exit 4
