// Reading the ROM blob (8 KB segments seen through the 0xA000 window) and LZSS image loading.
#pragma once
#include "msxgl.h"

void Blob_Open(u8 seg, u16 off);          // seg is relative to BLOB_FIRST_SEG
u8   Blob_Byte(void);
// Image container: [w bytes][h][flags: bit0 = 32-byte palette follows][xb][y][palette][LZSS stream of h rows of w bytes].
// It carries its own destination (byte column xb, line y of page 0); yoff shifts it to another page and
// mirror=1 also writes the same rows to page 1.
void Blob_LoadImage(u8 seg, u16 off, u16 yoff, u8 mirror);
void Blob_SetPalette(u8 seg, u16 off);    // 16 x [RB, G]
