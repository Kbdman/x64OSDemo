.intel_syntax noprefix
.section .text
.global outport
.type outport, @function
outport:
    mov dx, di
    mov al, sil
    out dx, al
    ret

.global inport
.type inport, @function
inport:
    mov dx, di
    in al, dx
    ret
