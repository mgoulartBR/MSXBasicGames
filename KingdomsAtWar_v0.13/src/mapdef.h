/* World layers in VRAM (stride 128 bytes per row, so maps up to 128x128 fit). 0x66000..0x7A000 is free VRAM (above the raw
 * enemy tiles, below the layer A names). */
#ifndef KAW_MAPDEF_H
#define KAW_MAPDEF_H
#define MAP_T 0x66000UL
#define MAP_ART 0x6A000UL
#define MAP_STOCK 0x6E000UL
#define MAP_FOGM 0x76000UL
#define MAP_FOGX 0x72000UL /* explored flag */
#endif
