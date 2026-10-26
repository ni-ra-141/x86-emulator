BITS 16
ORG 0
; Demonstrate logical/unary operations and supported shift/rotate count forms.
mov ax, 00F3h
and ax, 00FFh
or ax, 0004h
xor ax, 00F1h
test ax, 0006h
mov bx, 00F0h
not bx
not bx
neg bx
neg bx
mov cl, 1
shl bx, 1
shr bx, cl
rol bx, 1
ror bx, cl
hlt
