// Shim para compilar sim.c no PC (autoplay/balanceamento): so os tipos usados pela logica do jogo.
#pragma once
#include <stdint.h>
typedef uint8_t u8; typedef int8_t i8; typedef uint16_t u16; typedef int16_t i16; typedef uint32_t u32; typedef int32_t i32;
#define GET_BANK_SEGMENT(b) 3
#define __banked
#define SET_BANK_SEGMENT(b, s) ((void)0)
