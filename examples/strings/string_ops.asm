BITS 16
ORG 0
; Copy and scan small initialized data using supported string instructions.
mov byte [0200h], 11h
mov byte [0201h], 22h
mov si, 0200h
mov di, 0300h
mov cx, 2
cld
rep movsb
mov al, 22h
mov di, 0300h
mov cx, 2
repne scasb
mov si, 0300h
lodsb
mov ax, 1234h
mov di, 0310h
stosw
mov si, 0300h
mov di, 0300h
cmpsb
hlt
