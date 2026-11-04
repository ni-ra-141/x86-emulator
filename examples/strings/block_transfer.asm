BITS 16
ORG 0
; Copy five bytes from DS:0200h to ES:0300h using REP MOVSB.
; Expected destination bytes at ES:0300h..0304h: 11h,22h,33h,44h,55h.
mov byte [0200h], 11h
mov byte [0201h], 22h
mov byte [0202h], 33h
mov byte [0203h], 44h
mov byte [0204h], 55h

mov si, 0200h
mov di, 0300h
mov cx, 5
cld
rep movsb
hlt
