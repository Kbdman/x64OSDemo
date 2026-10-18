# x64 OS development learning repo

This repository will contain the code and notes for learning how to develop an OS for an x64 PC.

## Recommended starting stack

- Host OS: Linux
- Language: C first, then Rust later if desired
- Boot path: UEFI
- Bootloader: Limine
- Emulator: QEMU
- Debugging: GDB + serial output

## First milestones

1. Set up a cross-compilation toolchain
2. Boot a kernel in QEMU
3. Print a message over serial
4. Parse the boot memory map
5. Enter higher-half paging
6. Install GDT/IDT and handle interrupts
7. Add a physical memory manager
8. Add a heap allocator
9. Add a timer and simple scheduler
10. Add keyboard input and a basic console

## Suggested repo layout

- boot/ - Limine and UEFI boot files
- kernel/ - kernel source
- libs/ - small shared runtime code
- docs/ - notes and learning logs

## Goal for the first version

Boot in QEMU and print one line from the kernel through serial output.

## Current UEFI hello world path

- Build: ./build.sh
- Run: ./run.sh
- Output binary: build/esp/EFI/BOOT/BOOTX64.EFI
- Disk image (built but not required for `run.sh`): build/esp.img

Notes:

- The launcher uses a single QEMU path and streams output from the OVMF debug console at I/O port `0x402` into `build/debugcon.log`.
- `Print(L"...")` output is shown in the UEFI console when running in GUI mode, but the repo's default learning workflow uses the debug console for terminal-friendly logs.
- Terminal output is filtered to printable ASCII characters for easier reading.

## reference

https://wiki.osdev.org/GNU-EFI
