/* Modelo de desempenho do Street Rod (porte de tools/powertrain.py para aritmetica inteira).
 * Entrada: variantes das pecas, desgaste do motor (0..10000), ajuste do ponto (-8..8) e danos; saida: arrays pt_* do veiculo `slot`.
 * Unidades internas: "acc" e "B" em 1/10000, A em 1/100. Requer pm_* (cars.h) e os arrays pt_* do jogo. */
#ifndef PTMODEL_H
#define PTMODEL_H

static i16 pt_i16abs(i16 v) { return v < 0 ? -v : v; }

/* B (qualidade) em 1/10000. (Escrito em passos simples: o SDCC gerava codigo errado para a expressao composta com ternario/i32.) */
static i32 pt_quality(u8 perf, u8 engine, u8 trans, u8 manifold, u8 carb, u16 wear, i8 trim, u8 dmg) {
    u16 acc = 0, wl;
    u32 r;
    if (dmg & 1) acc += 900;
    if (dmg & 2) acc += 500;
    if (dmg & 4) acc += 100;
    acc += (u16)((u32)pm_body[perf] * 1000 / 88);
    acc += (u16)pm_combo[5 * carb + manifold] * 15;
    acc += (u16)pm_eng[engine] * 25;
    acc += (u16)pm_trans[trans] * 15;
    acc += (u16)(8 - pt_i16abs(trim)) * 250;
    wl = 100 - wear / 100;
    r = (u32)acc * wl;
    r = r / 100;                                    /* acc * (10000 - desgaste) / 10000 */
    if (trans == 0) r = r * 24 / 100; else r = r * 3 / 10;
    return 7000 + (i32)r;
}
/* potencia A em 1/100 (maximo 130,00) */
static u16 pt_power(u8 perf, u8 engine, u8 trans, u8 manifold, u8 carb, u16 wear, i8 trim, u8 dmg) {
    i32 a = pt_quality(perf, engine, trans, manifold, carb, wear, trim, dmg) * 13 / 10;
    return (u16)(a > 13000 ? 13000 : a);
}
static void pt_build(u8 slot, u8 perf, u8 engine, u8 trans, u8 manifold, u8 carb, u8 tires, u16 wear, i8 trim, u8 dmg) {
    i32 B = pt_quality(perf, engine, trans, manifold, carb, wear, trim, dmg);
    i32 A = B * 13 / 10;
    u8 g, o = slot * 5;
    if (A > 13000) A = 13000;
    for (g = 0; g < 5; g++) {
        i32 t = pm_ratio[5 * trans + g], v, q;
        i32 a0;
        pt_ratio[o + g] = t ? (u16)(57000000L / (t * A)) : 28500;
        v = (i32)pm_accel[5 * engine + g] * B / 10000;
        q = g == 0 ? v : v / 5;
        a0 = q * (g == 1 ? (tires == 2 ? 3 : 2) : 1);
        pt_hi0[o + g] = pm_rpm[2 * engine];
        pt_hi1[o + g] = pm_rpm[2 * engine + 1];
        pt_acc[(o + g) * 3 + 0] = (u16)a0;
        pt_acc[(o + g) * 3 + 1] = (u16)v;
        pt_acc[(o + g) * 3 + 2] = (u16)(g ? v / 10 : v);
    }
    pt_ngears[slot] = (u8)pm_gears[trans];
    pt_redline[slot] = (u16)pm_redline[engine];
    pt_auto[slot] = trans == 0;
}
#endif
