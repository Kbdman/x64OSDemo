#!/bin/bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "$0")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
ESP_DIR="$BUILD_DIR/esp"
EFI_DIR="$ESP_DIR/EFI/BOOT"

mkdir -p "$EFI_DIR"
# compile the main.c
gcc \
	-g -O0 \
	-I/usr/include/efi \
	-I/usr/include/efi/x86_64 \
	-ffreestanding \
	-fshort-wchar \
	-fno-stack-protector \
	-fpic \
	-mno-red-zone \
	-maccumulate-outgoing-args \
	-c "$ROOT_DIR/osloader/main.c" \
	-o "$BUILD_DIR/osloader.o"
# link the osloader.o with uefi libraries
ld \
	-nostdlib \
	-znocombreloc \
	-T /usr/lib/elf_x86_64_efi.lds \
	-shared \
	-Bsymbolic \
	-L /usr/lib \
	/usr/lib/crt0-efi-x86_64.o \
	"$BUILD_DIR/osloader.o" \
	-o "$BUILD_DIR/osloader.so" \
	-lefi \
	-lgnuefi
# convert the linked shared object to a UEFI executable
objcopy \
	--subsystem 10 \
	-O pei-x86-64 \
	-j .text \
	-j .rodata \
	-j .sdata \
	-j .data \
	-j .dynamic \
	-j .dynsym \
	-j .rel \
	-j .rela \
	-j .reloc \
	"$BUILD_DIR/osloader.so" \
	"$EFI_DIR/BOOTX64.EFI"

cat > "$ESP_DIR/startup.nsh" <<'NSH'
fs0:\EFI\BOOT\BOOTX64.EFI
NSH
as "$ROOT_DIR/src/start.s" -g -o "$BUILD_DIR/start.o"
ld "$BUILD_DIR/start.o" -nostdlib -T "$ROOT_DIR/src/kernel.ld" -o "$BUILD_DIR/kernel.elf"

KERNEL_BIN="$ESP_DIR/kernel.bin"
objcopy \
	-O binary \
	"$BUILD_DIR/kernel.elf" \
	"$KERNEL_BIN"


# create a disk image and copy the UEFI executable and startup script into it
truncate -s 64M "$BUILD_DIR/esp.img"
mkfs.vfat -F 32 "$BUILD_DIR/esp.img"
mmd -i "$BUILD_DIR/esp.img" ::/EFI ::/EFI/BOOT
mcopy -i "$BUILD_DIR/esp.img" "$EFI_DIR/BOOTX64.EFI" ::/EFI/BOOT/BOOTX64.EFI
mcopy -i "$BUILD_DIR/esp.img" "$ESP_DIR/startup.nsh" ::/startup.nsh
mcopy -i "$BUILD_DIR/esp.img" "$KERNEL_BIN" ::/kernel.bin

echo "Built UEFI application: $EFI_DIR/BOOTX64.EFI"
echo "Disk image: $BUILD_DIR/esp.img"
