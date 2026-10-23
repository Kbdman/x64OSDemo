set pagination off
set confirm off

target remote localhost:1234

# load the ELF symbols
# address is computed from imagebase and vma of each section
add-symbol-file build/osloader.so -s .text 0x1df8b000 \
  -s .data   0x1df93000 \
  -s .rodata 0x1df97000

add-symbol-file build/kernel.elf -s .text.start 0x100000
b efi_main
b *0x100000
