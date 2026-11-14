#ifndef MSXGL_H
#define MSXGL_H
#include <stdint.h>
// Match byte-aligned SDCC struct layout when testing metadata overlays.
#pragma pack(1)
#define __banked
typedef uint32_t u32;
typedef uint8_t u8;typedef int8_t i8;typedef uint16_t u16;typedef int16_t i16;
#endif
