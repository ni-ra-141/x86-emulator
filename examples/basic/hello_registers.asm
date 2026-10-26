BITS 16
ORG 0
; Basic immediate loads and 16-bit register increment/decrement.
nop
mov ax, 1234h
inc ax
mov cx, 0100h
dec cx
mov bx, 0
hlt
