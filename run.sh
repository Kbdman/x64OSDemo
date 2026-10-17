#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
VARS_FILE="$BUILD_DIR/OVMF_VARS_4M.fd"
MODE="${1:-gui}"
EFI_APP="$BUILD_DIR/esp/EFI/BOOT/BOOTX64.EFI"
DEBUG_LOG="$BUILD_DIR/debugcon.log"

if [[ ! -f "$EFI_APP" ]]; then
  echo "Missing $EFI_APP"
  echo "Run ./build.sh first."
  exit 1
fi

if [[ "$MODE" != "gui" && "$MODE" != "serial" ]]; then
  echo "Usage: ./run.sh [gui|serial]"
  exit 1
fi

cp /usr/share/OVMF/OVMF_VARS_4M.fd "$VARS_FILE"

# Why 3 drives:
# - OVMF_CODE_4M.fd: UEFI firmware code (read-only flash)
# - VARS_FILE: UEFI NVRAM variables (writable flash for boot entries/settings)
# - fat:rw:$BUILD_DIR/esp: EFI System Partition with BOOTX64.EFI
#
# The ESP does not directly "depend on" or reference the flash files by path.
# QEMU exposes all of them as virtual devices, then OVMF scans devices and
# loads \EFI\BOOT\BOOTX64.EFI using UEFI boot rules/protocols.

if [[ "$MODE" == "gui" ]]; then
  echo "Running in GUI mode: check the QEMU window for UEFI Print() output."
  qemu-system-x86_64 \
    -m 512M \
    # 1) OVMF firmware code (UEFI firmware binary, read-only)
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    # 2) OVMF NVRAM/variables store (BootOrder, UEFI vars, etc.), writable per run
    -drive if=pflash,format=raw,file="$VARS_FILE" \
    # 3) Emulated EFI System Partition (our app files under EFI/BOOT)
    -drive format=raw,file=fat:rw:"$BUILD_DIR/esp"
else
  echo "Running in serial mode: GUI stays enabled, terminal streams debugcon logs."
  : > "$DEBUG_LOG"

  qemu-system-x86_64 \
    -m 512M \
    # Keep GUI active so UEFI console output still appears in QEMU window.
    -display gtk \
    # Disable QEMU monitor and legacy serial redirection.
    -monitor none \
    -serial none \
    # Capture debug I/O port writes (debugcon) into a log file.
    -debugcon file:"$DEBUG_LOG" \
    # Same 3-drive setup as GUI mode: firmware code, NVRAM vars, and ESP content.
    -drive if=pflash,format=raw,readonly=on,file=/usr/share/OVMF/OVMF_CODE_4M.fd \
    -drive if=pflash,format=raw,file="$VARS_FILE" \
    -drive format=raw,file=fat:rw:"$BUILD_DIR/esp" &

  QEMU_PID=$!

  cleanup() {
    kill "$QEMU_PID" 2>/dev/null || true
  }
  trap cleanup INT TERM

  tail --pid="$QEMU_PID" -n +1 -f "$DEBUG_LOG" | LC_ALL=C tr -cd '\11\12\15\40-\176'
  wait "$QEMU_PID"
fi
