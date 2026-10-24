set pagination off
set confirm off

target remote localhost:1234

# Choose ONE target:
# 1) raw kernel loaded at 0x100000
add-symbol-file build/kernel.elf -s .text.start 0x100000 -s .text 0x101000 -s .data 0x102000
b _start

# 2) UEFI app loaded by OVMF (if you are debugging the EFI binary instead)
# add-symbol-file build/osloader.so -s .text 0x1df8b000 -s .data 0x1df93000 -s .rodata 0x1df97000
# b efi_main

continue
