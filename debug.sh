#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VARS_FILE="$BUILD_DIR/OVMF_VARS_4M.fd"
EFI_APP="$BUILD_DIR/esp/EFI/BOOT/BOOTX64.EFI"

if [[ ! -f "$EFI_APP" ]]; then
  echo "Missing $EFI_APP"
  echo "Run ./build.sh first."
  exit 1
fi

cp /usr/share/OVMF/OVMF_VARS_4M.fd "$VARS_FILE"

echo "QEMU is waiting for GDB on localhost:1234"
echo "Run: gdb -q -x debug.gdb"

qemu-system-x86_64 \
  -m 512M \
  -S -s \
  -serial stdio \
  -display none \
  -monitor telnet:127.0.0.1:5555,server,nowait \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file="$VARS_FILE" \
  -drive format=raw,file=fat:rw:"$BUILD_DIR/esp"
