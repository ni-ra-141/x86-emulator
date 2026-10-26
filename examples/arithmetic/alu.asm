BITS 16
ORG 0
; Exercise arithmetic directions, carry, compare, conversion and unsigned MUL/DIV.
mov ax, 5
mov bx, 2
add ax, bx
stc
adc ax, 1
sub ax, 3
clc
sbb ax, 1
cmp ax, 4
inc bx
dec bx
mov al, 7
mov bl, 3
mul bl
mov bl, 3
div bl
mov al, 80h
cbw
cwd
hlt
