#include "msx_seg.h"

// Marker bytes defined at the start of each data segment (src/data/seg_s*_b3.c).
// Read through their link addresses (inside the A000h window), not a fixed address.




u8 Seg_SelfTest(void)
{
	static const u8* const magic[3] = { g_SegMagic4, g_SegMagic5, g_SegMagic6 };
	for (u8 i = 0; i < 3; ++i)
	{
		const volatile u8* m = magic[i];
		SET_BANK_SEGMENT(SEG_WINDOW_BANK, SEG_NEWS + i);
		if (m[0] != 0x52 || m[1] != (u8)('4' + i))
			return SEG_NEWS + i;
	}
	SET_BANK_SEGMENT(SEG_WINDOW_BANK, 3);
	return 0;
}
