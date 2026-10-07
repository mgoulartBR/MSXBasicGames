; s16 mulq14(s16 a, s16 b): (a*b)>>14 por soma e deslocamento (sinal-magnitude, trunca para zero).
; Substitui (s32)a*b do SDCC 4.2.0, que errou com multiplicador negativo. sdcccall(1): a=HL, b=DE, retorno=DE.
 .module math
 .globl _mulq14
 .area _CODE
_mulq14:
 push bc
 ld a,h
 xor d
 push af                ; bit 7 de A/flag S = sinal do resultado
 bit 7,h
 jr z,mq_a
 ld a,l
 cpl
 ld l,a
 ld a,h
 cpl
 ld h,a
 inc hl
mq_a:
 bit 7,d
 jr z,mq_b
 ld a,e
 cpl
 ld e,a
 ld a,d
 cpl
 ld d,a
 inc de
mq_b:
 ld b,h
 ld c,l                 ; BC = |a|
 ld hl,#0               ; HL:DE = produto de 32 bits (DE = multiplicador, vira a parte baixa)
 ld a,#16
mq_loop:
 bit 0,e
 jr z,mq_skip
 add hl,bc
 jr mq_shift
mq_skip:
 or a
mq_shift:
 rr h
 rr l
 rr d
 rr e
 dec a
 jr nz,mq_loop
 ld a,d                 ; resultado = (HL<<2) | (D>>6)
 rlca
 rlca
 and #3
 ld b,a
 add hl,hl
 add hl,hl
 ld a,l
 or b
 ld l,a
 pop af
 jp p,mq_pos
 ld a,l
 cpl
 ld l,a
 ld a,h
 cpl
 ld h,a
 inc hl
mq_pos:
 ex de,hl
 pop bc
 ret
