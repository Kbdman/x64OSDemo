set pagination off
set confirm off

# The kernel is linked at its runtime load address.
file build/kernel.elf
target remote localhost:1234
break entry

# 2) UEFI app loaded by OVMF (if you are debugging the EFI binary instead)
# add-symbol-file build/osloader.so -s .text 0x1df8b000 -s .data 0x1df93000 -s .rodata 0x1df97000
# b efi_main
