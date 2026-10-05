// Portable fixed-width types: MSXgl's own when building for the Z80, <stdint.h> on the host
// (the pure game-logic modules are compiled natively by tests/ for regression testing).
#pragma once
#ifdef __SDCC
	#include "core.h"
	#define BANKED __banked     /* function lives in a mapper code segment (bank 2) */
#else
	#define BANKED
	#include <stdint.h>
	#include <stdbool.h>
	#include <stddef.h>
	typedef uint8_t  u8;
	typedef int8_t   i8;
	typedef uint16_t u16;
	typedef int16_t  i16;
	typedef uint32_t u32;
	typedef int32_t  i32;
	typedef char     c8;
	#ifndef TRUE
		#define TRUE  1
		#define FALSE 0
	#endif
#endif
