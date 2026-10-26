BITS 16
ORG 0
; Exercise carry/direction/interrupt controls and LAHF/SAHF flag transfer.
clc
stc
cmc
cli
sti
cld
std
mov ah, 0D5h
sahf
lahf
clc
cmp al, 0
lahf
hlt
