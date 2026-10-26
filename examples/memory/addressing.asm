BITS 16
ORG 0
; Exercise direct, moffs, base/index, displacement, BP and LEA forms.
mov word [0200h], 1234h
mov ax, [0200h]
mov [0202h], ax
mov bx, 0200h
mov si, 2
mov cx, [bx+si]
mov bp, 0200h
mov word [bp+di+2], 5678h
mov dx, [bp+di+2]
mov ax, [si+0200h]
lea dx, [bx+si+4]
mov [0204h], al
mov al, [0204h]
; Remaining ModR/M effective-address combinations, with safe initialized regs.
mov di, 4
mov ax, 0A55Ah
mov [bx+di], ax
mov ax, [bx+di]
mov [bp+si], ax
mov ax, [bp+si]
mov [di], ax
mov ax, [di]
mov [bp], ax
mov ax, [bp]
mov [bx], ax
mov ax, [bx]
; Segment-register moves use DS/ES (all segments start at the same value).
mov ax, ds
mov es, ax
hlt
