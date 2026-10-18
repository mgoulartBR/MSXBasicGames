/* Prototipo da estrada pseudo-3D de Street Rod para MSX2 (Screen 5, V9938).
 * Mede o custo de ~23 linhas de projeção por frame desenhadas com HMMV. */
#include <stdint.h>
#define BK __banked       /* funcao chamada de outro banco de codigo (ver bankify.py) */
#include "tables.h"
#include "assets.h"
#include "cars.h"

#ifndef STEP_SH
#define STEP_SH 2       /* log2 de scanlines por grupo de retangulos da pista */
#endif
#define STEP (1 << STEP_SH)
#ifndef VSYNC
#define VSYNC 1         /* 1 = troca de pagina sincronizada com o vblank */
#endif
#ifndef KERB_MERGE
#define KERB_MERGE 1    /* 1 = zebra desenhada como um retangulo largo sob a pista */
#endif
#ifndef VIEW_BOT
#define VIEW_BOT 95     /* altura da janela da estrada */
#endif
#ifndef N_ROWS
#define N_ROWS 15       /* linhas projetadas (original: 14 distantes + proximas) */
#endif
#ifndef DRAW_MODE
#define DRAW_MODE 2     /* bit0: zebras separadas, bit1: gramado por scanline (cada pixel pintado 1x) */
#endif
#ifndef VIEW_X0
#define VIEW_X0 0
#endif
#ifndef VIEW_X1
#define VIEW_X1 256
#endif
#define CMD_MAX 700

typedef uint32_t u32; typedef int8_t i8; typedef int16_t i16; typedef uint16_t u16; typedef uint8_t u8; typedef int32_t i32;

/* variaveis lidas pelo script de teste (enderecos fixos) */
volatile __at(0xF000) u16 dbg_frames;
volatile __at(0xF002) u16 dbg_cmds;
volatile __at(0xF004) u16 dbg_fps10;
volatile __at(0xF006) u16 dbg_speed;
volatile __at(0xF008) u16 dbg_pos;
volatile __at(0xF00A) u16 dbg_t60;      /* jiffies desde o inicio ate 60 mph */
volatile __at(0xF00C) u16 dbg_t100;
volatile __at(0xF00E) u16 dbg_top;
volatile __at(0xF010) u16 dbg_rpm;
volatile __at(0xF012) u16 dbg_gear;
volatile __at(0xF014) u16 dbg_state;    /* 0 contagem, 1 corrida, 2 fim */
volatile __at(0xF016) u16 dbg_opos;
volatile __at(0xF018) u16 dbg_result;   /* 1 venceu, 2 perdeu, 3 largada falsa */
volatile __at(0xF01A) u16 dbg_time10;   /* tempo da corrida em decimos de segundo */
volatile __at(0xF01C) u16 dbg_avg;
volatile __at(0xF01E) u16 dbg_ospeed;
volatile __at(0xF020) u16 dbg_in;       /* bit0 espaco, bit1 up_held, bit2 largada falsa, bits4+ marcha */
volatile __at(0xF030) u16 dbg_money;
volatile __at(0xF032) u16 dbg_day;
volatile __at(0xF034) u16 dbg_nown;
volatile __at(0xF036) u16 dbg_nspare;
volatile __at(0xF038) u16 dbg_power;
volatile __at(0xF03A) u16 dbg_t1;
volatile __at(0xF03C) u16 dbg_t2;
volatile __at(0xF03E) u16 dbg_t3;
volatile __at(0xFC9E) u16 JIFFY;

u8 cmd_buf[CMD_MAX * 11];
u16 cmd_n; u8 *cp; u16 g_ybase;
u8 g_trig, g_row6, g_row7, g_st0, g_st1, g_t0, g_t1, g_t3;
u8 g_reg, g_val, g_stick, g_sh, g_sgn, g_fc;
u16 g_lzsrc, g_vlo, g_palp, g_bk_src, g_bk_dst; u8 g_vhi, g_vpg, g_group, g_bk_bank, g_bk_len;
extern void bank_copy(void);
static char nbc[28], nbp[21], nbd[11];
static const char *txt(char *buf, u16 ofs, u8 len) { g_bk_bank = TEXT_BANK; g_bk_src = 0x8000 + TEXT_BASE + ofs; g_bk_dst = (u16)buf; g_bk_len = len; bank_copy(); return buf; }
#define CARN(i) txt(nbc, TXT_CAR_OFS + (i) * 28, 28)
#define PARTN(i) txt(nbp, TXT_PART_OFS + (i) * 21, 21)
#define PILN(i) txt(nbd, TXT_PIL_OFS + (i) * 11, 11)
extern void lz_unpack(void);
extern void psg_w(void);
extern void pal_one(void);
extern void pal_load2(void);
extern void vram_blit(void);
i16 g_a, g_r, g_fx, g_fy, g_fw, g_fh; u16 g_b;
extern void mul12(void);
extern void mul16x8(void);
extern void vdp_wait(void);
u8 rec[11]; u8 g_dry; i16 clip_l, clip_r;
u8 s_yt, s_rem, s_rc, s_kc, s_gc, s_mode, t_y; i16 s_xlf, s_xrf, s_sl2, s_sr2, s_kw, t_xl, t_xr, t_w;
extern void strip_run(void);
extern void draw_road_asm(void);
extern void fill_g(void);
extern void exec_cmds(void);
extern void vdp_reg_g(void);
extern void pal_load(void);
extern void set_screen5(void);
extern void read_input(void);
extern void read_stick(void);

#define ybase g_ybase              /* 0 ou 256: pagina de desenho */

static void vdp_reg(u8 r, u8 v) { g_reg = r; g_val = v; vdp_reg_g(); }

static void push_fill(i16 x, i16 y, i16 w, i16 h, u8 c) {
    g_fx = x; g_fy = y; g_fw = w; g_fh = h; g_fc = c; fill_g();
}

/* ---- estado do jogo ---- */
#ifndef AUTOPILOT
#define AUTOPILOT 0
#endif
#ifndef RACE_LEN
#define RACE_LEN 380            /* arrancada: 380 segmentos (como no original) */
#endif
#define ROAD_LEN 520            /* corrida de estrada (pista com curvas e ladeiras) */
static u16 race_len = RACE_LEN;
#ifndef STREET_LEN
#define STREET_LEN 420
#endif
#define STREET_LIMIT 45                              /* mph: acima disso o guarda multa */
static u8 street_mode, street_res, street_cop;       /* street_res: 0 chegou ao Bobs, 1 desistiu (ESC) */
static u16 street_fine; static u8 street_tix;
static const u16 cop_at[3] = {110, 230, 350};
i8 curve_ram[256], hill_ram[256];                 /* pista da corrida atual (proj.s le estes arrays) */
static void set_track(u8 road) {                  /* arrancada: reta e plana; estrada: a pista sintetica */
    u16 i;
    for (i = 0; i < 256; i++) { curve_ram[i] = road ? curve_tab[i] : 0; hill_ram[i] = road ? hill_tab[i] : 0; }
}
#ifndef OPP_START
#define OPP_START 0            /* teste: posicao inicial do oponente (segmentos) */
#endif
/* powertrain dos 2 veiculos (0 = jogador, 1 = oponente), copiado das tabelas de carros (cars.h) ao iniciar a corrida */
static u16 pt_ratio[10], pt_hi0[10], pt_hi1[10], pt_acc[30], pt_redline[2];
static u8 pt_ngears[2], pt_auto[2], opp_a, opp_b, opp_set;
#define MAXCARS 4
typedef struct { u8 id, eng, trn, man, carb, tir; i8 trim; u16 wear; u8 fuel, brk, tp; } Car;   /* carro do jogador: variantes instaladas, ajuste do ponto, desgaste (0..10000) */
static u16 money; static u8 day, nown, cur; static Car gc[MAXCARS];     /* estado do jogo */
static u16 rnd16(void) { static u16 s = 0x1D2B; s = s * 25173u + 13849u; s ^= JIFFY; return s; }
#define MAXSPARE 12
static u8 spare[MAXSPARE], nspare;
static u8 offer_day = 255;
static u8 game_end;                          /* 0 jogando, 1 acabou o verao, 2 rei do condado, 3 faliu */
static u8 wins_drag, wins_road;
#ifdef START_WINS
#define WINS0 START_WINS
#else
#define WINS0 0
#endif
static u8 sel_car = DEF_CAR, sel_opp = DEF_OPP_CAR, sel_pil = DEF_OPP;
#include "ptmodel.h"
static void car_stock(Car *c, u8 id) {      /* configuracao de fabrica a partir do campo flags do HOT_DATA */
    u16 f = car_flags[id];
    c->id = id; c->tir = (f >> 4) & 3; c->trn = (f >> 6) & 3; c->eng = (f >> 8) & 7; c->man = (f >> 11) & 7; c->carb = (f >> 14) & 3;
    c->trim = 0; c->wear = 0; c->fuel = 10; c->brk = 0; c->tp = 0;      /* fuel em galoes (max 20); brk: 1 motor, 2 cambio, 4 pneus; tp: desgaste dos pneus 0..100 */
}
static void load_pt(u8 slot, const Car *c) { pt_build(slot, car_perf[c->id], c->eng, c->trn, c->man, c->carb, c->tir, c->wear, c->trim, 0); }
static void load_car(u8 slot, u8 id) { Car t; car_stock(&t, id); load_pt(slot, &t); }
#define OPP_A opp_a
#define OPP_B opp_b
#pragma codeseg BANK1
static u8 jhz = 60;                    /* 50 ou 60 Hz (detectado em execucao) */
static u16 tick_inc, adv_k, go_j;
#define GO_J go_j               /* semaforo: 3 s ate o verde */
u16 pos;  u8 sub;
i16 P;                      /* lateral da camera (0 = centro da pista) */
static u16 vrpm[2], vspd[2];            /* 0 = jogador, 1 = oponente */
static u8 vgear[2];
static u8 launched[2];
#define rpm vrpm[0]
#define gear vgear[0]
#define speed vspd[0]
static u16 opos; static u8 osub;
#define OLANE 6                 /* faixa do oponente (em 1/16 da meia pista) */
static u8 state, result, false_start;
static u16 cd_t, opp_wait, race_ticks;
static u32 sum_spd;

i16 XC[N_ROWS], Y[N_ROWS];
u8 PAR[N_ROWS]; u8 g_b8;
i16 xoff_ahead;
extern void project_asm(void);

static u8 step_drag(u16 v) {            /* SRSE.EXE img 0xD544 (arrancada): subunidades por frame */
    return v == 0 ? 0 : v <= 71 ? (u8)(v / 8 + 1) : (u8)(v / 4 - 8);
}

/* ---- entrada do jogador ---- */
static u8 in_throttle, in_brake, in_left, in_right, up_held, dn_held, go_held;
static u8 gear_changed;

static u8 over_t, badshift;                     /* over_t: ticks acima da rotacao de corte; badshift: trocas com o motor passado (risco de quebra) */
static void set_gear(u8 c, u8 ng) {    /* troca de marcha: a velocidade e mantida */
    u32 r;
    if (ng == vgear[c]) return;
    if (c == 0 && ng > vgear[c] && vrpm[0] > pt_redline[0] + 300 && badshift < 250) badshift++;
    r = (u32)vspd[c] * pt_ratio[c * 5 + ng];
    if (ng && r > 5700) { if (ng > vgear[c]) { r = 5700; } else return; }   /* reducao com rotacao excessiva: recusada */
    vgear[c] = ng; vrpm[c] = (u16)r;
    if (c == 0) gear_changed = 1;
}

extern volatile u8 ev_space, ev_shift;
extern void install_hook(void);

#pragma codeseg CODE
static void combine_input(void) {
    u8 e;
    g_stick = g_st0 ? g_st0 : g_st1;
    g_trig = (g_t0 ? 1 : 0) | (g_t1 ? 2 : 0) | (g_t3 ? 4 : 0);
    e = ev_space; if (e) { ev_space = 0; g_trig |= 1; }       /* toque curto travado pela interrupcao */
    e = ev_shift; if (e) { ev_shift = 0; g_trig |= 4; }
}

static u8 want_back(void) {                 /* voltar: ESC, STOP, BS, SHIFT ou botao B do joystick */
    return (g_trig & 4) != 0 || !(g_row6 & 1) || (g_row7 & 0x34) != 0x34;
}

#pragma codeseg BANK1
static void handle_input(void) {
    u8 st = g_stick, up, dn;
    in_throttle = (st == 1 || st == 2 || st == 8);
    in_brake    = (st == 4 || st == 5 || st == 6);
    in_left     = (st == 6 || st == 7 || st == 8);
    in_right    = (st == 2 || st == 3 || st == 4);
    up = (g_trig & 3) != 0;
    dn = (g_trig & 4) != 0 || !(g_row6 & 1);
#if AUTOPILOT
    in_throttle = 1; in_brake = 0;
#endif
#if !AUTOPILOT
    if (state != 2 && !pt_auto[0]) {
        if (up && !up_held && gear < pt_ngears[0]) {
            if (state == 0) false_start = 1;                  /* engatou antes do verde */
            set_gear(0, gear + 1);
        }
        if (dn && !dn_held && gear > 1) set_gear(0, gear - 1);
    }
#endif
    up_held = up; dn_held = dn;
}

/* um tick de fisica de um veiculo (o original roda a 72,826/12 = 6,0688 Hz) */
static void veh_tick(u8 c, u8 thr, u8 brk) {
    u8 g = vgear[c], s = 0, n = pt_ngears[c], i = c * 5 + g;
    u16 r = vrpm[c], cap = pt_redline[c] < 5300 ? pt_redline[c] : 5300;
    if (r >= pt_hi0[i]) { s = 1; if (r >= pt_hi1[i]) s = 2; }
    if (thr) { r += pt_acc[i * 3 + s]; if (r > 5300) r = 5300; }
    else if (r > 120) r -= 120; else r = 0;                    /* freio-motor */
    if (brk) { if (r > 500) r -= 500; else r = 0; }
    if (c == 0 && (P > 950 || P < -950) && r > 200) r -= 200;  /* fora da pista */
    vrpm[c] = r;
    if (c == 0 && r > pt_redline[0] + 50 && over_t < 250) over_t++;
    vspd[c] = g ? r / pt_ratio[i] : 0;
    if (!launched[c]) return;                                   /* ainda na contagem: so acelera o motor */
    if (g == 0) {                                               /* largada */
        if (r >= 2900) set_gear(c, 1);
        return;
    }
    if (g < n) {
        u16 lim;
        if (pt_auto[c]) lim = cap / 100 * 97;
        else if (c == 1) lim = cap - 100 * OPP_A;              /* piloto: troca em (rotacao de corte - 100*habilidade) */
#if AUTOPILOT
        else lim = cap / 100 * 97;
#else
        else return;
#endif
        if (thr && r >= lim) set_gear(c, g + 1);
    } else if (pt_auto[c] && g > 1 && r < 1200) set_gear(c, g - 1);
}

/* IA do oponente (simplificada de SRSE.EXE img 0xD29D): acelera, alivia nas curvas e quando esta longe na frente */
static void opp_ai(void) {
    u8 thr = 1, brk = 0;
    i16 gap = (i16)(opos - pos), cv = curve_ram[(opos + 3) & 255];
    u16 lim;
    if (cv < 0) cv = -cv;
    lim = 160 - 3 * cv;
    if (vspd[1] > lim) { thr = 0; brk = 1; }
    if (gap > 9 && vspd[1] > vspd[0] + 4) thr = 0;           /* nao deixa a vantagem crescer demais */
    veh_tick(1, thr, brk);
}

/* direcao e forca centrifuga por frame (dj = jiffies) */
static void steer_frame(u16 dj) {
    i16 d = 0, e, tgt;
#if AUTOPILOT
    tgt = (xoff_ahead >> 1) - 300;
    e = (tgt - P) >> 2;
    if (e > 40) e = 40; if (e < -40) e = -40;
    P += e * (i16)dj;
#else
    if (in_left) d -= 1;
    if (in_right) d += 1;
    P += d * (i16)(4 + (speed >> 4)) * (i16)dj;
#endif
    P -= (i16)(((i32)curve_ram[pos & 255] * speed * dj) >> 9);   /* forca centrifuga */
    if (P > 1400) P = 1400; if (P < -1400) P = -1400;
}

/* avanco na pista por frame: step(speed) subunidades por frame a ~8,09 fps equivalentes */
static u16 adv_acc[2];
#define ADV_K adv_k
static void advance(u16 dj) {
    adv_acc[0] += step_drag(vspd[0]) * dj * ADV_K;
    while (adv_acc[0] >= 256) { adv_acc[0] -= 256; sub++; if (sub >= 8) { sub = 0; pos++; } }
    adv_acc[1] += step_drag(vspd[1]) * dj * ADV_K;
    while (adv_acc[1] >= 256) { adv_acc[1] -= 256; osub++; if (osub >= 8) { osub = 0; opos++; } }
}

static void project(void) { project_asm(); }

/* retangulo limitado verticalmente a janela da estrada */
static void push_fill_v(i16 x, i16 y, i16 w, i16 h, u8 c) {
    if (y >= VIEW_BOT) return;
    if (y + h > VIEW_BOT) h = VIEW_BOT - y;
    if (y < 0) { h += y; y = 0; }
    push_fill(x, y, w, h, c);
}

u8 vc[15];
extern void vdp_cmd15(void);
#pragma codeseg CODE
static void vc_w(u8 i, i16 v) { vc[i] = (u8)v; vc[i + 1] = (u8)(v >> 8); }

#pragma codeseg BANK1
static void draw_opp(void) {
    i16 d = (i16)(opos - pos), y, xc, hw, w, h, x, sx, sw, dx0, dx1, ty;
    u8 r, si;
    if (d < -1) return;
    d += 3;                                 /* a camera fica no painel: o carro lado a lado aparece ~3 segmentos a frente */
    if (d >= N_ROWS - 1) return;
    r = (u8)d;
    y = Y[r - 1];
    if (y < 0 || y > 250) return;
    xc = XC[r - 1];
    hw = hwy_tab[y];
    x = xc + ((hw * OLANE) >> 4);
    w = hw >> 1;
#if HAVE_COCKPIT
    if (w > 127) w = 127;
    si = spr_sel[w];
    if (si == 255) return;
    sw = spr_w[si]; h = spr_h[opp_set][si]; sx = spr_x[si];
    dx0 = x - (sw >> 1); dx1 = dx0 + sw;                         /* recorte horizontal na janela */
    if (dx0 < clip_l) { sx += clip_l - dx0; dx0 = clip_l; }
    if (dx1 > clip_r) dx1 = clip_r;
    if (dx1 <= dx0) return;
    ty = y - h; if (y > VIEW_BOT) h -= y - VIEW_BOT; if (h <= 0) return;
    vc_w(0, sx); vc_w(2, SPR_Y0 + opp_set * SPR_STRIDE); vc_w(4, dx0); vc_w(6, ybase + ty);
    vc_w(8, dx1 - dx0); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0x98;   /* LMMM com TIMP: cor 0 transparente */
    vdp_cmd15();
#else
    h = (w >> 1) + (w >> 2) + 1;
    push_fill_v(x - (w >> 1), y - h, w, h, 14);
#endif
}

#define LIGHT_X 136
#define LIGHT_TOP 24
static u8 light_rest;                    /* quadros restantes para apagar o semaforo (2 paginas) */
static void draw_lights(u8 on) {         /* semaforo (LIB2 item 13): apagado, vermelho (1/3 da contagem), verde (largada) */
    u8 st = cd_t < GO_J / 3 ? 0 : cd_t < GO_J ? 1 : 2;
    if (on) {
        vc_w(0, st * 48); vc_w(2, LIGHT_Y);
        light_rest = 2;
    } else {
        if (!light_rest) return;
        vc_w(0, LIGHT_X); vc_w(2, 512 + LIGHT_TOP);                  /* copia limpa do cockpit */
        light_rest--;
    }
    vc_w(4, LIGHT_X); vc_w(6, ybase + LIGHT_TOP - WIN_Y);
    vc_w(8, 48); vc_w(10, 32); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}

/* ---- som (PSG): motor nos canais A/B (tom segue a rotacao), bipes do semaforo no canal C ---- */
#pragma codeseg CODE
static u16 beep_end; static u8 beep_on, beep_stage;
static void psg(u8 r, u8 v) { g_reg = r; g_val = v; psg_w(); }
static void snd_init(void) {
    psg(7, 0xB8); psg(8, 0); psg(9, 0); psg(10, 0);       /* tons A,B,C ligados, ruido desligado; bit 7 = porta B como saida (selecao de joystick da BIOS) */
}
static void beep(u16 period, u8 jf) {
    psg(4, (u8)period); psg(5, (u8)(period >> 8)); psg(10, 12);
    beep_end = JIFFY + jf; beep_on = 1;
}
static void snd_engine(u8 on) {
    u16 r = rpm < 600 ? 600 : rpm, per;
    if (beep_on && (i16)(JIFFY - beep_end) >= 0) { psg(10, 0); beep_on = 0; }
    if (!on) { psg(8, 0); psg(9, 0); return; }
    per = (u16)(1677900UL / r);                            /* frequencia = rpm/15 Hz */
    psg(0, (u8)per); psg(1, (u8)(per >> 8));
    per = per - (per >> 2);                                /* B: quarta acima, mais baixa em volume */
    psg(2, (u8)per); psg(3, (u8)(per >> 8));
    psg(8, in_throttle ? 12 : 8); psg(9, in_throttle ? 7 : 4);
}

/* ---- HUD: digitos e letras 3x5 em escala 2x2 ---- */
static const u8 font[10][5] = {
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
static const u8 letters[26][5] = {
    {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},
    {1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,6,3},{6,5,6,5,5},
    {3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
static const u8 sym[9][5] = {{3,6,2,3,6},{2,2,2,0,2},{6,1,2,0,2},{0,2,0,2,0},{0,0,0,2,4},{0,2,7,2,0},{1,1,2,4,4},{5,1,2,4,5},{2,2,0,0,0}};   /* $ ! ? : , + / % ' */
static void draw_glyph(i16 x, i16 y, const u8 *g, u8 c) {
    u8 row, col;
    push_fill(x, y, 8, 12, 0);
    for (row = 0; row < 5; row++)
        for (col = 0; col < 3; col++)
            if (g[row] & (4 >> col)) push_fill(x + col * 2, y + row * 2, 2, 2, c);
}
static void draw_digit(i16 x, i16 y, u8 d, u8 c) { draw_glyph(x, y, font[d], c); }
static void draw_numc(i16 x, i16 y, u16 v, u8 digits, u8 c) {
    u8 i;
    for (i = 0; i < digits; i++) { draw_digit(x + (digits - 1 - i) * 8, y, v % 10, c); v /= 10; }
}
#define draw_num(x, y, v, d) draw_numc(x, y, v, d, 8)
static void draw_text(i16 x, i16 y, const char *s, u8 c) {
    for (; *s; s++, x += 8) {
        if (*s >= 'A' && *s <= 'Z') draw_glyph(x, y, letters[*s - 'A'], c);
        else if (*s >= '0' && *s <= '9') draw_glyph(x, y, font[*s - '0'], c);
        else if (*s == '.') push_fill(x + 2, y + 8, 2, 2, c);
        else if (*s == '-') push_fill(x, y + 4, 6, 2, c);
        else if (*s == '$') draw_glyph(x, y, sym[0], c);
        else if (*s == '!') draw_glyph(x, y, sym[1], c);
        else if (*s == '?') draw_glyph(x, y, sym[2], c);
        else if (*s == ':') draw_glyph(x, y, sym[3], c);
        else if (*s == ',') draw_glyph(x, y, sym[4], c);
        else if (*s == '+') draw_glyph(x, y, sym[5], c);
        else if (*s == '/') draw_glyph(x, y, sym[6], c);
        else if (*s == '%') draw_glyph(x, y, sym[7], c);
        else if (*s == '\'') draw_glyph(x, y, sym[8], c);
    }
}

/* instrumentos do cockpit: ponteiros (LINE) sobre o fundo restaurado da pagina 2 (HMMM); so redesenha quando o valor muda
   (cache por pagina, pois ha 2 paginas alternadas) */
#define GEAR_SHEET_Y 1000        /* digitos 0-9 pre-desenhados na pagina 3 */
#define HUD_GEAR_X 133
#define HUD_GEAR_Y 115
#pragma codeseg BANK1
static void gauge(i16 cx, i16 cy, i16 tx, i16 ty) {
    i16 dx = tx, dy = ty, ax = tx < 0 ? -tx : tx, ay = ty < 0 ? -ty : ty;
    u8 maj = ay > ax;
    vc_w(0, (cx - 16) & ~1); vc_w(2, 512 + cy - 16); vc_w(4, (cx - 16) & ~1); vc_w(6, ybase + cy - 16);   /* fundo limpo (pagina 2) */
    vc_w(8, 34); vc_w(10, 32); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
    vc_w(4, cx); vc_w(6, ybase + cy);                                                        /* ponteiro (2 pixels de espessura) */
    vc_w(8, maj ? ay : ax); vc_w(10, maj ? ax : ay); vc[12] = 6;
    vc[13] = (maj ? 1 : 0) | (dx < 0 ? 4 : 0) | (dy < 0 ? 8 : 0); vc[14] = 0x70;
    vdp_cmd15();
    vc_w(4, cx + 1); vdp_cmd15();
}
static u16 hud_c[2][3];
static void hud_reset(void) { u8 i; for (i = 0; i < 3; i++) hud_c[0][i] = hud_c[1][i] = 0xFFFF; }
static void draw_hud_page(void) {
    u8 pg = ybase != 0;
    u16 sp = speed > NEEDLE_SPD_N - 1 ? NEEDLE_SPD_N - 1 : speed, rp = rpm / 50, g = gear;
    if (rp > NEEDLE_RPM_N - 1) rp = NEEDLE_RPM_N - 1;
    if (hud_c[pg][0] != sp) { hud_c[pg][0] = sp; gauge(GAUGE_SPD_X, GAUGE_SPD_Y, needle_spd[sp * 2], needle_spd[sp * 2 + 1]); }
    if (hud_c[pg][1] != rp) { hud_c[pg][1] = rp; gauge(GAUGE_RPM_X, GAUGE_RPM_Y, needle_rpm[rp * 2], needle_rpm[rp * 2 + 1]); }
    (void)pg;
    vc_w(0, g * 8); vc_w(2, GEAR_SHEET_Y); vc_w(4, HUD_GEAR_X); vc_w(6, ybase + HUD_GEAR_Y);   /* o overlay apaga o mostrador pequeno: recopia a marcha todo quadro */
    vc_w(8, 8); vc_w(10, 12); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}

#ifdef DEBUG_HUD
static void draw_hud(void) {                /* escrito nas duas paginas, 1x por segundo */
    u8 pg;
    clip_l = 0; clip_r = 256;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        cmd_n = 0; cp = cmd_buf;
        draw_num(8, 120, dbg_fps10, 3);
        draw_num(48, 120, dbg_cmds, 3);
        draw_num(88, 120, dbg_speed, 3);
        vdp_wait();
    }
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

#endif

extern void load_assets(void);
static void load_cockpit(void) {            /* descomprime os assets (cockpit nas paginas 0/1/2, sprites na 3) para a VRAM */
#if HAVE_COCKPIT
    u8 rg1 = *(volatile u8 *)0xF3E0;
    vdp_reg(1, rg1 & ~0x40);                /* tela apagada: acesso a VRAM sem espera */
    g_group = 0; load_assets();             /* troca de banco ASCII16 dentro do carregador (pagina 1) */
    vdp_reg(1, rg1 | 0x40);
#endif
}

static void init_static(void) {
    u8 pg;
    clip_l = 0; clip_r = 256;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        cmd_n = 0; cp = cmd_buf;
        push_fill(0, 0, 256, 212, 0);
        vdp_wait();
    }
    load_cockpit();
    ybase = GEAR_SHEET_Y; cmd_n = 0; cp = cmd_buf;
    for (pg = 0; pg < 10; pg++) draw_digit(pg * 8, 0, pg, 13);
    vdp_wait();
    hud_reset();
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

static void draw_t10(i16 x, i16 y, u16 t10, u8 c) {
    draw_num(x, y, t10 / 10, 3);
    push_fill(x + 24, y + 8, 2, 2, c);
    draw_digit(x + 28, y, t10 % 10, 8);
}

static void show_result(void) {               /* tela de resultado, nas duas paginas */
    u8 pg;
    psg(8, 0); psg(9, 0); psg(10, 0); beep_on = 0;
    u16 t10 = (u16)(((u32)race_ticks * 1687) >> 10), avg = race_ticks ? (u16)(sum_spd / race_ticks) : 0;
    clip_l = 0; clip_r = 256;
    dbg_time10 = t10; dbg_avg = avg; dbg_result = result; dbg_state = state;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        push_fill(40, 20, 176, 100, 0);
        if (result == 1) draw_text(96, 28, "YOU WIN", 13);
        else if (result == 2) draw_text(92, 28, "YOU LOSE", 6);
        else draw_text(56, 28, "JUMPED THE LIGHT", 6);
        if (result != 3) {
            draw_text(60, 52, "TIME", 7); draw_t10(110, 52, t10, 7);
            draw_text(60, 72, "AVG MPH", 7); draw_num(128, 72, avg, 3);
        }
        draw_text(64, 98, "PRESS SPACE", 8);
        vdp_wait();
    }
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

#pragma codeseg BANK2
static void show_title(void) BK {
    u8 pg;
    clip_l = 0; clip_r = 256;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        push_fill(0, 0, 256, 212, 0);
        draw_text(56, 16, "STREET ROD MSX2", 13);
        draw_text(72, 32, "DRAG RACE DEMO", 7);
        draw_text(16, 64, "UP", 8);       draw_text(80, 64, "GAS", 7);
        draw_text(16, 78, "DOWN", 8);     draw_text(80, 78, "BRAKE", 7);
        draw_text(16, 92, "LEFT RIGHT", 8); draw_text(112, 92, "STEER", 7);
        draw_text(16, 106, "SPACE", 8);   draw_text(80, 106, "SHIFT UP", 7);
        draw_text(16, 120, "SHIFT", 8);   draw_text(80, 120, "SHIFT DOWN", 7);
        draw_text(16, 134, "JOY BUTTON A-B", 8);
        draw_text(40, 160, "PRESS SPACE TO START", 13);
        draw_text(28, 182, "DONT SHIFT BEFORE GREEN", 6);
        vdp_wait();
    }
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

static void wait_start(void) BK {            /* espera ESPACO / botao A (ignora tecla ja apertada no boot) */
    u8 held = 1, go;
    for (;;) {
        read_input(); combine_input();
        go = (g_trig & 3) != 0;
        if (go && !held) return;
        held = go;
    }
}

/* ---- menu de selecao: carro do jogador, carro e piloto do oponente ---- */
#pragma codeseg CODE
static const u8 paint_rb[8] = {0x50 | 5, 0x60, 0x10 | 6, 0x00 | 1, 0x70, 0x60 | 6, 0x70, 0x00 | 5};   /* R<<4|B da cor da pintura do oponente */
static const u8 paint_g[8]  = {0, 0, 2, 4, 6, 6, 3, 5};

/* ---- estado do jogo e hub (garagem) ---- */
#ifndef START_DAY
#define START_DAY 0
#endif
#ifndef START_MONEY
#define START_MONEY 1000
#endif
#define HC_W 4          /* cores da paleta da garagem (indices EGA) */
#define HC_Y 15
#define HC_G 13
#define HC_R 11
#pragma codeseg BANK2
static void new_game(void) BK {
    money = START_MONEY; day = START_DAY; wins_drag = WINS0; wins_road = 0; game_end = 0; nown = 0; cur = 0; sel_car = 0; offer_day = 255; nspare = 0;
#ifdef START_CAR
    car_stock(&gc[0], START_CAR); nown = 1; sel_car = START_CAR;                  /* so para testes automatizados */
#endif
}

#pragma codeseg CODE
static void draw_nz(i16 x, i16 y, u16 v, u8 digits, u8 c) {      /* numero sem zeros a esquerda */
    u16 p = 1; u8 i;
    for (i = 1; i < digits; i++) p *= 10;
    for (i = 0; i < digits; i++, p /= 10) { if (v >= p || i == digits - 1) { draw_digit(x + i * 8, y, (v / p) % 10, c); } }
}
static void load_garage(void) {             /* cena da garagem (LIB2 item 32) nas paginas 0/1/2, paleta propria */
    u8 rg1 = *(volatile u8 *)0xF3E0;
    vdp_reg(1, rg1 & ~0x40);
    g_palp = (u16)pal_garage; pal_load2();
    g_group = 1; load_assets();
    vdp_reg(1, rg1 | 0x40);
}
static void hub_restore(i16 x, i16 y, i16 w, i16 h) {   /* devolve um pedaco da cena limpa (copia na pagina 2) */
    vc_w(0, x); vc_w(2, 512 + y); vc_w(4, x); vc_w(6, y); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}
static void hub_status(void) {
    static const char mon[2][4] = {"AUG", "SEP"};
    u8 mo = day >= 31;
    push_fill(0, 190, 256, 22, 0);
    draw_text(8, 190, "$", HC_Y); draw_nz(16, 190, money, 5, HC_Y);
    draw_text(88, 190, mon[mo], HC_W); draw_numc(120, 190, mo ? day - 30 : day + 1, 2, HC_W); draw_text(144, 190, "1963", HC_W);
    draw_text(8, 202, nown ? CARN(gc[cur].id) : "NO CAR - BUY ONE", HC_G);
    vdp_wait();
}
/* menu vertical sobre a cena: devolve o item escolhido (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_X 8
#define UI_W 240
/* lista rolavel (ate UI_VIS linhas visiveis); o texto de cada item vem de ui_get(i). Devolve o indice (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_VIS 8
static u8 ui_nohint;                         /* 1 = menu raiz (sem voltar): nao mostra a dica ESC */
static const char *(*ui_get)(u8 i);
static const char *const *ui_arr;
static const char *ui_arr_get(u8 i) { return ui_arr[i]; }
static char tb[32];
static void ui_row(u8 i, i16 y0, u8 top, u8 sel) {
    i16 y = y0 + 24 + 14 * (i - top);
    push_fill(UI_X + 4, y - 2, UI_W - 8, 14, 0);
    draw_text(UI_X + 26, y, ui_get(i), i == sel ? HC_G : HC_W);
    if (i == sel) push_fill(UI_X + 10, y, 8, 8, HC_G);
}
static u8 ui_list(const char *title, u8 n, u8 sel) {
    u8 vis = n < UI_VIS ? n : UI_VIS, top = 0, prev = 0xFF, i, held = 1, bheld = 1;
    i16 h = 24 + 14 * vis, y0 = 186 - h;        /* o menu fica no chao da garagem (a parte baixa da cena) */
    if (!n) return 255;
    if (sel >= vis) top = sel - vis + 1;
    clip_l = 0; clip_r = 256; ybase = 0; dbg_state = 7; dbg_money = money; dbg_day = day; dbg_nown = nown; dbg_nspare = nspare;
    cmd_n = 0; cp = cmd_buf;
    hub_restore(0, 22, 256, 168);                /* apaga o menu anterior */
    push_fill(UI_X, y0, UI_W, h, 0);
    push_fill(UI_X, y0, UI_W, 2, HC_W); push_fill(UI_X, y0 + h - 2, UI_W, 2, HC_W);
    push_fill(UI_X, y0, 2, h, HC_W); push_fill(UI_X + UI_W - 2, y0, 2, h, HC_W);
    draw_text(UI_X + 10, y0 + 6, title, HC_Y);
    push_fill(184, 190, 72, 12, 0);                  /* dica de voltar na barra de status (menu raiz nao tem) */
    if (!ui_nohint) draw_text(184, 190, "ESC BACK", HC_R);
    for (i = top; i < top + vis; i++) ui_row(i, y0, top, sel);
    vdp_wait();
    for (;;) {
        u8 st, go, back, ns = sel;
        read_input(); combine_input();
        st = g_stick; go = (g_trig & 3) != 0; back = want_back();
        if (go && !held) return sel;
        if (back && !bheld) return 255;
        held = go; bheld = back;
        if (st == prev) continue;
        prev = st;
        if (st == 1 && sel > 0) ns = sel - 1; else if (st == 5 && sel + 1 < n) ns = sel + 1;
        if (ns != sel) {
            cmd_n = 0; cp = cmd_buf;
            if (ns < top || ns >= top + vis) {          /* rolagem: redesenha tudo */
                top = ns < top ? ns : ns - vis + 1; sel = ns;
                for (i = top; i < top + vis; i++) ui_row(i, y0, top, sel);
            } else {
                i = sel; sel = ns; ui_row(i, y0, top, sel); ui_row(sel, y0, top, sel);
            }
            vdp_wait();
        }
    }
}
static u8 ui_pick(const char *title, const char *const *it, u8 n, u8 sel) { ui_arr = it; ui_get = ui_arr_get; return ui_list(title, n, sel); }
static void ui_note(const char *a, const char *b) {   /* aviso de 2 linhas, ate ESPACO/SHIFT */
    const char *it[2]; it[0] = a; it[1] = b;
    ui_pick(a, it + 1, 1, 0);
}
/* "NOME....... $1234" em tb[] (nome cortado/preenchido em 21 colunas, preco em 4 digitos) */
static const char *fmt_price(const char *name, u16 price) {
    u8 i = 0;
    while (i < 20 && name[i]) { tb[i] = name[i]; i++; }
    while (i < 21) tb[i++] = ' ';
    tb[i++] = '$';
    tb[i + 3] = '0' + price % 10; tb[i + 2] = price >= 10 ? '0' + (price / 10) % 10 : ' ';
    tb[i + 1] = price >= 100 ? '0' + (price / 100) % 10 : ' '; tb[i] = price >= 1000 ? '0' + (price / 1000) % 10 : ' ';
    tb[i + 4] = 0;
    return tb;
}
static const char *fmt_short(u16 v) {       /* "YOU ARE $123 SHORT" */
    u8 i = 0;
    static const char pre[] = "YOU ARE $";
    while (pre[i]) { tb[i] = pre[i]; i++; }
    if (v >= 1000) tb[i++] = '0' + (v / 1000) % 10;
    if (v >= 100) tb[i++] = '0' + (v / 100) % 10;
    if (v >= 10) tb[i++] = '0' + (v / 10) % 10;
    tb[i++] = '0' + v % 10;
    { static const char suf[] = " SHORT"; u8 k = 0; while (suf[k]) tb[i++] = suf[k++]; }
    tb[i] = 0;
    return tb;
}

/* ---- inventario ---- */

/* ---- garagem: carros, pecas, ajuste do ponto, informacoes, venda ---- */
#pragma codeseg BANK2
static const char *const cat_label[5] = {"ENGINE", "TRANS", "CARB", "MANIFOLD", "TIRES"};
static u8 *car_var(Car *c, u8 cat) { return cat == 0 ? &c->eng : cat == 1 ? &c->trn : cat == 2 ? &c->carb : cat == 3 ? &c->man : &c->tir; }
static u8 car_maker(const Car *c) { return car_flags[c->id] & 7; }        /* 1 = GM, 2 = Ford, 4 = Chrysler (mesma mascara das pecas) */
static u8 part_find(u8 cat, u8 var, u8 maker) {         /* id do catalogo da peca instalada (categoria, variante, montadora) */
    u8 i;
    for (i = 0; i < NPARTS; i++) if (part_cat[i] == cat && part_var[i] == var && (part_mk[i] & maker)) return i;
    return 255;
}
static void put_str(char *d, u8 *n, const char *s) { while (*s) d[(*n)++] = *s++; }
static void put_num(char *d, u8 *n, u16 v) {
    u16 p = 10000; u8 on = 0;
    for (; p; p /= 10) { u8 dg = (v / p) % 10; if (dg || on || p == 1) { d[(*n)++] = '0' + dg; on = 1; } }
}
static Car *gcar;                                       /* carro em edicao */
static u8 gcat, gsp[MAXSPARE], gsn;
static const char *inst_get(u8 cat) {                   /* "ENGINE        GM V-8 283 CI" */
    u8 n = 0, id = part_find(cat, *car_var(gcar, cat), car_maker(gcar)); 
    put_str(tb, &n, cat_label[cat]);
    while (n < 10) tb[n++] = ' ';
    if (id != 255) put_str(tb, &n, PARTN(id)); else put_str(tb, &n, "?");
    tb[n] = 0;
    return tb;
}
static const char *spare_cat_get(u8 i) { return PARTN(spare[gsp[i]]); }
static void change_part(u8 cat) {
    u8 i, r, old, fit = car_maker(gcar);
    gsn = 0;
    for (i = 0; i < nspare; i++) if (part_cat[spare[i]] == cat && (part_mk[spare[i]] & fit)) gsp[gsn++] = i;
    if (!gsn) { ui_note("NO SPARE PART", "THAT WILL FIT"); return; }
    ui_get = spare_cat_get;
    r = ui_list(cat_label[cat], gsn, 0);
    if (r == 255) return;
    r = gsp[r];
    old = part_find(cat, *car_var(gcar, cat), fit);
    *car_var(gcar, cat) = part_var[spare[r]];
    if (cat == 0) gcar->wear = 0;                       /* motor novo */
    if (cat == 4) gcar->tp = 0;                         /* pneus novos */
    if (old != 255) spare[r] = old; else spare[r] = spare[--nspare];
    ui_note("DONE!", "PART INSTALLED");
}
static void car_parts(void) {
    u8 c = 0;
    static const char *const dummy[1] = {""};
    (void)dummy;
    for (;;) {
        ui_get = inst_get;
        c = ui_list("CAR PARTS - SPACE TO CHANGE", 5, c);
        if (c == 255) return;
        change_part(c);
    }
}
static u8 try_pay(u16 price) {            /* confere o dinheiro; mensagem se faltar */
    if (money < price) { ui_note("NO GO, HOT SHOT", fmt_short(price - money)); return 0; }
    money -= price; return 1;
}
static u16 repair_cost(const Car *c) {                  /* motor/cambio/pneus quebrados: preco das pecas atuais */
    u16 t = 0;
    if (c->brk & 1) t += PART_PRICE(part_find(0, c->eng, car_maker(c)));
    if (c->brk & 2) t += PART_PRICE(part_find(1, c->trn, car_maker(c)));
    if (c->brk & 4) t += PART_PRICE(part_find(4, c->tir, 7));
    return t;
}
static u16 scrap_value(const Car *c) { return car_price[c->id] / 10; }
static void remove_car(u8 ci) {
    u8 n;
    for (n = ci; n + 1 < nown; n++) gc[n] = gc[n + 1];
    nown--;
    if (cur >= nown) cur = nown ? nown - 1 : 0; else if (cur > ci) cur--;
    if (nown) sel_car = gc[cur].id;
}
static u16 num_title(char *d, const char *pre, u16 v) { u8 n = 0; put_str(d, &n, pre); put_num(d, &n, v); d[n] = 0; return v; }
static u8 fix_car(u8 ci) {                              /* 1 = o carro saiu da lista (sucata) */
    static const char *const it[3] = {"FIX IT", "JUNK IT", "FORGET IT"};
    char title[28]; u16 cost = repair_cost(gcar);
    u8 r;
    if (!gcar->brk) { ui_note("NOTHING TO FIX", "IT RUNS FINE"); return 0; }
    num_title(title, "DAMN. IT'LL COST $", cost);
    r = ui_pick(title, it, 3, 0);
    if (r == 0) {
        if (!try_pay(cost)) return 0;
        if (gcar->brk & 1) gcar->wear = 0;
        if (gcar->brk & 4) gcar->tp = 0;
        gcar->brk = 0;
        ui_note("OH, WELL, IT COULD", "HAVE BEEN WORSE.");
    } else if (r == 1) {
        money += scrap_value(gcar); remove_car(ci);
        ui_note("JUNKED IT", "AND YOU GOT A FEW BUCKS");
        return 1;
    }
    return 0;
}
static void gas_up(void) {
    u8 gal = 20 - gcar->fuel; u16 cost;
    if (!gal) { ui_note("YOU'VE GOT", "A FULL TANK!"); return; }
    if ((u16)gal > money * 3) gal = (u8)(money * 3);
    if (!gal) { ui_note("GAS AIN'T FREE!", "YOU'RE BROKE"); return; }
    cost = (gal + 2) / 3;
    money -= cost; gcar->fuel += gal;
    ui_note("FILL 'ER UP!", "TANK FILLED");
}
static u16 car_power(const Car *c) { return pt_power(car_perf[c->id], c->eng, c->trn, c->man, c->carb, c->wear, c->trim, 0); }
static char info_buf[7][30];
static const char *info_get(u8 i) { return info_buf[i]; }
static void car_info(void) {
    u8 n; u16 p = car_power(gcar);
    dbg_power = p; dbg_t1 = (u16)pt_quality(car_perf[gcar->id], gcar->eng, gcar->trn, gcar->man, gcar->carb, gcar->wear, gcar->trim, 0); dbg_t2 = gcar->wear; dbg_t3 = car_perf[gcar->id] * 256 + gcar->eng * 16 + gcar->trn;
    n = 0; put_str(info_buf[0], &n, "POWER "); put_num(info_buf[0], &n, p / 100); info_buf[0][n] = 0;
    n = 0; put_str(info_buf[1], &n, "GEARS "); put_num(info_buf[1], &n, pm_gears[gcar->trn]); put_str(info_buf[1], &n, "  REDLINE "); put_num(info_buf[1], &n, pm_redline[gcar->eng]); info_buf[1][n] = 0;
    n = 0; put_str(info_buf[2], &n, gcar->trn == 0 ? "AUTOMATIC" : "MANUAL"); put_str(info_buf[2], &n, "  WEAR "); put_num(info_buf[2], &n, gcar->wear / 100); put_str(info_buf[2], &n, "%"); info_buf[2][n] = 0;
    n = 0; put_str(info_buf[3], &n, "TIMING "); if (gcar->trim < 0) info_buf[3][n++] = '-'; put_num(info_buf[3], &n, gcar->trim < 0 ? -gcar->trim : gcar->trim); info_buf[3][n] = 0;
    n = 0; put_str(info_buf[4], &n, "FUEL "); put_num(info_buf[4], &n, gcar->fuel); put_str(info_buf[4], &n, " GAL  TIRES "); put_num(info_buf[4], &n, gcar->tp); put_str(info_buf[4], &n, "% WORN"); info_buf[4][n] = 0;
    n = 0; put_str(info_buf[5], &n, gcar->brk ? "BROKEN:" : "RUNS GREAT"); if (gcar->brk & 1) put_str(info_buf[5], &n, " ENGINE"); if (gcar->brk & 2) put_str(info_buf[5], &n, " TRANS"); if (gcar->brk & 4) put_str(info_buf[5], &n, " TIRES"); info_buf[5][n] = 0;
    ui_get = info_get;
    ui_list(CARN(gcar->id), 6, 0);
}
static void car_tune(void) {
    static const char *const it[3] = {"RETARD", "ADVANCE", "DONE"};
    u8 r, n; char title[28]; u8 sel = 1;
    for (;;) {
        n = 0; put_str(title, &n, "TIMING "); if (gcar->trim < 0) title[n++] = '-'; put_num(title, &n, gcar->trim < 0 ? -gcar->trim : gcar->trim);
        put_str(title, &n, "  POWER "); put_num(title, &n, car_power(gcar) / 100); title[n] = 0;
        r = ui_pick(title, it, 3, sel);
        if (r == 255 || r == 2) return;
        sel = r;
        if (r == 0 && gcar->trim > -8) gcar->trim--;
        if (r == 1 && gcar->trim < 8) gcar->trim++;
    }
}
static u16 sale_value(const Car *c) { return (u16)((u32)car_price[c->id] * (200 - c->wear / 100) / 200); }
static u8 sell_car(u8 ci) {                             /* 1 = vendido */
    static const char *const it[4] = {"TAKE IT", "ASK 20% MORE", "ASK 40% MORE", "FORGET IT"};
    u16 offer_ = (u16)((u32)sale_value(gcar) * 55 / 100); u8 r, n; char title[28];
    for (;;) {
        n = 0; put_str(title, &n, "I'LL GIVE YOU $"); put_num(title, &n, offer_); title[n] = 0;
        r = ui_pick(title, it, 4, 0);
        if (r == 255 || r == 3) return 0;
        if (r == 0) break;
        if ((u8)(rnd16() >> 8) % 100 < (r == 1 ? 55 : 20)) { offer_ = (u16)((u32)offer_ * (r == 1 ? 120 : 140) / 100); ui_note("O.K.!", "YOU'VE GOT A DEAL"); break; }
        ui_note("NO WAY!", "TAKE IT OR LEAVE IT");
        { static const char *const it2[2] = {"TAKE IT", "FORGET IT"}; if (ui_pick(title, it2, 2, 0) != 0) return 0; }
        break;
    }
    money += offer_;
    for (n = ci; n + 1 < nown; n++) gc[n] = gc[n + 1];
    nown--;
    if (cur >= nown) cur = nown ? nown - 1 : 0;
    else if (cur > ci) cur--;
    if (nown) sel_car = gc[cur].id;
    return 1;
}
static void car_menu(u8 ci) {
    static const char *const it[8] = {"DRIVE THIS CAR", "CAR PARTS", "TUNE TIMING", "CAR INFO", "FIX OR JUNK IT", "GAS UP", "SELL IT", "BACK"};
    u8 r = 0;
    gcar = &gc[ci];
    for (;;) {
        r = ui_pick(CARN(gc[ci].id), it, 8, r);
        if (r == 255 || r == 7) return;
        if (r == 0) { cur = ci; sel_car = gc[ci].id; ui_note("O.K.!", "NOW DRIVING THIS CAR"); }
        else if (r == 1) car_parts();
        else if (r == 2) car_tune();
        else if (r == 3) car_info();
        else if (r == 4) { if (fix_car(ci)) return; }
        else if (r == 5) gas_up();
        else if (sell_car(ci)) return;
    }
}
static const char *carlist_get(u8 i) {
    u8 n = 0;
    tb[n++] = i == cur ? '*' : ' ';
    put_str(tb, &n, CARN(gc[i].id));
    tb[n] = 0;
    return tb;
}
static void cars_screen(void) {
    u8 r, sel = 0;
    for (;;) {
        if (!nown) { ui_note("YOUR CARS", "BUY A CAR FIRST!"); return; }
        ui_get = carlist_get;
        r = ui_list("YOUR CARS", nown, sel < nown ? sel : nown - 1);
        if (r == 255) return;
        sel = r;
        car_menu(r);
    }
}

/* ---- jornal: carros usados ---- */
#define NOFFERS 6
static u8 offer[NOFFERS], offer_pct[NOFFERS], offer_mask;
static void make_offers(void) {            /* ofertas do dia: sorteio deterministico por dia; vendidos/recusados ficam marcados */
    u16 r = day * 2713u + 4219u; u8 i, j, id;
    if (offer_day == day) return;
    offer_day = day; offer_mask = 0;
    static const u8 cheap[5] = {0, 6, 16, 19, 23};        /* sempre ha uma pechincha: garante que da para comprar o primeiro carro */
    for (i = 0; i < NOFFERS; i++) {
        do {
            r = r * 25173u + 13849u; id = i ? (u8)((r >> 4) % 25) : cheap[(r >> 5) % 5];
            for (j = 0; j < i; j++) if (offer[j] == id) break;
        } while (j < i);
        offer[i] = id; offer_pct[i] = 80 + (u8)((r >> 9) % 41);
    }
}
static u16 offer_price(u8 i) { return (u16)((u32)car_price[offer[i]] * offer_pct[i] / 100); }
static const char *offer_get(u8 i) { return (offer_mask >> i) & 1 ? "(GONE)" : fmt_price(CARN(offer[i]), offer_price(i)); }
static void buy_car(u8 i) {
    static const char *const it[3] = {"BUY AT ASKING PRICE", "MAKE AN OFFER", "FORGET IT"};
    static const char *const dis[3] = {"10% LESS", "20% LESS", "30% LESS"};
    u16 price = offer_price(i); u8 r;
    r = ui_pick(CARN(offer[i]), it, 3, 0);
    if (r == 255 || r == 2) return;
    if (nown >= MAXCARS) { ui_note("YOU'VE GOT NO ROOM!", "SELL A CAR FIRST"); return; }
    if (r == 1) {
        u8 d = ui_pick("HOW ABOUT...", dis, 3, 0), chance;
        if (d == 255) return;
        chance = d == 0 ? 80 : d == 1 ? 50 : 15;
        if ((u8)(rnd16() >> 8) % 100 >= chance) { ui_note("GET LOST!", "NO DEAL"); offer_mask |= 1 << i; return; }
        price = (u16)((u32)price * (90 - 10 * d) / 100);
        ui_note("I'LL TAKE IT!", "DEAL");
    }
    if (!try_pay(price)) return;
    car_stock(&gc[nown], offer[i]); gc[nown].wear = (u16)(rnd16() % 3000);         /* usado: 0 a 30% de desgaste */
    nown++; offer_mask |= 1 << i;
    if (nown == 1) { cur = 0; sel_car = gc[0].id; }
    ui_note("IT'S YOURS!", "ENJOY THE RIDE");
}
static void used_cars(void) {
    u8 r, sel = 0;
    make_offers();
    for (;;) {
        ui_get = offer_get;
        r = ui_list("USED CARS", NOFFERS, sel);
        if (r == 255) return;
        sel = r;
        if ((offer_mask >> r) & 1) { ui_note("SOLD ALREADY", "TRY ANOTHER"); continue; }
        buy_car(r);
    }
}

/* ---- jornal: pecas ---- */
static u8 pcat, plist[16], pn;
static const char *part_get(u8 i) { return fmt_price(PARTN(plist[i]), PART_PRICE(plist[i])); }
static void parts_shop(void) {
    static const char *const cats[6] = {"ENGINES", "TRANSMISSIONS", "CARBURETORS", "MANIFOLDS", "TIRES", "BACK"};
    u8 c = 0, r, i;
    for (;;) {
        c = ui_pick("AUTO PARTS", cats, 6, c);
        if (c == 255 || c == 5) return;
        pn = 0;
        for (i = 0; i < NPARTS; i++) if (part_cat[i] == c) plist[pn++] = i;
        r = 0;
        for (;;) {
            ui_get = part_get;
            r = ui_list(cats[c], pn, r);
            if (r == 255) break;
            if (nspare >= MAXSPARE) { ui_note("TOO MUCH JUNK", "SELL SOME PARTS"); continue; }
            if (!try_pay(PART_PRICE(plist[r]))) continue;
            spare[nspare++] = plist[r];
            ui_note("IT'S YOURS!", PARTN(plist[r]));
        }
    }
}
static const char *spare_get(u8 i) { return fmt_price(PARTN(spare[i]), PART_PRICE(spare[i]) / 2); }
static void sell_parts(void) {
    u8 r, sel = 0;
    for (;;) {
        if (!nspare) { ui_note("SELL PARTS", "NO SPARE PARTS"); return; }
        ui_get = spare_get;
        r = ui_list("SELL SPARE PARTS", nspare, sel);
        if (r == 255) return;
        money += PART_PRICE(spare[r]) / 2;
        spare[r] = spare[--nspare];
        sel = r > 0 ? r - 1 : 0;
    }
}
static void newspaper(void) {
    static const char *const items[4] = {"USED CARS", "AUTO PARTS", "SELL SPARE PARTS", "BACK"};
    u8 r, sel = 0;
    for (;;) {
        r = ui_pick("LOS ANGELES NEWS", items, 4, sel);
        if (r == 255 || r == 3) return;
        sel = r;
        if (r == 0) used_cars(); else if (r == 1) parts_shop(); else sell_parts();
    }
}
/* ---- Bob's Drive-In: desafiar um piloto (arrancada / estrada / so por diversao / pink slip), apostas ---- */
static u8 race_type, opp_ok;                 /* 0 arrancada, 1 estrada, 2 so por diversao, 3 pink slip */
static u16 bet;

static const char *pm1, *pm2;                /* mensagem do resultado da ultima corrida (mostrada ao voltar a garagem) */
static const u16 bets[4] = {10, 25, 50, 100};
static const char *pil_get(u8 i) { return PILN(i); }
static void pick_opp_car(u8 pil) {           /* pilotos melhores (indice maior) correm com carros de classe mais alta */
    u8 target = (u8)((u16)pil * 8 / 21), c, tries;
    for (tries = 0; tries < 60; tries++) {
        i16 d;
        c = (u8)(rnd16() % 25); d = (i16)car_perf[c] - target;
        if (d >= -1 && d <= 1) break;
    }
    sel_opp = c;
}
static const char *const yes_msg[6] = {"YOU'RE ON!", "ALRIGHT!", "LET'S GO!", "COOL!", "O.K.!", "LET'S HIT IT!"};
static const char *const no_msg[10] = {"CHICKEN!", "GET LOST!", "GIVE ME A BREAK", "GET OUTTA MY FACE", "BUZZ OFF", "WHADDA LOSER", "BLOW IT OUT YOUR EAR", "FORGET IT, GEEK", "BEAT IT, TWERP", "NO WAY, YA NERD"};
static u8 refuses(u8 pct) {                  /* 1 = o oponente recusa (pct = chance de ACEITAR); -DTEST_ACCEPT aceita sempre */
#ifdef TEST_ACCEPT
    (void)pct; return 0;
#else
    return (u8)(rnd16() >> 8) % 100 >= pct;
#endif
}
static u8 bobs(void) {                       /* 1 = a corrida vai comecar */
    static const char *const types[5] = {"DRAG RACE", "ROAD RACE", "JUST FOR KICKS", "PINK SLIPS", "BACK"};
    static const char *const betm[5] = {"BET $10", "BET $25", "BET $50", "BET $100", "BACK"};
    u8 p = 0, t = 0, b;
    for (;;) {
        ui_get = pil_get;
        p = ui_list("CHALLENGE WHO?", NPILOTS, p);
        if (p == 255) return 0;
        pick_opp_car(p); sel_pil = p;
        ui_note(PILN(p), CARN(sel_opp));            /* quem e e com que carro */
        t = ui_pick("WHAT KIND OF RACE?", types, 5, t);
        if (t == 255 || t == 4) continue;
        race_type = t; bet = 0;
        if (t == 3) {                               /* pink slip: o carro de um contra o do outro */
            if ((u8)(wins_drag + wins_road) < 1 + p / 2) { ui_note("YOU'RE GONNA HAFTA", "WIN MORE RACES!"); continue; }
            if (refuses(70)) { ui_note(no_msg[rnd16() % 10], "NOT FOR PINK SLIPS"); continue; }
            ui_note("COOL, DUDE! GET YOUR", "PINK SLIP OUT, LET'S GO!");
        } else if (t != 2) {
            b = ui_pick("HOW MUCH?", betm, 5, 1);
            if (b == 255 || b == 4) continue;
            if (money < bets[b]) { ui_note("YOU DON'T HAVE", "ENOUGH DOUGH FOR THAT"); continue; }
            if (refuses(100 - 15 * b)) { ui_note(no_msg[rnd16() % 10], "TOO RICH FOR HIM"); continue; }
            bet = bets[b];
            ui_note(yes_msg[rnd16() % 6], "LET'S HIT THE STRIP");
        } else ui_note(yes_msg[rnd16() % 6], "JUST FOR KICKS");
        race_len = t == 1 ? ROAD_LEN : RACE_LEN;
        return 1;
    }
}
static char pbuf[30];
static void settle_race(void) BK {              /* desgaste, quebras, dinheiro, carros, vitorias e a mensagem do resultado */
    Car *c = &gc[cur];
    u8 win, ep, tpr, brk0 = 0, gone = 0, n;
    c->fuel = c->fuel > 3 ? c->fuel - 3 : 0;
    c->wear = c->wear + 100 + over_t * 30 > 10000 ? 10000 : c->wear + 100 + over_t * 30;
    c->tp = c->tp + 6 + (race_type == 1 ? 4 : 0) > 100 ? 100 : c->tp + 6 + (race_type == 1 ? 4 : 0);
    pm1 = pm2 = 0;
    if (result != 3) {                       /* quebras: motor (rotacao e desgaste), cambio (trocas com o motor passado), pneus (tabela do jogo) */
        ep = 2 + over_t * 6 + (u8)(c->wear / 500); if (ep > 85) ep = 85;
        tpr = 1 + badshift * 12; if (tpr > 85) tpr = 85;
#ifdef TEST_BRK
        brk0 = TEST_BRK;
        (void)ep; (void)tpr;
#else
        if (rnd16() % 100 < ep) brk0 = 1;
        else if (rnd16() % 100 < tpr) brk0 = 2;
        else if (rnd16() % 100 < pm_wtire[5 * c->tir + (c->tp >= 100 ? 4 : c->tp / 25)]) brk0 = 4;
#endif
        if (brk0) {
            c->brk |= brk0; result = 2;
            pm1 = brk0 == 1 ? "BLEW YOUR ENGINE AGAIN?" : brk0 == 2 ? "YOU JUST DROPPED YOUR" : "BLEW YOUR TIRES AGAIN?";
            pm2 = brk0 == 2 ? "TRANSMISSION, KLUTZ!" : "YOU LOSE. TAKE A WALK.";
        } else if (result == 2 && dbg_top >= 90 && rnd16() % 100 < 6) {      /* perdeu a alta velocidade: pode destruir o carro */
            u16 scrap = scrap_value(c);
            n = 0; put_str(pbuf, &n, "CAR TOTALED. $"); put_num(pbuf, &n, scrap); put_str(pbuf, &n, " SCRAP"); pbuf[n] = 0;
            money += scrap; remove_car(cur); gone = 1;
            pm1 = "LUCKY YOU WEREN'T KILLED!"; pm2 = pbuf;
        }
    }
    win = result == 1;
    if (!pm1) {
        pm1 = win ? "YOU WON!" : result == 3 ? "YA JUMPED THE LIGHT," : "YOU LOSE, SUCKER.";
        pm2 = win ? "TOO BAD YOU DIDN'T BET" : result == 3 ? "SPEEDY!" : "TAKE A WALK.";
    }
    if (win && race_type < 2) { if (race_type == 0) wins_drag++; else wins_road++; }
    if (win && sel_pil == NPILOTS - 1 && race_type != 2) game_end = 2;            /* venceu The King */
    if (race_type == 3) {
        if (win) {
            pm1 = "YOU JUST WON A"; pm2 = "HELLUVA CAR!";
            if (nown < MAXCARS) { car_stock(&gc[nown], sel_opp); nown++; }
            else { money += car_price[sel_opp] / 2; pm2 = "HELLUVA CAR! (SOLD IT)"; }
        } else {
            if (!gone) remove_car(cur);
            pm1 = "YOU LOST YOUR CAR!"; pm2 = "PINK SLIP GONE";
        }
    } else if (bet) {
        if (win) {
            money += bet;
            pm1 = bet == 10 ? "BIG DEAL." : bet == 25 ? "TWENTY-FIVE BUCKS." : bet == 50 ? "FIFTY SMACKERS." : "A HUNDRED SMACKERS,";
            pm2 = bet == 10 ? "TEN LOUSY BUCKS." : bet == 25 ? "NOT BAD." : bet == 50 ? "NOT BAD." : "HOT SHOT.";
        } else money = money > bet ? money - bet : 0;
    }
    if (day < 62) day++;
    if (day >= 61 && !game_end) game_end = 1;
    dbg_money = money; dbg_day = day;
}
static void end_screen(void) BK {                 /* fim de jogo: mensagem e estatisticas, depois volta ao titulo */
    static const char *const l1[4] = {"", "YOU'RE DONE FER", "YOU DESERVE TO BE", "YOU'RE OUTTA DOUGH."};
    static const char *const l2[4] = {"", "THE SUMMER!", "KING OF THE COUNTY!", "LOOKS LIKE YOU'RE DONE"};
    u8 n;
    load_garage(); ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cmd_n = 0; cp = cmd_buf; hub_status();
    ui_note(l1[game_end], l2[game_end]);
    n = 0; put_str(info_buf[0], &n, "FINAL BANKROLL $"); put_num(info_buf[0], &n, money); info_buf[0][n] = 0;
    n = 0; put_str(info_buf[1], &n, "CARS OWNED "); put_num(info_buf[1], &n, nown); info_buf[1][n] = 0;
    n = 0; put_str(info_buf[2], &n, "DRAG RACES WON "); put_num(info_buf[2], &n, wins_drag); info_buf[2][n] = 0;
    n = 0; put_str(info_buf[3], &n, "ROAD RACES WON "); put_num(info_buf[3], &n, wins_road); info_buf[3][n] = 0;
    ui_get = info_get;
    ui_list("THE SUMMER OF 1963", 4, 0);
    game_end = 0;
}
static void run_race(void) BK;
static u8 street_trip(void) {                /* 1 = chegou ao Bobs; 0 = desistiu (ESC) */
    Car *c = &gc[cur]; u8 gas = 1;
    u16 avg;
    ui_note("CRUISING IN TOWN...", "MAX SPEED 45 MPH"); ui_nohint = 0;
    street_mode = 1; race_type = 1; street_res = 1;
    run_race();
    street_mode = 0;
    load_garage(); ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cmd_n = 0; cp = cmd_buf; hub_status();
    if (street_res) return 0;
    avg = race_ticks ? (u16)(sum_spd / race_ticks) : 0;
    if (avg > 50) gas++;                                    /* quem acelera gasta mais */
    c->fuel = c->fuel > gas ? c->fuel - gas : 0;
    if (street_tix) {
        u16 f = street_fine > money ? money : street_fine; u8 n = 0;
        money -= f;
        put_str(pbuf, &n, "FINE $"); put_num(pbuf, &n, f); pbuf[n] = 0;
        ui_note(street_tix > 1 ? "SPEEDING TICKETS!" : "SPEEDING TICKET!", pbuf);
        cmd_n = 0; cp = cmd_buf; hub_status();
    }
    if (c->fuel < 3) { ui_note("YOU'RE LOW ON GAS!", "FILL UP AT THE GARAGE"); cmd_n = 0; cp = cmd_buf; hub_status(); return 0; }
    return 1;
}
static u8 hub(void) BK {                       /* 0 = ir ao Bob's (corrida), 1 = sair para o titulo */
    static const char *const items[4] = {"YOUR CARS", "NEWSPAPER", "BOBS DRIVE-IN", "QUIT GAME"};
    u8 sel = 2, r;
    load_garage();
    ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cmd_n = 0; cp = cmd_buf; hub_status();
    if (pm1) { ui_note(pm1, pm2); pm1 = 0; cmd_n = 0; cp = cmd_buf; hub_status(); }
    for (;;) {
        if (game_end) return 1;
        if (!nown && money < 300 && !nspare) { game_end = 3; return 1; }
        ui_nohint = 1; r = ui_pick("MAIN MENU", items, 4, sel); ui_nohint = 0;
        if (r == 255) continue;
        sel = r;
        if (r == 0) cars_screen();
        else if (r == 1) newspaper();
        else if (r == 2) {
            if (!nown) ui_note("BOBS DRIVE-IN", "BUY A CAR FIRST!");
            else if (gc[cur].brk) ui_note("YOUR CAR'S NOT RUNNING,", "SPEEDY!");
            else if (gc[cur].fuel < 3) ui_note("YOU DON'T GOT ENOUGH GAS", "TO CRUISE DOWN TO BOBS");
            else if (street_trip() && bobs()) return 0;
        }
        else return 1;
        cmd_n = 0; cp = cmd_buf; hub_status();
    }
}

#pragma codeseg BANK1
static void new_race(void) {
    u8 k = (u8)(JIFFY & 7);
    set_track(race_type == 1);
    load_pt(0, &gc[cur]); load_car(1, sel_opp); opp_a = pil_a[sel_pil]; opp_b = pil_b[sel_pil]; opp_set = car_spr[sel_opp];
    g_reg = 12; g_val = paint_rb[k]; g_sh = paint_g[k]; pal_one();                    /* pintura do oponente (cor 12 da paleta) */
    over_t = badshift = 0;
    gear = 0; rpm = 1500; speed = 0; pos = 0; sub = 0; P = -300;
    vgear[1] = 0; vrpm[1] = 1500; vspd[1] = 0; opos = OPP_START; osub = 0;
    launched[0] = launched[1] = 0; adv_acc[0] = adv_acc[1] = 0;
    state = 0; cd_t = 0; result = 0; false_start = 0; race_ticks = 0; sum_spd = 0;
    beep_stage = 0; light_rest = 0;
    opp_wait = OPP_B * 3 + 6; gear_changed = 1; go_held = 1; up_held = dn_held = 1;   /* tecla ainda apertada nao conta como troca */
    dbg_t60 = dbg_t100 = dbg_top = 0;
    if (street_mode) {                                                       /* rua: sem oponente nem semaforo; o jogo comeca ja andando */
        race_len = STREET_LEN; state = 1; launched[0] = 1; opp_wait = 0xFFFF; opos = 0;
        street_cop = 0; street_fine = 0; street_tix = 0;
    }
}

static void run_race(void) BK {
    u16 last_j, t, dj, j0, fr0, tickacc = 0, nt, start_j;
    u8 shown = 0;
    u16 base; u8 k;
    dbg_frames = 0; dbg_t60 = 0; dbg_t100 = 0; dbg_top = 0;
    clip_l = VIEW_X0; clip_r = VIEW_X1;
    pal_load();
    new_race();
    init_static();
    ybase = 256;
    tickacc = 0;
    last_j = JIFFY; j0 = last_j; fr0 = 0; start_j = last_j;
    for (;;) {
        t = JIFFY; dj = t - last_j; last_j = t;
        if (dj > 12) dj = 12;
        read_input(); combine_input();
        if (state == 2) {                                   /* resultado: espera ESPACO / botao A */
            u8 go = (g_trig & 3) != 0;
            if (go && !go_held) return;                     /* volta ao hub */
            go_held = go;
            continue;
        }
        handle_input();
        snd_engine(1);
        if (state == 0) {
            if (beep_stage == 0 && cd_t >= GO_J / 3) { beep_stage = 1; beep(254, 9); }
            else if (beep_stage == 1 && cd_t >= 2 * GO_J / 3) { beep_stage = 2; beep(254, 9); }
        }
        dbg_in = (g_t0 ? 1 : 0) | (up_held << 1) | (false_start << 2) | (gear << 4);
        if (state == 0) {
            cd_t += dj;
            if (false_start) { result = 3; state = 2; show_result(); continue; }
            if (cd_t >= GO_J) { state = 1; launched[0] = 1; start_j = JIFFY; beep(127, 24); }
        } else {
            if (opp_wait > dj) opp_wait -= dj; else { opp_wait = 0; launched[1] = 1; }
        }
        /* fisica a 6,0688 Hz: acumula 6,0688/JIFFY_HZ ticks por jiffy (8.8) */
        tickacc += dj * tick_inc;
        nt = tickacc >> 8; tickacc &= 255;
        if (state == 1) { advance(dj); steer_frame(dj); }
        while (nt--) {
            veh_tick(0, in_throttle, in_brake);
            if (!street_mode) opp_ai();
            if (state == 1) { race_ticks++; sum_spd += speed; }
        }
        if (state == 1) {
            if (street_mode) {
                if (street_cop < 3 && pos >= cop_at[street_cop]) {          /* radar do guarda */
                    street_cop++;
                    if (speed > STREET_LIMIT) { street_fine += 5 + (speed - STREET_LIMIT) / 4; street_tix++; beep(60, 20); }
                }
                if (want_back()) { street_res = 1; psg(8, 0); psg(9, 0); psg(10, 0); beep_on = 0; return; }
                if (pos >= race_len) { street_res = 0; psg(8, 0); psg(9, 0); psg(10, 0); beep_on = 0; return; }
            }
            else if (opos == pos && (P - (OLANE * 56)) < 300 && (P - (OLANE * 56)) > -300) {   /* colisao */
                vrpm[0] -= vrpm[0] >> 4; vrpm[1] -= vrpm[1] >> 4;
            }
            if (!dbg_t60 && speed >= 60) dbg_t60 = (u16)(JIFFY - start_j);
            if (!dbg_t100 && speed >= 100) dbg_t100 = (u16)(JIFFY - start_j);
            if (speed > dbg_top) dbg_top = speed;
            if (!street_mode && (pos >= race_len || opos >= race_len)) {
                u16 pp = pos * 8 + sub, op = opos * 8 + osub;
                result = (pos >= race_len && pp >= op) ? 1 : 2;
#ifdef TEST_LOSE
                result = 2;
#endif
                state = 2; show_result();
                dbg_state = state;
                continue;
            }
        }
        dbg_rpm = rpm; dbg_gear = gear; dbg_state = state; dbg_opos = opos; dbg_ospeed = vspd[1];
        cmd_n = 0; cp = cmd_buf;
        project();
        base = ybase; ybase = base + WIN_Y;                 /* janela da estrada: y relativo ao para-brisa */
        push_fill(0, SKY_TOP, 256, Y[N_ROWS - 1] - SKY_TOP, 0);
        draw_road_asm();
        if (!street_mode) { draw_opp(); draw_lights(state == 0 || (state == 1 && (u16)(JIFFY - start_j) < 2 * jhz)); }
#if HAVE_COCKPIT
#ifndef NO_OV
        /* moldura da esquerda (porta/coluna A) por cima da estrada: HMMM de retangulos justos da copia limpa (pagina 2).
           A estrada nao desce alem do para-brisa (VIEW_BOT), entao o painel/capo nunca e sobrescrito. */
        for (k = 0; k < OV_N; k++) {
            i16 ox = ov_x[k] << 1, oy = ov_y[k];
            vc_w(0, ox); vc_w(2, 512 + WIN_Y + SKY_TOP + oy); vc_w(4, ox); vc_w(6, ybase + SKY_TOP + oy);
            vc_w(8, ov_w[k] << 1); vc_w(10, ov_h[k]); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
            vdp_cmd15();
        }
#endif
#endif
        ybase = base;
        clip_l = 0; clip_r = 256;
#ifndef NO_HUD
        draw_hud_page();
#endif
        clip_l = VIEW_X0; clip_r = VIEW_X1;
        dbg_cmds = cmd_n; dbg_speed = speed; dbg_pos = pos;
        vdp_wait();
#if VSYNC
        while (JIFFY == t) ;
#endif
        shown = (ybase != 0);
        vdp_reg(2, (shown << 5) | 0x1F);
        ybase = shown ? 0 : 256;
        dbg_frames++;
        if ((u16)(JIFFY - j0) >= jhz) {
            dbg_fps10 = (u16)(((u16)(dbg_frames - fr0) * (10u * jhz)) / (u16)(JIFFY - j0));
            fr0 = dbg_frames; j0 = JIFFY;
#ifdef DEBUG_HUD
            draw_hud();                                 /* contadores de depuracao: ~90 comandos VDP em 2 paginas = engasgo visivel */
            ybase = shown ? 0 : 256;
#endif
        }
    }
}

#pragma codeseg CODE
void main(void) {
    set_screen5();
    jhz = (*(volatile u8 *)0xFFE8 & 2) ? 50 : 60;        /* bit 1 do R#9 (espelho da BIOS): 1 = 50 Hz */
    tick_inc = jhz == 50 ? 31 : 26; adv_k = jhz == 50 ? 41 : 35; go_j = 3 * jhz;
    vdp_reg(8, (*(volatile u8 *)0xFFE7 | 0x02) & ~0x20);   /* sprites off (comandos mais rapidos); TP=0: cor 0 e preta, nao transparente (WebMSX/BIOS reais deixam TP=1) */
    vdp_reg(7, 0);                                       /* cor de fundo/borda = 0 */
#ifdef DRY
    g_dry = 1;
#endif
    install_hook();
    snd_init();
    for (;;) {
        pal_load();
        clip_l = VIEW_X0; clip_r = VIEW_X1;
        show_title();
        vdp_reg(2, 0x1F);
        dbg_state = 9;                                  /* 9 = tela inicial pronta (aguardando ESPACO) */
        wait_start();
        new_game();
        while (!hub()) {                                /* hub: garagem; 0 = Bobs Drive-In */
            pal_load();
            run_race();
            settle_race();
        }
        if (game_end) end_screen();                    /* o jogo acabou: depois do aviso, volta ao titulo */
    }
}
