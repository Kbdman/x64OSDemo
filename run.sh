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
echo "Running EFI app with COM1 on stdio..."


# Why 3 drives:
# - OVMF_CODE_4M.fd: UEFI firmware code (read-only flash)
# - VARS_FILE: UEFI NVRAM variables (writable flash for boot entries/settings)
# - fat:rw:$BUILD_DIR/esp: EFI System Partition with BOOTX64.EFI
#
# The ESP does not directly "depend on" or reference the flash files by path.
# QEMU exposes all of them as virtual devices, then OVMF scans devices and loads
# \EFI\BOOT\BOOTX64.EFI using UEFI boot rules/protocols.

qemu-system-x86_64 \
  -m 512M \
  -display none \
  -serial stdio \
  -monitor telnet:127.0.0.1:5555,server,nowait \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file="$VARS_FILE" \
  -drive format=raw,file=fat:rw:"$BUILD_DIR/esp"
