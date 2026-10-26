BITS 16
ORG 0
; Demonstrate register, memory and segment stack operations plus flags stack.
mov ax, 1234h
mov bx, 5678h
push ax
push bx
pop cx
pop dx
mov [0200h], ax
push word [0200h]
pop si
push ds
pop es
pushf
popf
hlt
