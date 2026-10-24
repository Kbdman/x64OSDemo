.intel_syntax noprefix
.global _start
.section .text.start
_start:
    
    mov al,[inited]
    test al,al
    jne _inited
    call init_serial
    movb [inited],1
 _inited:   
    jmp _start

.section .data
inited:
 .byte 0
