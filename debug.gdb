set pagination off
set confirm off

target remote localhost:1234
symbol-file build/main.so
b efi_main
continue
layout src
