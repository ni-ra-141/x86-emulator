BITS 16
ORG 0
; Demonstrate byte/word MOV and XCHG register encodings.
mov ax, 1122h
mov bx, 3344h
xchg ax, bx
mov cl, 5Ah
xchg al, ah
hlt
