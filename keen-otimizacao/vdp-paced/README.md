Experimento (não incluído nas ROMs): escrita na VRAM com ritmo (`VP`, em `keen_msx_s21_b1.c`) usada nos
uploads do herói (`HeroData*`, `HeroCompact*`, `s105`), nos bitmaps de textura (`brocco_bitmap.c`, `cart_draw.c`,
`s100/s101`) e nos atributos de sprite (`HWUpload`). Substitui `VDP_WriteVRAM` (OTIR, 21 T/byte) por
`outi + nop + jr` (32 T/byte). Arquivos aqui: `keen_msx_s21_b1.c` (com `VP`), `keen_msx_s22_b1.c`, `keen_msx_s105_b1.c`,
`brocco_bitmap.c` (exemplos já com `VP(`); nos demais basta trocar `VDP_WriteVRAM(` por `VP(` e declarar
`void VP(const u8* src,u16 dst,u8 page,u16 n) __banked;`.
