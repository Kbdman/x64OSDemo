# x64 OS development learning repo

This repository is a small UEFI-based x64 OS learning project. The current focus is understanding how an x64 machine boots under UEFI.
## Recommended starting stack

- Host OS: Linux
- Language: C + assembly
- Boot path: UEFI
- Emulator: QEMU
- Debugging: GDB + COM1 serial output

## Current project status

This repo currently builds two artifacts:

- A UEFI application at `build/esp/EFI/BOOT/BOOTX64.EFI`
- A flat kernel stub at `build/esp/kernel.bin`

The kernel has an assembly entry point, a C entry function, serial output,
and GDT inspection code.

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

## Repo layout

- `src/` - kernel and startup assembly
- `include/` - kernel headers shared between modules
- `boot_info/` - shared loader-kernel boot information interface
- `osloader/` - UEFI bootloader/loader code
- `build/` - generated binaries and disk images

## Build and run

Requires CMake 3.20+, GCC, binutils, GNU-EFI, dosfstools, mtools,
QEMU, and OVMF. The scripts expect GNU-EFI under `/usr` and firmware under
`/usr/share/OVMF`.

```bash
./build.sh
./run.sh
```

`build.sh` builds the kernel with CMake, generates the flat binary with
`objcopy`, then builds the UEFI loader and packages the ESP disk image.
`run.sh` boots headless with COM1 connected to the terminal.

Artifacts:

- EFI binary: `build/esp/EFI/BOOT/BOOTX64.EFI`
- flat kernel stub: `build/esp/kernel.bin`
- kernel symbols for GDB: `build/kernel.elf`
- disk image: `build/esp.img`

## Debugging

From the repository root, start QEMU paused:

```bash
./debug.sh
```

In another terminal, attach GDB:

```bash
gdb -q -x debug.gdb
```

Use `continue` to resume if GDB is stopped. The QEMU GDB server uses port
`1234`; the QEMU monitor is available via telnet at `127.0.0.1:5555`.

## Notes

- Kernel serial output uses COM1 (`0x3F8`), routed to the terminal with
  QEMU's `-serial stdio`.
- Firmware debugcon output at `0x402` is separate from COM1 and is not captured
  by the current launch scripts.

## Reference

https://wiki.osdev.org/GNU-EFI
