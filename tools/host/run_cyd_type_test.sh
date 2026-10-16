#!/bin/sh
# Host typing gate for the CYD keyboard. Fails nonzero.
# Does not need PlatformIO or a device.
set -eu
root=$(CDPATH= cd -- "$(dirname "$0")/../.." && pwd)
g++ -std=gnu++17 -I "$root/src" "$root/tools/host/cyd_osk_test.cpp" -o /tmp/cyd_osk_test
g++ -std=gnu++17 -I "$root/src" "$root/tools/host/cyd_type_test.cpp" -o /tmp/cyd_type_test
/tmp/cyd_osk_test
/tmp/cyd_type_test
echo "host cyd type gate passed"
