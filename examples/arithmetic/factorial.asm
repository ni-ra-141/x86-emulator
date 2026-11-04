BITS 16
ORG 0
; Compute 5! using the 16-bit MUL form. Result: AX = 120 (0078h).
mov ax, 1
mov bx, 0
mov cx, 5

factorial_loop:
    inc bx
    mul bx              ; DX:AX = AX * BX
    loop factorial_loop

hlt
