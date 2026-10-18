/* teste do modelo de desempenho no Z80 (SDCC): SRC=pt_dbg.c ./build.sh; resultados em 0xF000.. (lidos pelo openMSX) */
#include <stdint.h>
typedef uint8_t u8; typedef uint16_t u16; typedef uint32_t u32; typedef int8_t i8; typedef int16_t i16; typedef int32_t i32;
#include "cars.h"
static u16 pt_ratio[10], pt_hi0[10], pt_hi1[10], pt_acc[30], pt_redline[2]; static u8 pt_ngears[2], pt_auto[2];
#include "ptmodel.h"
volatile __at(0xF000) u16 out[16];
u8 g_dry, g_trig, g_row6, g_row7, g_st0, g_st1, g_t0, g_t1, g_t3;
void main(void) {
    pt_build(0, 8, 2, 3, 4, 2, 2, 0, 0, 0);
    out[0] = pt_ratio[0]; out[1] = pt_ratio[1]; out[2] = pt_ratio[2]; out[3] = pt_ratio[3]; out[4] = pt_ratio[4];
    out[5] = pt_acc[3]; out[6] = pt_acc[4]; out[7] = pt_acc[5]; out[8] = pt_acc[6]; out[9] = pt_acc[7];
    out[10] = pt_hi0[1]; out[11] = pt_hi1[1]; out[12] = pt_ngears[0]; out[13] = pt_redline[0]; out[14] = pt_auto[0];
    out[15] = 0xBEEF;
    for (;;) ;
}
