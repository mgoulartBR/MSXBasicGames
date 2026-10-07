 .module crt
 .globl _main
 .area _HEADER (ABS)
 .org 0x4000
 di
 ld sp,#0xF380
 call _main
 .area _CODE
 .area _HOME
 .area _DATA
