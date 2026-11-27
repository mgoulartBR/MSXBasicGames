/* V9990 P1 VRAM layout constants shared by the renderer (video.h) and the boot loader (bank 54). */
#ifndef KAW_VDEFS_H
#define KAW_VDEFS_H
#define P1_SGT 0x38000UL
#define P1_PGT_B_BASE 0x40000UL
#define P1_SGT_LINE 1792U
#define POSE32_Y 240U
#define POSE48_Y 688U
#define PB_PLAIN 8U
#define PB_DITH 776U
#define ENEMY_PAT 1544U /* enemy-kingdom building tiles, after the dithered block; no dithered copy (remembered = same pattern) */
#define EOV_Y 3072U /* VRAM line (0x60000) of the raw enemy tiles for occlusion, 16 per row */
#define OCC_ENEMY 141 /* occluder cache key = OCC_ENEMY + tile index (>= 1); human keys are at most 141, 0xFFFF is 'empty' */
#define PA_SOLID 641U
#define PA_FONT 672U
#define PA_HUDTOP 660U
#define PNT_A 0x7C000UL
#define PNT_B 0x7E000UL
#define SPAT 0x3FE00UL
#define OV_Y 1648U
#define CELL_BAR_ALLY 192   /* 192..200 */
#define CELL_ARROW 201      /* 201..204 */
#define CELL_CROSS 205
#define CELL_PTR 206
#define CELL_RING 207
#define CELL_BAR_ENEMY 212  /* 212..220 */
#define CELL_BAR_WHITE 228  /* 228..236 */
#define CELL_VLINE15 191  /* one-pixel vertical line, 16 rows: white (drag box) */
#define CELL_VLINE10 175  /* placement ghost, valid */
#define CELL_VLINE7 159   /* placement ghost, blocked */
#define OCC_N 14
#endif
