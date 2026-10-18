#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VARS_FILE="$BUILD_DIR/OVMF_VARS_4M.fd"
EFI_APP="$BUILD_DIR/esp/EFI/BOOT/BOOTX64.EFI"
DEBUG_LOG="$BUILD_DIR/debugcon.log"

if [[ ! -f "$EFI_APP" ]]; then
  echo "Missing $EFI_APP"
  echo "Run ./build.sh first."
  exit 1
fi

cp /usr/share/OVMF/OVMF_VARS_4M.fd "$VARS_FILE"

# We keep one default run path for this repo: boot the EFI app in QEMU and stream
# the log from the OVMF debug console to the terminal. We do not distinguish a
# separate GUI mode here because the debug console path is enough for learning and
# debugging while keeping the launch simple.

echo "Running EFI app with QEMU debug console output..."
: > "$DEBUG_LOG"

DISPLAY_MODE="${QEMU_DISPLAY:-gtk}"

# Why 3 drives:
# - OVMF_CODE_4M.fd: UEFI firmware code (read-only flash)
# - VARS_FILE: UEFI NVRAM variables (writable flash for boot entries/settings)
# - fat:rw:$BUILD_DIR/esp: EFI System Partition with BOOTX64.EFI
#
# The ESP does not directly "depend on" or reference the flash files by path.
# QEMU exposes all of them as virtual devices, then OVMF scans devices and loads
# \EFI\BOOT\BOOTX64.EFI using UEFI boot rules/protocols.

echo "Using QEMU display mode: $DISPLAY_MODE"
qemu-system-x86_64 \
  -m 512M \
  -display "$DISPLAY_MODE" \
  -global isa-debugcon.iobase=0x402 \
  -debugcon file:"$DEBUG_LOG" \
  -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
  -drive if=pflash,format=raw,file="$VARS_FILE" \
  -drive format=raw,file=fat:rw:"$BUILD_DIR/esp" &

QEMU_PID=$!

cleanup() {
  kill "$QEMU_PID" 2>/dev/null || true
}
trap cleanup INT TERM

echo "Debug log: $DEBUG_LOG"
echo "Waiting for output from the EFI app..."
tail --pid="$QEMU_PID" -n +1 -f "$DEBUG_LOG" | LC_ALL=C tr -cd '\11\12\15\40-\176'
wait "$QEMU_PID"
