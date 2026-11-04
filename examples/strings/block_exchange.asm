BITS 16
ORG 0
; Swap two three-byte blocks at DS:0200h and DS:0300h.
; Expected: [0200h..0202h] = 44h,55h,66h;
;          [0300h..0302h] = 11h,22h,33h.
mov byte [0200h], 11h
mov byte [0201h], 22h
mov byte [0202h], 33h
mov byte [0300h], 44h
mov byte [0301h], 55h
mov byte [0302h], 66h

mov si, 0200h
mov di, 0300h
mov cx, 3

exchange_loop:
    mov al, [si]
    mov bl, [di]
    mov [si], bl
    mov [di], al
    inc si
    inc di
    loop exchange_loop

hlt
