BITS 16
ORG 0
; Exercise short branches, counter branches, near relative CALL/RET and JMP.
mov ax, 0
cmp ax, 0
je equal
inc ax
equal:
mov cx, 3
again:
inc ax
loop again
jcxz done
inc ax
done:
call twice
jmp finish
twice:
inc ax
ret
finish:
hlt
