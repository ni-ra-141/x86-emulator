BITS 16
ORG 0
; Check whether the five-byte string "LEVEL" is a palindrome.
; AX = 1 when it is a palindrome, AX = 0 otherwise.
mov byte [0200h], 'L'
mov byte [0201h], 'E'
mov byte [0202h], 'V'
mov byte [0203h], 'E'
mov byte [0204h], 'L'

mov si, 0200h
mov di, 0204h
mov cx, 2               ; only the outer two pairs need comparison
palindrome_loop:
    mov al, [si]
    mov bl, [di]
    cmp al, bl
    jne not_palindrome
    inc si
    dec di
    loop palindrome_loop
    mov ax, 1
    jmp finished

not_palindrome:
    mov ax, 0
    jmp finished

finished:
    hlt
