# x64 OS development learning repo

This repository is a small UEFI-based x64 OS learning project. The current focus is understanding how an x64 machine boots under UEFI.
## Recommended starting stack

- Host OS: Linux
- Language: C + assembly
- Boot path: UEFI
- Emulator: QEMU
- Debugging: GDB + OVMF debug console

## Current project status

This repo currently builds two artifacts:

- A UEFI application at `build/esp/EFI/BOOT/BOOTX64.EFI`
- A flat kernel stub at `build/esp/kernel.bin`


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

- `src/` - kernel and startup assembly
- `osloader/` - UEFI bootloader/loader code
- `build/` - generated binaries and disk images
- `docs/` - notes and learning logs

## Build and run

```bash
./build.sh
./run.sh
```

Artifacts:

- EFI binary: `build/esp/EFI/BOOT/BOOTX64.EFI`
- flat kernel stub: `build/esp/kernel.bin`
- disk image: `build/esp.img`
- debug log: `build/debugcon.log`

## Notes

- The UEFI program uses QEMU's OVMF debug console at I/O port `0x402` and writes to `build/debugcon.log`.
- `Print(L"...")` output is visible in the UEFI console when running in GUI mode, but the repo's default workflow uses the debug console for terminal-friendly logging.
- Terminal output is filtered to printable ASCII characters for easier reading.

## Reference

https://wiki.osdev.org/GNU-EFI
