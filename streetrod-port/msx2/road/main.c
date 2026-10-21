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
u8 cart_slot, g_s0, g_s1, g_reg, g_val, g_stick, g_sh, g_sgn, g_fc;
u16 g_lzsrc, g_vlo, g_palp, g_bk_src, g_bk_dst; u8 g_vhi, g_vpg, g_group, g_bk_bank, g_bk_len;
extern void bank_copy(void);
extern void spr_put(void);
extern u8 g_sx, g_sy;
extern void read_mouse(void);
extern i8 g_mdx, g_mdy;
static char nbc[28], nbp[21], nbd[11];
static const char *txt(char *buf, u16 ofs, u8 len) { g_bk_bank = TEXT_BANK; g_bk_src = 0x8000 + TEXT_BASE + ofs; g_bk_dst = (u16)buf; g_bk_len = len; bank_copy(); return buf; }
#define CARN(i) txt(nbc, TXT_CAR_OFS + (i) * 28, 28)
#define PARTN(i) txt(nbp, TXT_PART_OFS + (i) * 21, 21)
#define PILN(i) txt(nbd, TXT_PIL_OFS + (i) * 11, 11)
extern void lz_unpack(void);
extern void psg_w(void);
extern void key_get(void);
extern void sr_exec(void);
extern void bd_exec(void);
extern void get_cart_slot(void);
extern void fm_w(void);
extern void rd_slot(void);
extern void wr_slot(void);
extern void pal_one(void);
extern void pal_load2(void);
extern void vram_blit(void);
i16 g_a, g_r, g_fx, g_fy, g_fw, g_fh; u16 g_b;
extern void mul12(void);
extern void mul16x8(void);
extern void vdp_wait(void);
u8 rec[11]; u8 g_dry; i16 clip_l, clip_r;
u8 s_yt, s_rem, s_rc, s_kc, s_gc, s_mode, s_dash, t_y; i16 s_xlf, s_xrf, s_sl2, s_sr2, s_kw, t_xl, t_xr, t_w;
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
#define END_DAY 46                                  /* o calendario do original circula 15 de setembro: o verao acaba ali */
static u16 race_len = RACE_LEN;
#ifndef STREET_LEN
#define STREET_LEN 420
#endif
#define STREET_LIMIT 45                              /* mph: acima disso o guarda multa */
static u8 shown_id = 255, shown_tir, shown_paint, shown_var, shown_stk;       /* carro desenhado agora na garagem (255 = vazia) */
static u8 cvar, cstk;                                   /* variante (teto rebaixado + 2 * sem para-choques) e adesivo do carro sendo desenhado */
static u8 street_mode, street_res, street_cop;       /* street_res: 0 chegou ao Bobs, 1 desistiu (ESC) */
static u16 street_fine; static u8 street_tix;
static const u16 cop_at[3] = {110, 230, 350};
i8 curve_ram[256], hill_ram[256];                 /* pista da corrida atual (proj.s le estes arrays) */
#pragma codeseg BANK1
static void set_track(u8 road) {                  /* arrancada: reta e plana; estrada: a pista sintetica */
    u16 i;
    for (i = 0; i < 256; i++) { curve_ram[i] = road ? curve_tab[i] : 0; hill_ram[i] = road ? hill_tab[i] : 0; }
}
#pragma codeseg CODE
#ifndef OPP_START
#define OPP_START 0            /* teste: posicao inicial do oponente (segmentos) */
#endif
/* powertrain dos 2 veiculos (0 = jogador, 1 = oponente), copiado das tabelas de carros (cars.h) ao iniciar a corrida */
static u16 pt_ratio[10], pt_hi0[10], pt_hi1[10], pt_acc[30], pt_redline[2];
static u8 pt_ngears[2], pt_auto[2], opp_a, opp_b, opp_set;
#define MAXCARS 4
typedef struct { u8 id, eng, trn, man, carb, tir; i8 trim; u16 wear; u8 fuel, brk, tp, paint, roof, bump, stick; } Car;   /* carro do jogador: variantes instaladas, ajuste do ponto, desgaste (0..10000) */
static u16 money; static u8 day, nown, cur; static Car gc[MAXCARS];     /* estado do jogo */
static u16 rnd16(void) { static u16 s = 0x1D2B; s = s * 25173u + 13849u; s ^= JIFFY; return s; }
#define MAXSPARE 12
static u8 spare[MAXSPARE], nspare, bay_bad = 255;      /* bay_bad: carro (indice) com peca/parafuso solto no motor: nao anda; 255 = nenhum */
static u8 offer_day = 255;
static char pname[13];                       /* nome do jogador (carteira DMV); vazio = SPEEDY */
static u8 ask_name;                          /* 1 = pedir o nome (jogo novo) */
static u8 ask_load;                          /* 1 = no comeco do jogo, oferecer o carregamento de um jogo salvo */
static u8 game_end;                          /* 0 jogando, 1 acabou o verao, 2 rei do condado, 3 faliu */
static u8 wins_drag, wins_road;
static u8 race_road, races_tot, king_st;         /* race_road: pista de estrada; races_tot: corridas valendo (fora as por diversao); king_st: 1 = ja venceu o Rei na arrancada */
static i8 pil_net[22];                           /* por piloto: 2 x (vezes que ele ganhou) - (corridas valendo contra ele): quem te venceu topa de novo */
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
    c->trim = 0; c->wear = 0; c->fuel = 10; c->brk = 0; c->tp = 0; c->paint = (u8)((id * 3 + 1) & 7); c->roof = c->bump = c->stick = 0;      /* fuel em galoes (max 20); brk: 1 motor, 2 cambio, 4 pneus; tp: desgaste dos pneus 0..100 */
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
    if (c == 0 && ng > vgear[c] && vrpm[0] > pt_redline[0] + 300 && badshift < 250 && !pt_auto[0]) badshift++;
    r = (u32)vspd[c] * pt_ratio[c * 5 + ng];
    if (ng && r > 5700) { if (ng > vgear[c]) { r = 5700; } else return; }   /* reducao com rotacao excessiva: recusada */
    vgear[c] = ng; vrpm[c] = (u16)r;
    if (c == 0) gear_changed = 1;
}

extern volatile u8 ev_space, ev_shift;
extern void install_hook(void);

#pragma codeseg CODE
static u8 mus_st = 1, mus_hold;                 /* radio: estacao 1..5 (0 = desligado); mus_hold = corrida em andamento (o PSG e do motor) */
static void mus_tick(void) BK;
static void combine_input(void) {
    u8 e;
    if (mus_st && !mus_hold) mus_tick();
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
    if (street_mode) { in_throttle = speed < 40; in_brake = speed > 44; in_left = in_right = 0; }      /* a ida ao Bob's e automatica: o carro dirige sozinho (45 mph) */
#if !AUTOPILOT
    if (state != 2 && !pt_auto[0] && !street_mode) {
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
    if (c == 0 && r > pt_redline[0] + 50 && over_t < 250 && g < n && !pt_auto[0]) over_t++;      /* so conta rotacao acima do corte se ainda ha marcha para engatar (na ultima nao ha o que fazer) e no cambio manual (no automatico e quase impossivel estourar o motor) */
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
        else if (c == 0 && street_mode) lim = cap / 100 * 97;          /* cambio manual na rua: o piloto automatico troca por voce */
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
#if !AUTOPILOT
    if (street_mode) {
        tgt = (xoff_ahead >> 1) - 300; e = (tgt - P) >> 2;
        if (e > 40) e = 40; if (e < -40) e = -40;
        P += e * (i16)dj;
    } else {
        if (in_left) d -= 1;
        if (in_right) d += 1;
        P += d * (i16)(4 + (speed >> 4)) * (i16)dj;
    }
#endif
#if AUTOPILOT
    tgt = (xoff_ahead >> 1) - 300;
    e = (tgt - P) >> 2;
    if (e > 40) e = 40; if (e < -40) e = -40;
    P += e * (i16)dj;
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
#define vc_w(i, v) do { i16 vw_ = (i16)(v); vc[i] = (u8)vw_; vc[(i) + 1] = (u8)(vw_ >> 8); } while (0)      /* macro: sem chamada de funcao (comandos de VDP diretos sao frequentes) */

static void draw_scn(void) BK;
#pragma codeseg BANK1
#include "assets_race.h"                  /* tabelas so da corrida (sprites do oponente, ponteiros): nos dados do banco 1, fora do nucleo */
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
#pragma codeseg BANK1
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
#pragma codeseg CODE

/* instrumentos do cockpit: ponteiros (LINE) sobre o fundo restaurado da pagina 2 (HMMM); so redesenha quando o valor muda
   (cache por pagina, pois ha 2 paginas alternadas) */
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
/* manopla de cambio (itens 227-232 da LIB2): o painel (3 marchas, 4 marchas ou automatico) vai para as 3 paginas no inicio da corrida e o botao anda pela grade
   (ate a barra do meio, ao longo dela e para a marcha) em passos de 3 px por quadro; so redesenha (fundo limpo da pagina 2 + LMMM) quando muda de lugar */
#define GPX 212
#define GPY 122
#define GSRC_Y 972
static u8 gk_type;                              /* 0 automatico, 1 manual de 3 marchas, 2 de 4 */
static i16 gk_x[2], gk_y[2], gk_dx[2], gk_dy[2]; /* posicao do centro do botao no painel (automatico: y da alavanca) e o ultimo desenho em cada pagina */
static i16 toward(i16 v, i16 t, i16 s) { return v < t ? (t - v < s ? t : v + s) : v > t ? (v - t < s ? t : v - s) : v; }
static void gear_init(void) {
    u8 p, rg1 = *(volatile u8 *)0xF3E0;
    gk_type = pt_auto[0] ? 0 : pt_ngears[0] > 3 ? 2 : 1;
    vdp_reg(1, rg1 & ~0x40);
    g_group = 60 + gk_type; load_assets();
    for (p = 0; p < 3; p++) { vc_w(0, GPX); vc_w(2, GSRC_Y); vc_w(4, GPX); vc_w(6, p * 256 + GPY); vc_w(8, 38); vc_w(10, 43); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0; vdp_cmd15(); }
    vdp_wait();
    g_group = 63 + gk_type; load_assets();
    vdp_reg(1, rg1 | 0x40);
    for (p = 0; p < 2; p++) { gk_x[p] = gk_type ? 20 : 8; gk_y[p] = gk_type ? 19 : 21; gk_dx[p] = -1; }
}
static void gear_knob(u8 pg) {
    i16 x = gk_x[pg], y = gk_y[pg], tx, ty;
    if (gk_type == 0) { tx = 8; ty = gear ? 26 : 21; y = toward(y, ty, 3); }                 /* alavanca: N no meio, D embaixo */
    else {
        tx = gear < 3 ? 20 : 27; ty = gear == 0 ? 19 : (gear & 1) ? 12 : 27;                /* 1 e 3 em cima, 2 e 4 embaixo */
        if (x != tx && y != 19) y = toward(y, 19, 3); else if (x != tx) x = toward(x, tx, 3); else y = toward(y, ty, 3);
    }
    gk_x[pg] = x; gk_y[pg] = y;
    if (x == gk_dx[pg] && y == gk_dy[pg]) return;
    gk_dx[pg] = x; gk_dy[pg] = y;
    vc_w(0, GPX); vc_w(2, 512 + GPY); vc_w(4, GPX); vc_w(6, ybase + GPY); vc_w(8, 38); vc_w(10, 43); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;      /* painel limpo (pagina 2) */
    vdp_cmd15();
    vc_w(0, 0); vc_w(2, GSRC_Y); vc_w(4, gk_type ? GPX + x - 10 : GPX + x); vc_w(6, ybase + GPY + y - (gk_type ? 9 : 4)); vc_w(8, 20); vc_w(10, gk_type ? 19 : 8); vc[12] = 0; vc[13] = 0; vc[14] = 0x98;      /* LMMM: cor 0 transparente */
    vdp_cmd15();
}
static void draw_hud_page(void) {
    u8 pg = ybase != 0;
    u16 sp = speed > NEEDLE_SPD_N - 1 ? NEEDLE_SPD_N - 1 : speed, rp = rpm / 50;
    if (rp > NEEDLE_RPM_N - 1) rp = NEEDLE_RPM_N - 1;
    if (hud_c[pg][0] != sp) { hud_c[pg][0] = sp; gauge(GAUGE_SPD_X, GAUGE_SPD_Y, needle_spd[sp * 2], needle_spd[sp * 2 + 1]); }
    if (hud_c[pg][1] != rp) { hud_c[pg][1] = rp; gauge(GAUGE_RPM_X, GAUGE_RPM_Y, needle_rpm[rp * 2], needle_rpm[rp * 2 + 1]); }
    (void)pg;
    gear_knob(pg);
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
    hud_reset();
    gear_init();
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

#pragma codeseg BANK3
/* ---- radio: 5 estacoes (music_gen.py) tocadas no PSG; 1 linha = 3 bytes (A melodia, B baixo, C harmonia/percussao), lidas do banco da estacao ---- */
static u16 mus_row, mus_due, mus_dur, mus_t0[3], mus_noise;       /* linha atual, jiffy da proxima, duracao da linha (8.8 jiffies), inicio de cada nota, fim do ruido */
static u8 mus_fr, mus_on[3], mus_lv[3], mus_fm, mus_hi[3], mus_dp;      /* mus_fm: 1 = ha MSX-MUSIC (OPLL), toca em FM; mus_hi: bloco/F9 do canal; mus_dp: bateria da linha anterior */
static const u8 mus_env[3][3] = {{12, 7, 6}, {13, 10, 8}, {9, 3, 4}};   /* PSG, por canal: volume inicial, piso, jiffies por degrau */
static const u8 mus_ins[5][3] = {{0xF3, 0xE2, 0x26}, {0xF3, 0xE2, 0x36}, {0x75, 0xE2, 0x36}, {0x43, 0xE2, 0x36}, {0xF3, 0xE2, 0x26}};   /* FM: instrumento<<4 | atenuacao (A, B, C) por estacao */
static void fm(u8 r, u8 v) { g_reg = r; g_val = v; fm_w(); }
static void fm_off(void) { u8 c; for (c = 0; c < 3; c++) fm(0x20 + c, mus_hi[c]); fm(0x0E, 0x20); mus_dp = 0; }
static u16 mus_tab(u8 v, u8 fm) {                 /* periodo PSG (fm = 0) ou numero F do OPLL (fm = 1) da nota v: tabelas no banco de assets (MT_BANK) */
    u16 t;
    g_bk_bank = MT_BANK; g_bk_src = 0x8000 + MT_BASE + (fm ? 256 : 0) + 2 * v; g_bk_dst = (u16)&t; g_bk_len = 2; bank_copy();
    return t;
}
static void mus_row_play(void) {
    u8 b[4], c, v, d; u16 per;
    g_bk_bank = mus_bank[mus_st - 1]; g_bk_src = 0x8000 + mus_row * 4; g_bk_dst = (u16)b; g_bk_len = 4; bank_copy();
    d = b[3];
    if (mus_fm) {
        for (c = 0; c < 3; c++) {
            v = b[c];
            if (v >= 12) {
                per = mus_tab(v, 1); mus_hi[c] = (u8)(per >> 8);
                fm(0x20 + c, mus_hi[c]); fm(0x10 + c, (u8)per); fm(0x20 + c, mus_hi[c] | 0x10);
            } else if (v == 1) fm(0x20 + c, mus_hi[c]);
        }
        if (mus_dp) fm(0x0E, 0x20);
        if (d) fm(0x0E, 0x20 | d);
        mus_dp = d;
        return;
    }
    for (c = 0; c < 3; c++) {
        v = b[c];
        if (v >= 12) {
            per = mus_tab(v, 0); psg(2 * c, (u8)per); psg(2 * c + 1, (u8)(per >> 8));
            mus_t0[c] = JIFFY; mus_on[c] = 1; mus_lv[c] = 255;
            if (c == 2 && mus_noise) { psg(7, 0xB8); mus_noise = 0; }
        } else if (v == 1) { mus_on[c] = 0; mus_lv[c] = 0; psg(8 + c, 0); }
    }
    if (d & 15) {                                                         /* bateria no PSG: ruido no canal C (caixa longa; chimbal, prato e tom curtos); o bumbo fica de fora */
        psg(6, (d & 8) ? 14 : 4); psg(7, 0x9C); psg(10, (d & 8) ? 12 : 8);
        mus_noise = JIFFY + ((d & 8) ? 5 : 2); mus_on[2] = 0;
    }
}
static void mus_tick(void) BK {
    u8 c, v, age; i16 d;
    while ((d = (i16)(JIFFY - mus_due)) >= 0) {
        u16 t;
        if (d > 40) mus_due = JIFFY;                                      /* ficou muito atras (carga de tela): recomeca o relogio */
        mus_row_play();
        if (++mus_row >= mus_rows[mus_st - 1]) mus_row = 0;
        t = (u16)mus_fr + (u8)mus_dur; mus_fr = (u8)t;
        mus_due += (mus_dur >> 8) + (t >> 8);
    }
    if (mus_fm) return;
    if (mus_noise && (i16)(JIFFY - mus_noise) >= 0) { psg(10, 0); psg(7, 0xB8); mus_noise = 0; }
    for (c = 0; c < 3; c++) if (mus_on[c]) {
        age = JIFFY - mus_t0[c]; if (age > 120) age = 120;
        v = mus_env[c][0]; age /= mus_env[c][2];
        v = v > mus_env[c][1] + age ? v - age : mus_env[c][1];
        if (v != mus_lv[c]) { mus_lv[c] = v; psg(8 + c, v); }
    }
}
static u8 fm_sig(u8 slot) {                                               /* 1 = ha a ROM do MSX-MUSIC (APRLOPLL) ou do FM-PAC (PAC2OPLL) em 0x4018 deste slot */
    static const char ap[9] = "APRLOPLL", pc[9] = "PAC2OPLL";
    u8 i, a = 1, p = 1;
    for (i = 0; i < 8; i++) {
        g_bk_bank = slot; g_bk_src = 0x4018 + i; rd_slot();
        if (g_val != ap[i]) a = 0;
        if (g_val != pc[i]) p = 0;
        if (!a && !p) return 0;
    }
    if (p) { g_bk_bank = slot; g_bk_src = 0x7FF6; g_val = 1; wr_slot(); }   /* FM-PAC: liga as portas do OPLL */
    return 1;
}
static void fm_detect(void) BK {
    u8 p, s, sl;
    mus_fm = 0;
    for (p = 0; p < 4 && !mus_fm; p++) {
        if (*(volatile u8 *)(0xFCC1 + p) & 0x80) { for (s = 0; s < 4 && !mus_fm; s++) { sl = 0x80 | (s << 2) | p; mus_fm = fm_sig(sl); } }
        else mus_fm = fm_sig(p);
    }
#ifdef NO_FM
    mus_fm = 0;
#endif
#ifdef FORCE_FM
    mus_fm = 1;
#endif
}
static void mus_pause(void) BK {                                          /* corrida: sem musica (o PSG e do motor; o FM e silenciado) */
    mus_hold = 1; if (mus_fm) fm_off(); mus_noise = 0; mus_on[0] = mus_on[1] = mus_on[2] = 0;
    psg(8, 0); psg(9, 0); psg(10, 0); psg(7, 0xB8);
}
static void mus_set(u8 st) BK {                                           /* troca de estacao (0 = desliga) */
    u16 x; u8 c;
    psg(8, 0); psg(9, 0); psg(10, 0); psg(7, 0xB8); mus_noise = 0; mus_on[0] = mus_on[1] = mus_on[2] = 0;
    if (mus_fm) {
        fm_off();
        if (st) {
            for (c = 0; c < 3; c++) fm(0x30 + c, mus_ins[st - 1][c]);
            fm(0x16, 0x20); fm(0x26, 0x05); fm(0x17, 0x50); fm(0x27, 0x05); fm(0x18, 0xC0); fm(0x28, 0x01);          /* afinacao dos canais de ritmo */
            fm(0x36, 0x03); fm(0x37, 0x53); fm(0x38, 0x45);                                                           /* volumes: bumbo; chimbal+caixa; tom+prato */
        }
    }
    mus_st = st;
    if (!st) return;
    mus_row = 0; mus_due = JIFFY; mus_fr = 0;
    x = mus_ms[st - 1] * (u16)jhz;
    mus_dur = (x / 125u) * 32u + ((x % 125u) * 32u) / 125u;               /* ms -> jiffies em 8.8 */
}
#pragma codeseg CODE
static void race_silence(void) { psg(8, 0); psg(9, 0); psg(10, 0); beep_on = 0; }

#pragma codeseg BANK2
static void title_screen(void) BK {            /* "California Dreams presents" (LIB1 item 2) e o titulo (item 0), paleta EGA; segue com clique/ESPACO */
    u8 rg1 = *(volatile u8 *)0xF3E0, held = 1, go; u16 j;
    clip_l = 0; clip_r = 256; ybase = 0;
    vdp_reg(1, rg1 & ~0x40);
    g_palp = (u16)pal_ega; pal_load2();
    g_group = 250; load_assets(); vdp_reg(2, 0x1F); vdp_reg(1, rg1 | 0x40);
    j = JIFFY;
    while ((u16)(JIFFY - j) < 2 * (u16)jhz) {                       /* 2 s ou um clique */
        read_input(); combine_input(); go = (g_trig & 3) != 0;
        if (go && !held) break;
        held = go;
    }
    vdp_reg(1, rg1 & ~0x40); g_group = 251; load_assets(); vdp_reg(1, rg1 | 0x40);
    clip_l = VIEW_X0; clip_r = VIEW_X1;
    dbg_state = 9;                                                   /* 9 = tela inicial pronta (aguardando ESPACO) */
    held = 1;
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
#ifndef START_END
#define START_END 0
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
    u8 i;
    money = START_MONEY; day = START_DAY; wins_drag = WINS0; wins_road = 0; races_tot = 0; king_st = 0; race_road = 0; for (i = 0; i < 22; i++) pil_net[i] = 0; game_end = START_END; nown = 0; cur = 0; sel_car = 0; offer_day = 255; nspare = 0; bay_bad = 255; shown_id = 255; ask_load = 1; ask_name = 1;
#ifdef NO_NAME
    ask_name = 0;                                                                   /* testes automatizados nao digitam o nome */
#endif
#ifdef START_CAR
    car_stock(&gc[0], START_CAR); nown = 1; sel_car = START_CAR;                  /* so para testes automatizados */
#endif
}

#pragma codeseg CODE
static void font_init(void) BK;
static void load_garage(void) {             /* cena da garagem (LIB2 item 32) nas paginas 0/1/2, paleta propria */
    u8 rg1 = *(volatile u8 *)0xF3E0;
    vdp_reg(1, rg1 & ~0x40);
    g_palp = (u16)pal_garage; pal_load2();
    g_group = 1; load_assets();
    font_init();
    vdp_reg(1, rg1 | 0x40);
}
static void hub_restore(i16 x, i16 y, i16 w, i16 h) {   /* devolve um pedaco da cena limpa (copia na pagina 2) */
    vc_w(0, x); vc_w(2, 512 + y); vc_w(4, x); vc_w(6, y); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}
static void lmmm(i16 sx, i16 sy, i16 dx, i16 dy, i16 w, i16 h) {
    vc_w(0, sx); vc_w(2, sy); vc_w(4, dx); vc_w(6, dy); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0x98;   /* TIMP: cor 0 transparente */
    vdp_cmd15();
}
/* ---- fonte da garagem: a fonte 6x8 da BIOS (CGTBL) copiada para a pagina 3 da VRAM (preta e branca) e desenhada por LMMM ---- */
#define FONT_Y 976
#define HC_K 1          /* preto */
#pragma codeseg BANK1
static void font_init(void) BK {
    const u8 *cg = (const u8 *)(*(volatile u16 *)0x0004);
    u8 g, r, b, x, bits; u16 o, off;
    for (o = 0; o < 6144; o++) cmd_buf[o] = 0;
    for (g = 0; g < 96; g++) {
        u8 gx = (g & 31) * 6, gy = (g >> 5) * 8;
        for (r = 0; r < 8; r++) {
            bits = cg[(u16)(g + 32) * 8 + r];
            for (b = 0; b < 6; b++) if (bits & (0x80 >> b)) {
                x = gx + b; off = (u16)(gy + r) * 128 + (x >> 1);
                cmd_buf[off] |= (x & 1) ? 0x01 : 0x10;                      /* preto (indice 1) nas linhas 0-23 */
                off += 3072; cmd_buf[off] |= (x & 1) ? 0x04 : 0x40;         /* branco (indice 4) nas linhas 24-47 */
            }
        }
    }
    g_vlo = 0xE800; g_vhi = 1; g_vpg = 24; vram_blit();
}
#pragma codeseg CODE
static void gtext(i16 x, i16 y, const char *s, u8 c) {      /* c: 0 = preto, 1 = branco; avanca 6 px por letra */
    i16 sy0 = FONT_Y + c * 24; u8 ch;
    for (; *s; s++, x += 6) {
        ch = *s - 32; if (ch > 95) ch = 0;
        lmmm((ch & 31) * 6, sy0 + (ch >> 5) * 8, x, y + ybase, 6, 8);
    }
}
static u8 num_str(char *d, u16 v) {         /* decimal sem zeros a esquerda; devolve o tamanho */
    char t[6]; u8 n = 0, k = 0;
    do { t[k++] = '0' + v % 10; v /= 10; } while (v);
    while (k) d[n++] = t[--k];
    return n;
}
static u8 bar_news;                          /* 1 = jornal: a barra de status leva os botoes SELL PARTS e EXIT (clicaveis com o mouse) */
static void hub_status(void) {              /* barra branca: dinheiro, data e carro (texto preto, como o visor da cena) */
    static const char mon[2][4] = {"AUG", "SEP"};
    char b[24]; u8 mo = day >= 31, n = 0, pg;
    for (pg = 0; pg < 2; pg++) {                    /* nas paginas 0 e 1: a animacao do carro alterna as duas, e a barra nao pode piscar */
    ybase = pg ? 256 : 0;
    n = 0;
    push_fill(0, 190, 256, 22, HC_W);
    b[n++] = '$'; n += num_str(b + n, money); b[n] = 0; gtext(6, 192, b, 0);
    n = 0; b[n++] = mon[mo][0]; b[n++] = mon[mo][1]; b[n++] = mon[mo][2]; b[n++] = ' '; n += num_str(b + n, mo ? day - 30 : day + 1);
    b[n++] = ','; b[n++] = ' '; b[n++] = '1'; b[n++] = '9'; b[n++] = '6'; b[n++] = '3'; b[n] = 0; gtext(96, 192, b, 0);
    if (bar_news) {
        if (bar_news == 1) { push_fill(132, 201, 76, 10, HC_K); push_fill(133, 202, 74, 8, HC_W); gtext(136, 202, "SELL PARTS", 0); }
        push_fill(214, 201, 36, 10, HC_K); push_fill(215, 202, 34, 8, HC_W); gtext(226, 202, "EXIT", 0);
    } else gtext(6, 203, nown ? CARN(gc[cur].id) : "NO CAR - BUY ONE", 0);
    vdp_wait();
    }
    ybase = 0;
}
#ifndef CUR_X
#define CUR_X 128
#define CUR_Y 110
#endif
static u8 cur_x = CUR_X, cur_y = CUR_Y, cur_ymax = 187, mouse_seen;
#pragma codeseg BANK1
/* cursor: a chave de boca do original (16x16, ponta no alto a esquerda), 2 planos: contorno vermelho escuro (cor 10) e miolo vermelho (cor 11); pixels medidos nas capturas do jogo original */
static const u16 cur_wa[16] = {0x2000,0x1000,0x8800,0x8800,0x8800,0x7400,0x0A00,0x0500,0x0280,0x0140,0x00A0,0x005C,0x0022,0x0020,0x0020,0x001C};
static const u16 cur_wb[16] = {0x1800,0x0C00,0x0400,0x4400,0x7600,0x0B00,0x0580,0x02C0,0x0160,0x00B0,0x005E,0x0023,0x0011,0x0010,0x0018,0x0000};
static void cursor_init(void) BK {                /* padroes 0 (miolo) e 4 (contorno) 16x16 + cores por linha, na VRAM livre apos a pagina 0 */
    u8 i;
    for (i = 0; i < 255; i++) cmd_buf[i] = 0;
    cmd_buf[255] = 0;
    for (i = 0; i < 16; i++) {
        cmd_buf[i] = (u8)(cur_wb[i] >> 8); cmd_buf[16 + i] = (u8)cur_wb[i];
        cmd_buf[32 + i] = (u8)(cur_wa[i] >> 8); cmd_buf[48 + i] = (u8)cur_wa[i];
    }
    g_vlo = 0x7800; g_vhi = 0; g_vpg = 1; vram_blit();
    for (i = 0; i < 32; i++) cmd_buf[i] = 0;
    for (i = 0; i < 16; i++) { cmd_buf[i] = 11; cmd_buf[16 + i] = 10; }
    g_vlo = 0x7400; vram_blit();
}
#pragma codeseg CODE
static void spr_on(void) {
#ifdef NO_SPR
    return;
#endif
    vdp_reg(1, *(volatile u8 *)0xF3E0 | 0x02); vdp_reg(8, *(volatile u8 *)0xFFE7 & ~0x22); }
static void spr_off(void) { g_sy = 0xD8; spr_put(); vdp_reg(8, (*(volatile u8 *)0xFFE7 | 0x02) & ~0x20); }
static u8 mouse_move(void) {                     /* mouse na porta 1: aplica o deslocamento ao cursor; 1 = mexeu */
    i16 nx, ny;
#ifdef TEST_MOUSE                                  /* testes: setas = deslocamento do mouse (4 px), ESPACO = botao esquerdo */
    g_mdx = (g_stick >= 2 && g_stick <= 4) ? 4 : (g_stick >= 6 && g_stick <= 8) ? -4 : 0;
    g_mdy = (g_stick == 8 || g_stick == 1 || g_stick == 2) ? -4 : (g_stick >= 4 && g_stick <= 6) ? 4 : 0;
    if (g_trig & 1) g_trig |= 2;
    { u16 j = JIFFY; while (JIFFY == j) ; }
#else
    read_mouse();
#endif
    if ((g_mdx || g_mdy) && g_mdx > -64 && g_mdx < 64 && g_mdy > -64 && g_mdy < 64) {
        nx = cur_x + g_mdx; ny = cur_y + g_mdy;
        cur_x = nx < 1 ? 1 : nx > 254 ? 254 : (u8)nx; cur_y = ny < 0 ? 0 : ny > cur_ymax ? cur_ymax : (u8)ny;
        mouse_seen = 1; return 1;
    }
    return 0;
}
/* menu vertical sobre a cena: devolve o item escolhido (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_X 8
#define UI_W 240
/* lista rolavel (ate UI_VIS linhas visiveis); o texto de cada item vem de ui_get(i). Devolve o indice (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_VIS 8
#define UI_RH 12
static u8 ui_nohint;                         /* 1 = menu raiz (sem voltar): nao mostra a dica ESC */
static const char *(*ui_get)(u8 i);
static const char *const *ui_arr;
static const char *ui_arr_get(u8 i) { return ui_arr[i]; }
static char tb[32];
static void ui_row(u8 i, i16 y0, u8 top, u8 sel) {
    i16 y = y0 + 20 + UI_RH * (i - top);
    push_fill(UI_X + 4, y - 2, UI_W - 8, UI_RH, i == sel ? HC_K : HC_W);          /* selecionado: barra preta, texto branco */
    gtext(UI_X + 10, y, ui_get(i), i == sel);
}
static u8 ul_n, ul_vis, ul_top, ul_sel, ui_dbg = 7;     /* ui_dbg: valor de dbg_state enquanto a janela esta aberta (2 = resultado da corrida, para os testes) */ static i16 ul_y0;
static void ui_box(i16 x, i16 y, i16 w, i16 h, const char *t) {          /* botao: moldura preta, fundo branco */
    push_fill(x, y, w, h, HC_K); push_fill(x + 1, y + 1, w - 2, h - 2, HC_W); gtext(x + 4, y + 1, t, 0);
}
static void ul_set(u8 ns) {                      /* muda o item selecionado (rolando a janela se preciso) */
    u8 i;
    if (ns == ul_sel || ns >= ul_n) return;
    cmd_n = 0; cp = cmd_buf;
    if (ns < ul_top || ns >= ul_top + ul_vis) {
        ul_top = ns < ul_top ? ns : ns - ul_vis + 1; ul_sel = ns;
        for (i = ul_top; i < ul_top + ul_vis; i++) ui_row(i, ul_y0, ul_top, ul_sel);
    } else {
        i = ul_sel; ul_sel = ns; ui_row(i, ul_y0, ul_top, ul_sel); ui_row(ul_sel, ul_y0, ul_top, ul_sel);
    }
    vdp_wait();
}
#define BTN_X (UI_X + UI_W - 18)                 /* botoes da barra de titulo: X (voltar), v e ^ (rolar) */
static u8 ui_list(const char *title, u8 n, u8 sel) {
    u8 prev = 0xFF, i, kheld = 1, mheld = 1, bheld = 1, shown = 0;
    i16 h, y0;
    ul_n = n; ul_vis = n < UI_VIS ? n : UI_VIS; ul_top = 0; ul_sel = sel;
    h = 24 + UI_RH * ul_vis; y0 = 186 - h; ul_y0 = y0;        /* o menu fica no chao da garagem (a parte baixa da cena) */
    if (!n) return 255;
    if (sel >= ul_vis) ul_top = sel - ul_vis + 1;
    clip_l = 0; clip_r = 256; ybase = 0; dbg_state = ui_dbg; dbg_money = money; dbg_day = day; dbg_nown = nown; dbg_nspare = nspare;
    cmd_n = 0; cp = cmd_buf;
    hub_restore(0, 22, 256, 168);                /* apaga o menu anterior */
    push_fill(UI_X, y0, UI_W, h, HC_K);                         /* janela: moldura preta, fundo branco, titulo sublinhado */
    push_fill(UI_X + 2, y0 + 2, UI_W - 4, h - 4, HC_W);
    gtext(UI_X + 8, y0 + 6, title, 0);
    push_fill(UI_X + 6, y0 + 17, UI_W - 12, 1, HC_K);
    if (!ui_nohint) ui_box(BTN_X, y0 + 4, 12, 11, "X");           /* voltar com o mouse */
    if (n > ul_vis) { ui_box(BTN_X - 16, y0 + 4, 12, 11, "v"); ui_box(BTN_X - 32, y0 + 4, 12, 11, "^"); }
    push_fill(190, 190, 66, 11, HC_W);                           /* dica de voltar na barra de status (menu raiz nao tem) */
    if (!ui_nohint) gtext(202, 192, "ESC BACK", 0);
    for (i = ul_top; i < ul_top + ul_vis; i++) ui_row(i, y0, ul_top, ul_sel);
    vdp_wait();
    for (;;) {
        u8 st, back, ns = ul_sel, over = 255, moved;
        read_input(); combine_input();
        moved = mouse_move();
        st = g_stick; back = want_back();
#ifdef TEST_MOUSE
        st = 0;
#endif
        if (mouse_seen) {
            if (!shown) { shown = 1; spr_on(); }
            g_sx = cur_x - 3; g_sy = cur_y - 1; spr_put();
            if (cur_x >= UI_X + 4 && cur_x < UI_X + UI_W - 4 && cur_y >= y0 + 18 && cur_y < y0 + 18 + UI_RH * ul_vis) over = ul_top + (cur_y - y0 - 18) / UI_RH;
            if (moved && over != 255 && over != ul_sel) ul_set(over);          /* passar o cursor seleciona o item */
        }
        if ((g_trig & 2) && !mheld) {                           /* clique do mouse (botao esquerdo / A) */
            if (cur_y >= y0 + 4 && cur_y < y0 + 15 && mouse_seen) {
                if (!ui_nohint && cur_x >= BTN_X && cur_x < BTN_X + 12) { if (shown) spr_off(); return 255; }
                if (ul_n > ul_vis && cur_x >= BTN_X - 16 && cur_x < BTN_X - 4) ul_set(ul_sel + 1 < ul_n ? ul_sel + 1 : ul_sel);
                else if (ul_n > ul_vis && cur_x >= BTN_X - 32 && cur_x < BTN_X - 20) ul_set(ul_sel ? ul_sel - 1 : 0);
            } else if (over != 255 || !mouse_seen) { if (shown) spr_off(); return over != 255 ? over : ul_sel; }
        }
#ifndef TEST_MOUSE
        if ((g_trig & 1) && !kheld) { if (shown) spr_off(); return ul_sel; }
#endif
        if (back && !bheld) { if (shown) spr_off(); return 255; }
        kheld = (g_trig & 1) != 0; mheld = (g_trig & 2) != 0; bheld = back;
        if (st == prev) continue;
        prev = st;
        if (st == 1 && ul_sel > 0) ns = ul_sel - 1; else if (st == 5 && ul_sel + 1 < ul_n) ns = ul_sel + 1;
        ul_set(ns);
    }
}
static u8 ui_pick(const char *title, const char *const *it, u8 n, u8 sel) { ui_arr = it; ui_get = ui_arr_get; return ui_list(title, n, sel); }
static void ui_note(const char *a, const char *b) {   /* aviso de 2 linhas, ate ESPACO/SHIFT */
    const char *it[2]; it[0] = a; it[1] = b;
    ui_pick(a, it + 1, 1, 0);
}
/* "NOME....... $1234" em tb[] (nome cortado/preenchido em 21 colunas, preco em 4 digitos) */
#pragma codeseg BANK1
static const char *fmt_price(const char *name, u16 price) BK {
    u8 i = 0;
    while (i < 20 && name[i]) { tb[i] = name[i]; i++; }
    while (i < 21) tb[i++] = ' ';
    tb[i++] = '$';
    tb[i + 3] = '0' + price % 10; tb[i + 2] = price >= 10 ? '0' + (price / 10) % 10 : ' ';
    tb[i + 1] = price >= 100 ? '0' + (price / 100) % 10 : ' '; tb[i] = price >= 1000 ? '0' + (price / 1000) % 10 : ' ';
    tb[i + 4] = 0;
    return tb;
}
static const char *fmt_short(u16 v) BK {       /* "YOU ARE $123 SHORT" */
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

#pragma codeseg CODE
/* ---- inventario ---- */

/* ---- garagem: carros, pecas, ajuste do ponto, informacoes, venda ---- */
#pragma codeseg BANK2
#pragma codeseg BANK4
static const char *const cat_label[5] = {"ENGINE", "TRANS", "CARB", "MANIFOLD", "TIRES"};
static u8 *car_var(Car *c, u8 cat) { return cat == 0 ? &c->eng : cat == 1 ? &c->trn : cat == 2 ? &c->carb : cat == 3 ? &c->man : &c->tir; }
static u8 car_maker(const Car *c) { return car_flags[c->id] & 7; }        /* 1 = GM, 2 = Ford, 4 = Chrysler (mesma mascara das pecas) */
static u8 part_find(u8 cat, u8 var, u8 maker) {         /* id do catalogo da peca instalada (categoria, variante, montadora) */
    u8 i;
    for (i = 0; i < NPARTS; i++) if (part_cat[i] == cat && part_var[i] == var && (part_mk[i] & maker)) return i;
    return 255;
}
#pragma codeseg CODE
static void put_str(char *d, u8 *n, const char *s) { while (*s) d[(*n)++] = *s++; }
static void put_num(char *d, u8 *n, u16 v) {
    u16 p = 10000; u8 on = 0;
    for (; p; p /= 10) { u8 dg = (v / p) % 10; if (dg || on || p == 1) { d[(*n)++] = '0' + dg; on = 1; } }
}
#pragma codeseg BANK4
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
#pragma codeseg CODE
static u8 try_pay(u16 price) {            /* confere o dinheiro; mensagem se faltar */
    if (money < price) { ui_note("NO GO, HOT SHOT", fmt_short(price - money)); return 0; }
    money -= price; return 1;
}
#pragma codeseg BANK4
static u16 repair_cost(const Car *c) {                  /* motor/cambio/pneus quebrados: preco das pecas atuais */
    u16 t = 0;
    if (c->brk & 1) t += PART_PRICE(part_find(0, c->eng, car_maker(c)));
    if (c->brk & 2) t += PART_PRICE(part_find(1, c->trn, car_maker(c)));
    if (c->brk & 4) t += PART_PRICE(part_find(4, c->tir, 7));
    return t;
}
#pragma codeseg CODE
static u16 scrap_value(const Car *c) { return car_price[c->id] / 10; }
static void remove_car(u8 ci) {
    u8 n;
    for (n = ci; n + 1 < nown; n++) gc[n] = gc[n + 1];
    nown--;
    if (cur >= nown) cur = nown ? nown - 1 : 0; else if (cur > ci) cur--;
    if (nown) sel_car = gc[cur].id;
}
#pragma codeseg BANK4
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
#pragma codeseg CODE
static char info_buf[7][30];
static const char *info_get(u8 i) { return info_buf[i]; }
#pragma codeseg BANK4
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
static void cars_screen(void) BK {
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
#define NOFFERS 14
#pragma codeseg BANK2
static u8 offer[NOFFERS], offer_pct[NOFFERS]; static u16 offer_mask;
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
        if ((u8)(rnd16() >> 8) % 100 >= chance) { ui_note("GET LOST!", "NO DEAL"); offer_mask |= (u16)1 << i; return; }
        price = (u16)((u32)price * (90 - 10 * d) / 100);
        ui_note("I'LL TAKE IT!", "DEAL");
    }
    if (!try_pay(price)) return;
    car_stock(&gc[nown], offer[i]); gc[nown].wear = (u16)(rnd16() % 3000);         /* usado: 0 a 30% de desgaste */
    nown++; offer_mask |= (u16)1 << i;
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
#ifdef OLD_MENU
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
#endif
/* ---- Bob's Drive-In: desafiar um piloto (arrancada / estrada / so por diversao / pink slip), apostas ---- */
#pragma codeseg BANK3
static const char *const paint_name[12] = {"PURPLE", "RED", "BLUE", "GREEN", "YELLOW", "SILVER", "ORANGE", "TURQUOISE", "BLACK", "WHITE", "PINK", "BROWN"};
static void credits(void) BK;
static const char *const stk_name[9] = {"NO STICKER", "BANANA CLUB", "EXPLOSIVE", "STAR COP", "N.E.D.", "VETERAN", "FOR SALE", "EAGLE", "USA 1"};
static void body_shop(void) BK {                 /* pintura, adesivos, teto rebaixado, para-choques (uma compra por visita: o carro e redesenhado ao sair) */
    static const char *it[5];
    Car *c; u8 r, k;
    if (!nown) { ui_note("NO CAR TO WORK ON", "BUY ONE FIRST!"); return; }
    c = &gc[cur];
    it[0] = "NEW PAINT JOB $20"; it[1] = "STICKERS $5"; it[2] = c->roof ? "REBUILD ROOF $70" : "CHOP ROOF $70";
    it[3] = c->bump ? "RESTORE BUMPERS $15" : "STRIP BUMPERS $15"; it[4] = "BACK";
    r = ui_pick("PAINT AND BODY", it, 5, 0);
    if (r == 255 || r == 4) return;
    if (r == 0) { k = ui_pick("NEW PAINT JOB", paint_name, 12, c->paint); if (k != 255 && try_pay(20)) c->paint = k; }
    else if (r == 1) { k = ui_pick("STICKERS", stk_name, 9, c->stick); if (k != 255 && k != c->stick && (k == 0 || try_pay(5))) c->stick = k; }
    else if (r == 2) { if (try_pay(70)) c->roof ^= 1; }
    else if (try_pay(15)) c->bump ^= 1;
}
static u8 race_type, opp_ok;                 /* 0 arrancada, 1 estrada, 2 so por diversao, 3 pink slip */
static u16 bet;

static char pmb[2][28]; static u8 pm_on;     /* mensagem do resultado da ultima corrida (mostrada ao voltar a garagem); copiada, pois quem a escreve (banco 3) nao e quem a mostra (banco 2) */
static void pm_put(u8 i, const char *s) { u8 n = 0; while (*s && n < 27) pmb[i][n++] = *s++; pmb[i][n] = 0; if (!i) pm_on = 1; }
static const u16 bets[4] = {10, 25, 50, 100};
static const char *pil_get(u8 i) { return PILN(i); }
static void pick_opp_car(u8 pil) BK {           /* pilotos melhores (indice maior) correm com carros de classe mais alta */
    u8 target = (u8)((u16)pil * 8 / 21), c, tries;
    for (tries = 0; tries < 60; tries++) {
        i16 d;
        c = (u8)(rnd16() % 25); d = (i16)car_perf[c] - target;
        if (d >= -1 && d <= 1) break;
    }
    sel_opp = c;
}
static const char *const yes_msg[6] = {"COOL!", "O.K.!", "LET'S HIT IT!", "YOU'RE ON!", "ALRIGHT!", "LET'S GO!"};      /* 0-2 arrancada, 3-5 estrada */
static const char *const no_msg[10] = {"BLOW IT OUT YOUR EAR!", "FORGET IT, GEEK!", "BEAT IT, TWERP!", "NO WAY, YA NERD!", "GET LOST!", "GET LOST!", "GIVE ME A BREAK!", "GET OUTTA MY FACE!", "BUZZ OFF!", "WHADDA LOSER!"};
/* decisao do oponente de aceitar a aposta: porte de SRSE.EXE img 0x9BC5 (tabelas 9/10/11 do HOT_DATA). kind: 0 so por diversao, 1 $10, 2 $50, 3 $25, 4 $100, 5 pink slip */
static const u8 acc_t9[9] = {210, 180, 150, 175, 150, 125, 140, 120, 100};      /* "tempo" por classe do carro x motor: menor = mais forte */
static const u8 acc_t10[9] = {60, 45, 30, 55, 40, 20, 50, 30, 10};              /* piloto (4 / 7 / 11 por nivel) x tipo de aposta */
static const u8 acc_t11[12] = {40, 60, 90, 70, 65, 25, 90, 70, 15, 90, 35, 0};  /* tipo de aposta x (voce bem mais forte / parelho / ele mais fraco) */
static u8 perf_class(u8 id) { u8 p = car_perf[id]; return p < 3 ? 0 : p < 7 ? 1 : 2; }
static u8 opp_accepts(u8 kind, u8 p) {
    u8 col = kind == 0 ? 0 : kind == 5 ? 2 : 1, row = kind == 0 ? 0 : kind <= 2 ? 1 : kind <= 4 ? 2 : 3, rel, cool, oe = (car_flags[sel_opp] >> 8) & 7, pe = gc[cur].eng;
    i16 a = acc_t10[(p < 4 ? 0 : p < 11 ? 3 : 6) + col], b, c, ch;
#ifdef TEST_ACCEPT
    (void)kind; (void)p; return 1;
#endif
    if (oe > 2) oe = 2;
    if (pe > 2) pe = 2;
    b = acc_t9[perf_class(sel_opp) * 3 + oe]; c = acc_t9[perf_class(gc[cur].id) * 3 + pe];
    rel = a + b < c ? 0 : b > c ? 2 : 1;
    cool = gc[cur].stick == 0 ? 0 : (gc[cur].stick & 1) ? 4 : 6;                       /* adesivo: o original soma 4 (numero impar) ou 6 */
    ch = acc_t11[row * 3 + rel];
    ch += (i16)pil_net[p] * ch / 4 + cool;
    return (rnd16() % 100) <= ch;
}
static void gas_note(void) { ui_note("YOU'RE GONNA HAFTA", "GET MORE GAS"); }
static u8 bobs_race(u8 p) BK {                  /* piloto p e carro sel_opp ja escolhidos: tipo de corrida e aposta (regras do original). 1 = a corrida vai comecar; 0 = desistiu ou ele foi embora */
    static const char *const types[3] = {"DRAG RACE", "ROAD RACE", "NEVER MIND"};
    static const char *const bd[4] = {"JUST FOR KICKS", "BET $10", "BET $50", "NEVER MIND"};
    static const char *const br[4] = {"BET $25", "BET $100", "PINK SLIPS", "NEVER MIND"};
    static const u16 bv[6] = {0, 10, 50, 25, 100, 0};
    u8 t, b, kind, tries, last = 255, m;
    sel_pil = p; bet = 0;
    if (gc[cur].fuel < 1) { gas_note(); return 0; }
    if (p == NPILOTS - 1) {                         /* o Rei so corre com quem ja venceu bastante (original 0x9556) */
        u16 w = (u16)wins_drag + wins_road; u8 g;
        g = w < 3 ? 2 : (wins_drag < 6 && wins_road < 6) ? (w < 7 ? 2 : 3) : wins_drag < 6 ? 4 : wins_road < 6 ? 5 : (w < (races_tot >> 1) ? 3 : 0);
        if (g) {
            if (g == 2) ui_note("I DON'T GOT TIME FOR", "AMATEURS LIKE YOU!");
            else if (g == 3) ui_note("YOU'RE DOING OK KID,", "BUT NEED TO WIN MORE RACES!");
            else ui_note("YOU'RE GONNA HAFTA WIN", g == 4 ? "MORE DRAG RACES!" : "MORE ROAD RACES!");
            return 0;
        }
        if (gc[cur].fuel < 4) { gas_note(); return 0; }
        race_road = king_st; race_type = king_st ? 3 : 2;
        ui_note(king_st ? "COOL, DUDE! GET YOUR" : "YOU'RE ON! LET'S HIT", king_st ? "PINK SLIP OUT, LET'S GO!" : "THE DRAG RACE!");
        race_len = king_st ? ROAD_LEN : RACE_LEN;
        return 1;
    }
    t = ui_pick("WHAT KIND OF RACE?", types, 3, 0);
    if (t >= 2) { ui_note("CHICKEN!", PILN(p)); return 0; }
    if (t == 1 && gc[cur].fuel < 3) { gas_note(); return 0; }
    for (tries = 0; tries < 2; tries++) {
        b = ui_pick("HOW MUCH?", t ? br : bd, 4, 0);
        if (b >= 3) { ui_note("CHICKEN!", PILN(p)); return 0; }
        kind = t ? 3 + b : b;
        if (money < bv[kind]) { ui_note("YOU DON'T HAVE ENOUGH", "DOUGH FOR THAT BET!"); continue; }
        if (opp_accepts(kind, p)) {
            race_road = t; race_type = kind == 0 ? 2 : kind == 5 ? 3 : t; bet = bv[kind];
            ui_note(yes_msg[t * 3 + rnd16() % 3], PILN(p));
            race_len = t ? ROAD_LEN : RACE_LEN;
            return 1;
        }
        do m = (u8)(rnd16() % 5); while (m == last);                       /* nao repete a frase da recusa anterior */
        last = m;
        ui_note(no_msg[t * 5 + m], PILN(p));
    }
    return 0;
}
static u8 bobs(void) BK {                       /* fluxo de texto (testes automaticos): lista de pilotos -> bobs_race. 1 = a corrida vai comecar */
    u8 p = 0;
    for (;;) {
        ui_get = pil_get;
        p = ui_list("CHALLENGE WHO?", NPILOTS, p);
        if (p == 255) return 0;
        pick_opp_car(p);
        ui_note(PILN(p), CARN(sel_opp));            /* quem e e com que carro */
        if (bobs_race(p)) return 1;
    }
}
static char pbuf[30];
static void settle_race(void) BK {              /* desgaste, quebras, dinheiro, carros, vitorias e a mensagem do resultado */
    Car *c = &gc[cur];
    u8 win, ep, tpr, brk0 = 0, gone = 0, n;
    c->fuel = c->fuel > 3 ? c->fuel - 3 : 0;
    c->wear = c->wear + 100 + over_t * 30 > 10000 ? 10000 : c->wear + 100 + over_t * 30;
    c->tp = c->tp + 6 + (race_road ? 4 : 0) > 100 ? 100 : c->tp + 6 + (race_road ? 4 : 0);
    pm_on = 0;
    if (result != 3) {                       /* quebras: motor (rotacao e desgaste), cambio (trocas com o motor passado), pneus (tabela do jogo) */
        ep = over_t * 3 + (u8)(c->wear / 1000); if (ep > 60) ep = 60;                 /* so estoura se passou do corte (3% por tick) ou com o motor muito gasto */
        tpr = badshift * 8; if (tpr > 70) tpr = 70;
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
            pm_put(0, brk0 == 1 ? "BLEW YOUR ENGINE AGAIN?" : brk0 == 2 ? "YOU JUST DROPPED YOUR" : "BLEW YOUR TIRES AGAIN?");
            pm_put(1, brk0 == 2 ? "TRANSMISSION, KLUTZ!" : "YOU LOSE. TAKE A WALK.");
        } else if (result == 2 && dbg_top >= 90 && rnd16() % 100 < 6) {      /* perdeu a alta velocidade: pode destruir o carro */
            u16 scrap = scrap_value(c);
            n = 0; put_str(pbuf, &n, "CAR TOTALED. $"); put_num(pbuf, &n, scrap); put_str(pbuf, &n, " SCRAP"); pbuf[n] = 0;
            money += scrap; remove_car(cur); gone = 1;
            pm_put(0, "LUCKY YOU WEREN'T KILLED!"); pm_put(1, pbuf);
        }
    }
    win = result == 1;
    if (!pm_on) {
        pm_put(0, win ? "YOU WON!" : result == 3 ? "YA JUMPED THE LIGHT," : "YOU LOSE, SUCKER.");
        pm_put(1, win ? "TOO BAD YOU DIDN'T BET" : result == 3 ? "SPEEDY!" : "TAKE A WALK.");
    }
    if (result != 3 && !street_mode && sel_pil != NPILOTS - 1 && race_type != 2) {       /* corridas valendo: contam vitorias, total e a rivalidade com o piloto */
        if (races_tot < 255) races_tot++;
        if (win) { if (race_road) { if (wins_road < 255) wins_road++; } else if (wins_drag < 255) wins_drag++; }
        if (pil_net[sel_pil] > -100 && win) pil_net[sel_pil]--;
        else if (pil_net[sel_pil] < 100 && !win) pil_net[sel_pil] += 2;
    }
    if (result != 3 && sel_pil == NPILOTS - 1 && !street_mode) {                          /* o Rei: arrancada por diversao, depois estrada valendo o carro */
        if (!win) { king_st = 0; wins_drag = wins_drag > 3 ? wins_drag - 3 : 0; wins_road = wins_road > 2 ? wins_road - 2 : 0; }
        else if (king_st) game_end = 2;                                                 /* venceu o Rei */
        else king_st = 1;
    }
    if (race_type == 3) {
        if (win) {
            pm_put(0, "YOU JUST WON A"); pm_put(1, "HELLUVA CAR!");
            if (nown < MAXCARS) { car_stock(&gc[nown], sel_opp); nown++; }
            else { money += car_price[sel_opp] / 2; pm_put(1, "HELLUVA CAR! (SOLD IT)"); }
        } else {
            if (!gone) remove_car(cur);
            pm_put(0, "YOU LOST YOUR CAR!"); pm_put(1, "PINK SLIP GONE");
        }
    } else if (bet) {
        if (win) {
            money += bet;
            pm_put(0, bet == 10 ? "BIG DEAL." : bet == 25 ? "TWENTY-FIVE BUCKS." : bet == 50 ? "FIFTY SMACKERS." : "A HUNDRED SMACKERS,");
            pm_put(1, bet == 10 ? "TEN LOUSY BUCKS." : bet == 25 ? "NOT BAD." : bet == 50 ? "NOT BAD." : "HOT SHOT.");
        } else money = money > bet ? money - bet : 0;
    }
    if (day < 62) day++;
    if (day >= END_DAY && !game_end) game_end = 1;
    dbg_money = money; dbg_day = day;
}
static void wait_click(void) {
    u8 held = 1, go;
    for (;;) { read_input(); combine_input(); go = (g_trig & 3) != 0; if ((go && !held) || (want_back() && !held)) return; held = go || want_back(); }
}
static void full_screen(u8 grp, u8 wait) {      /* tela cheia da LIB1 (paleta EGA) ate um clique */
    u8 rg1 = *(volatile u8 *)0xF3E0;
    clip_l = 0; clip_r = 256; ybase = 0;
    vdp_reg(1, rg1 & ~0x40); g_palp = (u16)(grp == 252 ? pal_rank : grp == 254 ? pal_cred : pal_ega); pal_load2(); g_group = grp; load_assets(); vdp_reg(2, 0x1F); vdp_reg(1, rg1 | 0x40);
    if (wait) wait_click();
}
static char rk_name[10][13]; static u16 rk_pts[10];       /* ranking (guardado em RAM; ver rk_init) */
static u8 rk_add(void) BK;
static void rk_init(void) BK;
static void put_rank(u8 pos, const char *name, u16 score, u8 you) {   /* uma linha da tabela, na fonte do jogo: nome a esquerda, dinheiro a direita */
    char b[8]; u8 n = 1, y = 62 + 11 * pos;
    b[0] = '$'; n += num_str(b + 1, score); b[n] = 0;
    cmd_n = 0; cp = cmd_buf;
    if (you) { push_fill(32, y - 1, 134, 10, 15); vdp_wait(); }
    gtext(36, y, name, !you); gtext(162 - 6 * n, y, b, !you);
}
static void victory_and_ranking(void) {
    u8 i, pos;
    if (game_end == 2) full_screen(253, 1);                        /* "Well, you've done it, you've beaten the king..." */
    pos = rk_add();
    full_screen(252, 0);
    for (i = 0; i < 10; i++) put_rank(i, rk_name[i], rk_pts[i], i == pos);
    wait_click();
}
static void end_screen(void) BK {                 /* fim de jogo: mensagem e estatisticas, depois volta ao titulo */
    static const char *const l1[4] = {"", "YOU'RE DONE FER", "YOU DESERVE TO BE", "YOU'RE OUTTA DOUGH."};
    static const char *const l2[4] = {"", "THE SUMMER!", "KING OF THE COUNTY!", "LOOKS LIKE YOU'RE DONE"};
    u8 n;
    victory_and_ranking();
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
static void cr_mid(i16 y, const char *s) { u8 n = 0; while (s[n]) n++; gtext(128 - 3 * n, y, s, 0); }
static void credits(void) BK {                     /* LIB1 item 3: a placa "AD 1989"; o texto miudo original (320 -> 256 colunas) ficou ilegivel, entao e refeito na fonte do jogo */
    full_screen(254, 0);
    cmd_n = 0; cp = cmd_buf;
    push_fill(14, 56, 53, 43, 5); push_fill(183, 56, 65, 43, 5); push_fill(35, 101, 186, 7, 5); push_fill(60, 160, 136, 8, 5); push_fill(54, 170, 154, 17, 5);     /* apaga os textos pequenos (indice 5 = fundo da placa) */
    cr_mid(172, "PORT BY BIGFIVE STUDIOS"); cr_mid(180, "CLICK FOR CREDITS");
    wait_click();
    push_fill(12, 30, 234, 157, 5);                                                    /* pagina 2: so o texto, na moldura da placa */
    gtext(18, 36, "PROGRAM BY:", 0); gtext(18, 47, "ANDRZEJ SANOJCA", 0); gtext(18, 56, "BOGDAN BLASZCZAK", 0); gtext(18, 65, "TOMASZ PAWLOWSKI", 0);
    gtext(18, 74, "WOJTEK WAS", 0); gtext(18, 83, "JAREK OLSZEWSKI", 0); gtext(18, 92, "PRZEMEK ROKITA", 0);
    gtext(130, 36, "COMPUTOGRAPHY BY:", 0); gtext(130, 47, "MARCIN SZOSTAKOWSKI", 0); gtext(130, 56, "MACIEJ MARKUSZEWSKI", 0); gtext(130, 65, "HYRU GAU", 0);
    gtext(130, 80, "SOUNDS BY:", 0); gtext(130, 91, "DOROTA BLASZCZAK", 0);
    cr_mid(108, "DEVELOPMENT AND PROGRAMMING BY"); cr_mid(117, "P.Z. KAREN CO. DEVELOPMENT GROUP");
    cr_mid(131, "ORIGINAL CONCEPT DEVELOPED BY"); cr_mid(140, "MAGIC PARTNERS");
    cr_mid(154, "(C) 1989 LOGICAL DESIGN WORKS, INC."); cr_mid(163, "ALL RIGHTS RESERVED");
    cr_mid(175, "MSX2 PORT BY BIGFIVE STUDIOS");
    vdp_wait(); wait_click();
}
#pragma codeseg BANK2
static void run_race(void) BK;
static void race_go(void) BK;
static u8 street_trip(void) {                /* 1 = chegou ao Bobs; 0 = desistiu (ESC) */
    Car *c = &gc[cur]; u8 gas = 1;
    u16 avg;
#ifdef SKIP_STREET
    return 1;
#endif
    ui_note("CRUISING IN TOWN...", "CLICK TO SKIP"); ui_nohint = 0;
    street_mode = 1; race_type = 1; race_road = 1; street_res = 1;
    race_go();
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
/* ---- garagem clicavel: cursor (sprite 16x16) movido por setas/joystick; ESPACO/botao A clica no objeto sob a ponta ---- */
/* ---- o carro do jogador desenhado na garagem (corpo + rodas, LIB2; ver cars_art.py) ---- */
#define BAND_Y 80                               /* faixa do chao que o carro ocupa (nas paginas 0-2); a copia limpa fica na pagina 3 */
#define BAND_H 84
#define CAR_FLOOR 152                          /* y da base das rodas */
static u8 car_on, car_x0, car_x1, car_y0, car_y1;
static void band_cmd(i16 sy, i16 dy) {
    vc_w(0, 0); vc_w(2, sy); vc_w(4, 0); vc_w(6, dy); vc_w(8, 256); vc_w(10, BAND_H); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}
static void band_save(void) { band_cmd(512 + BAND_Y, BAND_COPY_Y); }
static const u8 pcol_rb[12] = {0x55, 0x60, 0x16, 0x01, 0x70, 0x66, 0x70, 0x05, 0x11, 0x77, 0x75, 0x42};    /* cores da pintura (R<<4|B) e G: as 8 primeiras = as dos oponentes */
static const u8 pcol_g[12]  = {0, 0, 2, 4, 6, 6, 3, 5, 1, 7, 3, 2};
static void set_paint(u8 p) {                   /* indice 6 da paleta = pintura, 7 = reflexo (mais claro) */
    u8 rb = pcol_rb[p], g = pcol_g[p], r = (rb >> 4) & 7, b = rb & 7;
    g_reg = 6; g_val = rb; g_sh = g; pal_one();
    r = r > 5 ? 7 : r + 2; b = b > 5 ? 7 : b + 2; g = g > 5 ? 7 : g + 2;
    g_reg = 7; g_val = (u8)(r << 4 | b); g_sh = g; pal_one();
}
#define CH(id) car_h[cvar * NCARS + (id)]
#define CWY(id) car_wy[cvar * NCARS + (id)]
static void car_art(u8 id, u8 tir) {                    /* descomprime o corpo (variante cvar), a roda e o adesivo no rascunho da pagina 3 */
    if (tir > 2) tir = 2;
    g_group = cvar ? 100 + id * 3 + cvar - 1 : 2 + id; load_assets();
    g_group = 28 + tir; load_assets();
    if (cstk) { g_group = 179 + cstk; load_assets(); }
}
static void clmmm(i16 sx, i16 sy, i16 dx, i16 dy, i16 w, i16 h) {      /* LMMM recortado nas bordas da tela */
    if (dx < 0) { sx -= dx; w += dx; dx = 0; }
    if (dx + w > 256) w = 256 - dx;
    if (w > 0) lmmm(sx, sy, dx, dy, w, h);
}
static i16 car_floor = CAR_FLOOR;                        /* y da base das rodas (garagem 152; Bob's mais baixo) */
static u8 chead = 255;                                   /* piloto cuja cabeca aparece na janela (255 = nenhuma) */
static i16 car_yt(u8 id) { return car_floor - (CWY(id) + (WH_H >> 1)); }
static i16 head_x(u8 id, i16 xl) { return xl + ((car_wx0[id] + car_wx1[id]) >> 1) - 16 - (head_w[chead] >> 1); }   /* canto da cabeca: atras do meio do carro (lugar do motorista) */
static i16 head_y(u8 id) { return car_yt(id) + (CH(id) * 28) / 100 - (head_h[chead] >> 1) - 4; }
static void cshadow(i16 xa, i16 y, i16 xb, i16 h) {     /* retangulo preto (LMMV) */
    if (xa < 0) xa = 0;
    if (xb > 256) xb = 256;
    if (xb <= xa) return;                                   /* fora da tela (carro entrando/saindo) */
    vc_w(4, xa); vc_w(6, y); vc_w(8, xb - xa); vc_w(10, h); vc[12] = HC_K; vc[13] = 0; vc[14] = 0x80;
    vdp_cmd15();
}
static u8 fr_n; static i16 fr_x[2] = {0, 24}, fr_w[2] = {12, 30};      /* objetos do cenario NA FRENTE do carro (bombas do posto): retangulos restaurados da copia limpa (pagina 2) por cima dele */
#define FR_Y 92
#define FR_H 82
static void car_blit(i16 base, u8 id, i16 xl) {         /* base = 0/256/512 (pagina); rodas por baixo, corpo por cima */
    i16 yt = base + car_yt(id), wy = yt + CWY(id) - (WH_H >> 1), fl = base + car_floor, x1 = xl + car_w[id];
    u8 i;
    cshadow(xl + 6, fl - 5, x1 - 6, 2); cshadow(xl + 2, fl - 3, x1 - 2, 3); cshadow(xl + 14, fl, x1 - 14, 2);     /* sombra elíptica sob o carro */
    clmmm(0, WHEEL_Y0, xl + car_wx0[id] - (WH_W >> 1), wy, WH_W, WH_H);
    clmmm(0, WHEEL_Y0, xl + car_wx1[id] - (WH_W >> 1), wy, WH_W, WH_H);
    if (chead != 255) clmmm(0, STK_Y0, head_x(id, xl), head_y(id) - car_yt(id) + yt, head_w[chead], head_h[chead]);      /* o retrato do piloto aparece pela janela (o corpo vem por cima) */
    clmmm(0, CAR_Y0, xl, yt, car_w[id], CH(id));
    if (cstk) clmmm(0, STK_Y0, xl + ((car_wx0[id] + car_wx1[id] - stk_w[cstk - 1]) >> 1) + 4, yt + (CH(id) * 3) / 5 - (stk_h[cstk - 1] >> 1), stk_w[cstk - 1], stk_h[cstk - 1]);
    for (i = 0; i < fr_n; i++) {                                         /* o carro passa ENTRE a bomba e a construcao: a bomba volta por cima */
        vc_w(0, fr_x[i]); vc_w(2, 512 + FR_Y); vc_w(4, fr_x[i]); vc_w(6, base + FR_Y); vc_w(8, fr_w[i]); vc_w(10, FR_H); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
        vdp_cmd15();
    }
}
static u8 slide_pg;                                    /* pagina que ficou visivel no fim da animacao */
/* animacao otimizada: o carro pronto (sombra, rodas, corpo, retrato, adesivo) e montado uma vez em x=0 na faixa da pagina 2 (rascunho durante a animacao) e as
   corridas opacas de cada linha sao medidas; a faixa e dividida em ate 3 retangulos opacos (copiados por HMMM, ~4x mais rapido que LMMM) e o resto (bordas,
   janelas, rodas) vai por LMMM. A cada quadro: restaura o fundo (HMMM da copia limpa da pagina 3) e desenha esses poucos blocos. Posicoes pares (HMMM so
   enderecia bytes); quadros com x impar usam o desenho completo. */
static i16 rs_y0; static u8 rs_n;
static u8 sg_n, sg_r[12], sg_h[12], sg_c1[12], sg_c2[12], sg_f[12];      /* faixas de linhas: f=1 retangulo opaco (miolo c1..c2 por HMMM, bordas por LMMM); f=0 plana (so LMMM de c1..c2) */
static void hm(i16 sx, i16 sy, i16 dx, i16 dy, i16 w, i16 h) {
    vc_w(0, sx); vc_w(2, sy); vc_w(4, dx); vc_w(6, dy); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}
static void seg_add(u8 r, u8 h, u8 f, u8 c1, u8 c2) { if (!h || sg_n >= 12) return; sg_r[sg_n] = r; sg_h[sg_n] = h; sg_f[sg_n] = f; sg_c1[sg_n] = c1; sg_c2[sg_n] = c2; sg_n++; }
static void slide_prep(u8 id) {
    u8 q[10][4], k, r, h, body, wz, w = car_w[id];
    g_bk_bank = RECT_BANK; g_bk_src = 0x8000 + RECT_BASE + 40 * ((u16)cvar * (u8)sizeof(car_w) + id); g_bk_dst = (u16)q; g_bk_len = 40; bank_copy();       /* faixas do corpo (gen_assets.py: best_rects) */
    rs_y0 = car_yt(id) - 4; if (rs_y0 < BAND_Y) rs_y0 = BAND_Y;
    k = car_floor + 2 - rs_y0; rs_n = k > 84 ? 84 : k;
    body = car_yt(id) - rs_y0;
    wz = body + CWY(id) - (WH_H >> 1);                          /* a partir daqui ha rodas e sombra: faixas planas valem a largura toda */
    vc_w(4, 0); vc_w(6, 512 + BAND_Y); vc_w(8, 256); vc_w(10, BAND_H); vc[12] = 0; vc[13] = 0; vc[14] = 0xC0;      /* HMMV: zera a faixa (cor 0 = transparente) */
    vdp_cmd15();
    car_blit(512, id, 0);
    sg_n = 0;
    for (k = 0; k < 10 && q[k][1]; k++) {
        r = body + q[k][0]; h = q[k][1] & 0x7F;
        if (r + h > rs_n) break;
        if (q[k][1] & 0x80) seg_add(r, h, 1, q[k][2], q[k][3]);
        else if (r + h > wz) seg_add(r, h, 0, 0, w);
        else seg_add(r, h, 0, q[k][2], q[k][3]);
    }
    r = sg_n ? sg_r[sg_n - 1] + sg_h[sg_n - 1] : body;
    seg_add(r, rs_n - r, 0, 0, w);                              /* de baixo do corpo ao chao: rodas e sombra */
    vdp_wait();
}
static void slide_frame(u8 id, u8 pg, i16 x, i16 px) {
    i16 base = (i16)pg * 256, xa = x < px ? x : px, xb = (x > px ? x : px) + car_w[id], y, ix0, ix1, c1, c2, l, r, sy;
    u8 k;
    if (xa < 0) xa = 0;
    xb = (xb + 1) & ~1; if (xb > 256) xb = 256;
    xa &= ~1;
    for (k = 0; k < sg_n; k++) {
        y = rs_y0 + sg_r[k]; c1 = sg_c1[k]; c2 = sg_c2[k]; sy = BAND_COPY_Y + (y - BAND_Y);
        if (sg_f[k]) {                                          /* retangulo opaco: o miolo cobre o fundo, so as margens precisam ser restauradas */
            l = x + c1; if (l < xa) l = xa; if (l > xb) l = xb;
            r = x + c2; if (r < xa) r = xa; if (r > xb) r = xb;
            if (l > xa) hm(xa, sy, xa, base + y, l - xa, sg_h[k]);
            if (xb > r) hm(r, sy, r, base + y, xb - r, sg_h[k]);
            clmmm(0, 512 + y, x, base + y, c1, sg_h[k]);
            ix0 = x + c1; ix1 = x + c2;
            if (ix0 < 0) ix0 = 0;
            if (ix1 > 256) ix1 = 256;
            if (ix1 > ix0) hm(ix0 - x, 512 + y, ix0, base + y, ix1 - ix0, sg_h[k]);
            clmmm(c2, 512 + y, x + c2, base + y, car_w[id] - c2, sg_h[k]);
        } else {
            if (xb > xa) hm(xa, sy, xa, base + y, xb - xa, sg_h[k]);
            clmmm(c1, 512 + y, x + c1, base + y, c2 - c1, sg_h[k]);
        }
    }
    for (k = 0; k < fr_n; k++) hm(fr_x[k], BAND_COPY_Y + (FR_Y - BAND_Y), fr_x[k], base + FR_Y, fr_w[k], BAND_Y + BAND_H - FR_Y);      /* posto: as bombas voltam por cima do carro (da copia limpa da pagina 3; abaixo da faixa o carro nao chega) */
}
static void car_slide(u8 id, i16 x0, i16 x1) {          /* anda o carro de x0 a x1 (12 px por quadro), desenhando na pagina oculta e trocando a pagina */
    i16 x = x0, d = x1 > x0 ? 1 : -1, px[2]; u8 pg = 1, fast = 1; u16 j;
    if (fast) { slide_prep(id); px[0] = px[1] = x0; }
    for (;;) {
        if (fast && !(x & 1)) slide_frame(id, pg, x, px[pg]);
        else { band_cmd(BAND_COPY_Y, pg * 256 + BAND_Y); car_blit(pg * 256, id, x); }
        px[pg] = x;
        vdp_wait();
        j = JIFFY; while (JIFFY == j) ;
        vdp_reg(2, (u8)((pg << 5) | 0x1F));
        pg ^= 1;
        if (x == x1) break;
        x += d * 12;
        if ((d > 0 && x > x1) || (d < 0 && x < x1)) x = x1;
        else x &= ~1;                                  /* posicoes pares: o miolo vai por HMMM */
    }
    slide_pg = pg ^ 1;
    if (fast) band_cmd(BAND_COPY_Y, 512 + BAND_Y);     /* devolve a faixa da pagina 2 (era rascunho) */
}
static void car_page(u8 p, u8 id, i16 xl) { band_cmd(BAND_COPY_Y, p * 256 + BAND_Y); car_blit(p * 256, id, xl); }
static void draw_car(void) {                    /* mostra o carro atual (com a animacao de entrada/saida) e deixa o resultado final nas 3 paginas (0 visivel, 1, 2 = copia para restaurar menus) */
    u8 p, sp, id = nown ? gc[cur].id : 255;
    i16 xl;
    if (shown_id != 255 && shown_id != id) {                       /* o carro antigo sai de re (para a esquerda) */
        cvar = shown_var; cstk = shown_stk; car_art(shown_id, shown_tir); set_paint(shown_paint);
        car_slide(shown_id, (256 - car_w[shown_id]) >> 1, -(i16)car_w[shown_id]);
    }
    car_on = 0;
    if (id == 255) { for (p = 0; p < 3; p++) band_cmd(BAND_COPY_Y, p * 256 + BAND_Y); vdp_wait(); shown_id = 255; vdp_reg(2, 0x1F); return; }
    cvar = gc[cur].roof | (gc[cur].bump << 1); cstk = gc[cur].stick;
    car_art(id, gc[cur].tir); set_paint(gc[cur].paint);
    xl = (256 - car_w[id]) >> 1;
    sp = 0;                                                       /* pagina visivel agora */
    if (shown_id == 255) { car_slide(id, -(i16)car_w[id], xl); sp = slide_pg; }      /* garagem vazia: o carro entra */
    else if (shown_id != id) sp = slide_pg;                       /* o antigo acabou de sair: a pagina visivel esta vazia */
    car_page(2, id, xl);                                           /* a pagina visivel nunca e apagada (sem piscar): desenha nas outras e troca */
    if (sp == 0) { car_page(1, id, xl); vdp_wait(); vdp_reg(2, 0x3F); car_page(0, id, xl); }
    else { car_page(0, id, xl); vdp_wait(); vdp_reg(2, 0x1F); car_page(1, id, xl); }
    vdp_wait(); vdp_reg(2, 0x1F);
    shown_id = id; shown_tir = gc[cur].tir; shown_paint = gc[cur].paint; shown_var = cvar; shown_stk = cstk;
    car_on = 1; car_x0 = xl; car_x1 = xl + car_w[id]; car_y0 = car_yt(id); car_y1 = car_y0 + CH(id);
}
#pragma codeseg CODE
typedef struct { u8 x0, y0, x1, y1, id; } Hot;
static const char *const hot_name[9] = {"YOUR CARS", "NEWSPAPER", "CALENDAR", "CATCH SOME TUNES", "WANNA TANK?", "HIT THE STREET", "TIME TO QUIT", "PAINT AND BODY", "OPEN THE HOOD"};
#define NHOT 9
static const Hot hot[NHOT] = {
    {78, 68, 95, 88, 0}, {108, 156, 136, 187, 0}, {10, 156, 40, 180, 1}, {236, 40, 254, 90, 2}, {204, 48, 232, 64, 3},
    {226, 160, 252, 188, 4}, {0, 66, 30, 134, 5}, {124, 50, 150, 74, 6}, {178, 158, 208, 188, 7}};
static const Hot *hs; static u8 hn, lab_mode;            /* objetos clicaveis da tela atual; lab_mode 0 = garagem (visor), 1 = jornal */
static const char *const *hnames;
static u8 hot_at(u8 x, u8 y) {
    u8 i;
    if (!lab_mode && car_on && x >= car_x0 && x < car_x1 && y >= car_y0 && y < car_y1) {
        if (x >= car_x1 - (car_x1 - car_x0) / 3 && y >= car_y0 + (car_y1 - car_y0) / 3 && y < car_y0 + (car_y1 - car_y0) * 3 / 5) return 8;       /* o capo (frente do carro): o motor */
        return 0;                                                                                                                                  /* o resto do carro desenhado: YOUR CARS */
    }
    for (i = 0; i < hn; i++) if (x >= hs[i].x0 && x <= hs[i].x1 && y >= hs[i].y0 && y <= hs[i].y1) return hs[i].id;
    return 255;
}
static void hot_label(u8 h) {
    u8 n = 0;
    cmd_n = 0; cp = cmd_buf;
    if (lab_mode == 1) { vdp_wait(); cmd_n = 0; cp = cmd_buf; return; }                       /* jornal: as faixas ja dizem o que sao */
    if (lab_mode == 2) {                                                                         /* Bob's: o nome do piloto sob o cursor, na linha de baixo da barra de status */
        push_fill(0, 201, 206, 10, HC_W);
        if (h == 0) { gtext(6, 202, hnames[0], 0); gtext(6 + 6 * 12, 202, "- CLICK TO CHALLENGE", 0); }
        vdp_wait(); cmd_n = 0; cp = cmd_buf; return;
    }
    if (lab_mode == 3) {                                                                         /* janelas do motor: a barra de status diz o que e */
        push_fill(0, 201, 206, 10, HC_W);
        if (h != 255) gtext(6, 202, hnames[h], 0);
        vdp_wait(); cmd_n = 0; cp = cmd_buf; return;
    }
    hub_restore(56, 33, 162, 15);        /* o visor branco no alto da garagem (ou a faixa no pe do jornal) mostra o nome do objeto sob o cursor */
    if (h != 255) {
        while (hnames[h][n]) n++;
        gtext(137 - 3 * n, 37, hnames[h], 0);
    }
    vdp_wait(); cmd_n = 0; cp = cmd_buf;
}
#ifdef OLD_MENU                                /* testes automaticos: o menu de texto antigo (ver README) */
static u8 scene_pick(void) {
    static const char *const items[4] = {"YOUR CARS", "NEWSPAPER", "BOBS DRIVE-IN", "QUIT GAME"};
    static const u8 map[4] = {0, 1, 5, 6};
    static u8 sel = 2;
    u8 r;
    dbg_state = 7;
    for (;;) { ui_nohint = 1; r = ui_pick("MAIN MENU", items, 4, sel); ui_nohint = 0; if (r != 255) { sel = r; return map[r]; } }
}
#else
static u8 pick_abort;                              /* uma funcao de pick_tick pode pedir a saida de pick_hot (devolve 253) */
static void (*pick_tick)(void);                  /* chamada a cada quadro por pick_hot (animacoes) */
static u8 pick_hot(u8 allow_back) {            /* devolve o id do objeto clicado (254 = voltar, se allow_back) */
    u8 held = 1, bheld = 1, prev = 0xFE, hit, hold = 0, st, step, go;
    u16 j;
    dbg_state = 7; dbg_money = money; dbg_day = day; dbg_nown = nown; dbg_nspare = nspare;
    spr_on();
    for (;;) {
        j = JIFFY; while (JIFFY == j) ;
        if (pick_tick) pick_tick();
        if (pick_abort) { pick_abort = 0; spr_off(); return 253; }
        read_input(); combine_input();
        st = g_stick; go = (g_trig & 3) != 0;
        if (st) {
            step = 1 + (hold >> 3); if (step > 6) step = 6; if (hold < 255) hold++;
            if (st >= 2 && st <= 4 && cur_x + step < 255) cur_x += step;
            if (st >= 6 && st <= 8 && cur_x > step) cur_x -= step;
            if ((st == 8 || st == 1 || st == 2) && cur_y > step) cur_y -= step;
            if (st >= 4 && st <= 6 && cur_y + step < cur_ymax + 1) cur_y += step;
        } else hold = 0;
        if (!st) mouse_move();
        hit = hot_at(cur_x, cur_y);
        if (hit != prev) { prev = hit; spr_off(); hot_label(hit); spr_on(); }
        g_sx = cur_x - 3; g_sy = cur_y - 1; spr_put();
        if (allow_back && want_back() && !bheld) { spr_off(); return 254; }
        bheld = allow_back && want_back();
        if (go && !held && hit != 255) { spr_off(); return hit; }
        held = go;
    }
}
static u8 scene_pick(void) { hs = hot; hn = NHOT; hnames = hot_name; lab_mode = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187; return pick_hot(0); }
#endif
#pragma codeseg BANK2
#ifdef OLD_MENU
#define engine_bay() ((void)0)                 /* (bankify le as definicoes na ordem do arquivo: nao repetir o nome de uma funcao em #ifdef) */
#else
#pragma codeseg BANK4
/* ---- o motor (clique no capo): janela com o motor, o coletor e o carburador do carro; TUNE (ponto), PARTS (trocar pecas, com a arte de cada peca) e DONE.
   A arte (LIB2: motores 76-78, coletores 79-83, carburadores 84-86, cambios 99-102, pneus 65-68) vem em folhas carregadas na pagina 1 (grupos 66-68) e e copiada por LMMM ---- */
static u8 art_grp_cur;
static void art_use(u8 idx) {                          /* garante a folha da peca idx (0-18 = grupo 66, 19-22 = 67, 23-26 = 68) na pagina 1 */
    u8 g = idx < 19 ? 66 : idx < 23 ? 67 : 68;
    if (g == art_grp_cur) return;
    g_group = g; load_assets();                          /* a folha vai para a pagina 1 (fora da tela): sem apagar a tela */
    art_grp_cur = g;
}
static void art_put(u8 idx, i16 x, i16 y) {            /* a peca vai da folha (pagina 1) para a tela (pagina 0); cor 0 e transparente */
    art_use(idx);
    vc_w(0, art_x[idx]); vc_w(2, 256 + art_y[idx]); vc_w(4, x); vc_w(6, y); vc_w(8, art_w[idx]); vc_w(10, art_h[idx]); vc[12] = 0; vc[13] = 0; vc[14] = 0x98;
    vdp_cmd15();
}
#define BW_X 36
#define BW_Y 24
#define BW_W 184
#define BW_H 126
#define BW_BTN (BW_Y + BW_H - 24)                      /* y dos botoes de baixo */
static void win_frame(u8 x, u8 w) {                    /* a moldura da janela: cinza e branco por fora, preto por dentro */
    cmd_n = 0; cp = cmd_buf; hub_restore(x, BW_Y, w, BW_H);
    push_fill(x, BW_Y, w, BW_H, 3); push_fill(x + 2, BW_Y + 2, w - 4, BW_H - 4, HC_W); push_fill(x + 4, BW_Y + 4, w - 8, BW_H - 8, HC_K);
}
static Hot bay_hot[13];
static void red_btn(u8 i, u8 x, u8 y, u8 w, const char *s) {     /* botao vermelho de borda branca, como o original (e ja registra o clique) */
    u8 n = 0;
    while (s[n]) n++;
    push_fill(x, y, w, 15, HC_W); push_fill(x + 1, y + 1, w - 2, 13, 10);
    gtext(x + (w - 6 * n) / 2, y + 4, s, 1);
    bay_hot[i].x0 = x; bay_hot[i].y0 = y; bay_hot[i].x1 = x + w - 1; bay_hot[i].y1 = y + 14; bay_hot[i].id = i;
}
/* parafusos (manual: "pressione duas vezes para soltar o parafuso; com todos soltos a peca some; clicar no furo vazio devolve o parafuso; duas vezes para apertar").
   Estado de cada parafuso: 0 apertado, 1 meio solto (soltando), 2 meio apertado (apertando), 3 parafuso erguido sobre o furo, 4 furo vazio.
   Soltar: 0 -> 1 -> 4. Apertar: 4 -> 3 -> 2 -> 0. Com todos os parafusos em 4 a peca sai e vai para as reservas; a peca nova (PARTS > SELECT) chega com os parafusos em 3.
   Ordem: carburadores -> coletor -> motor (o motor tambem tem a fiacao, 1 clique: 0 ligada, 4 solta). Encaixes: 0 motor, 1 coletor, 2 carburadores. */
static u8 bay_pr[3], bay_s[3][6], bay_nd, bay_dsl[10], bay_dix[10];
static i16 bay_dx[10], bay_dy[10];
static const u8 bay_cat[3] = {0, 3, 2};
static const char *const bay_nm[13] = {"TUNE THE ENGINE", "CHANGE THE PARTS", "CLOSE THE HOOD", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT", "THE BOLT"};
static const char *bay_msg;
static u8 bay_nsl(u8 sl) { u8 mm = gcar->man > 6 ? 6 : gcar->man; return sl == 2 ? 2 * mp_n[mm] : 2; }
static void bay_reset(void) { u8 i, j; for (i = 0; i < 3; i++) { bay_pr[i] = 1; for (j = 0; j < 6; j++) bay_s[i][j] = 0; } }
static u8 bay_tight(u8 sl) { u8 j, n = bay_nsl(sl); if (!bay_pr[sl]) return 0; for (j = 0; j < n; j++) if (bay_s[sl][j]) return 0; return 1; }
static u8 bay_ok(void) { return bay_tight(0) && bay_tight(1) && bay_tight(2); }
#ifdef BOLT_TEST
static u8 tscr_i;                                       /* teste: roteiro de cliques (0xA0|encaixe = soltar o proximo parafuso do encaixe; 0xB0|encaixe = apertar; 254 = nada) */
static u8 bay_test_k(u8 n) {
    static const u8 tscr[] = {255,255,255,0xA2,0xA2,0xA2,0xA2, 0xA1,0xA1,0xA1,0xA1, 0xA0,0xA0,0xA0, 1,0,3,2,6,
                              0xB0,0xB0,0xB0, 0xB1,0xB1,0xB1,0xB1, 0xB2,0xB2,0xB2,0xB2, 2};
    u8 k = tscr[tscr_i < sizeof tscr ? tscr_i : sizeof tscr - 1], d, sl, st; u16 j; u8 w;
    for (w = 0; w < 30; w++) { j = JIFFY; while (JIFFY == j) ; }
    dbg_t2 = bay_pr[0] | bay_pr[1] << 1 | bay_pr[2] << 2 | (u16)bay_nd << 8; dbg_t3 = bay_s[0][0] | bay_s[0][1] << 3 | bay_s[1][0] << 6 | bay_s[1][1] << 9 | bay_s[2][0] << 12;
    dbg_t1 = ++tscr_i; for (w = 0; w < 30; w++) { j = JIFFY; while (JIFFY == j) ; }
    if (k >= 0xA0 && k != 255) {
        sl = k & 3;
        for (d = 0; d < bay_nd; d++) if (bay_dsl[d] == sl) { st = bay_s[sl][bay_dix[d]]; if (k < 0xB0 ? st != 4 : st != 0) return 3 + d; }
        return 254;
    }
    return k;
}
#endif
static u8 bay_pick(u8 n, const char *const *names) {  /* espera um clique nos botoes (ESC = 254) */
    u8 k;
    vdp_wait(); cmd_n = 0; cp = cmd_buf;
#ifdef BOLT_TEST
    return bay_test_k(n);
#endif
    hs = bay_hot; hn = n; hnames = names; lab_mode = 3; pick_tick = 0; cur_ymax = 210;
    k = pick_hot(1);
    lab_mode = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187;
    return k;
}
#define ART_BOLT 13                                     /* indices da folha: 13 = item 87 (parafuso alto = solto), 14 = 88 (meio), 15 = 89 (so a cabeca = apertado), 16 = furo (138), 17 fiacao solta (94), 18 fiacao ligada (95) */
static void bay_dot(i16 x, i16 y, u8 st, u8 wire) {      /* (x, y) = canto da arte do parafuso (7x11, a base fica na linha 10) */
    if (wire) { art_put(st == 0 ? 18 : 17, x, y); return; }
    if (st >= 3) art_put(ART_BOLT + 3, x, y + 4);                                      /* o furo */
    if (st == 3) art_put(ART_BOLT, x, y);                                              /* parafuso solto sobre o furo: alto (original) */
    else if (st == 1 || st == 2) art_put(ART_BOLT + 1, x, y);
    else if (st == 0) art_put(ART_BOLT + 2, x, y);                                     /* apertado: so a cabeca vermelha (original) */
}
static void bay_add(u8 sl, u8 ix, i16 x, i16 y) { u8 n = bay_nd++; bay_dsl[n] = sl; bay_dix[n] = ix; bay_dx[n] = x; bay_dy[n] = y; }
/* motor, coletor e carburadores nas posicoes medidas no jogo original (README): o coletor comeca na borda do topo do motor com a base na borda frontal;
   cada carburador fica sobre uma almofada do coletor (base 9 linhas abaixo do topo do coletor), com 2 parafusos; o coletor tem 1 parafuso em cada ponta; o motor, 1 no "rosto" de baixo mais a fiacao */
#define AX(d) ((d) - ((d) + 1) / 5)                     /* coluna nativa (320 px, relativa ao motor) -> coluna da arte (a 5a coluna foi removida) */
static void bay_engine(i16 cy, u8 dots) {
    u8 e = gcar->eng > 2 ? 2 : gcar->eng, mm = gcar->man > 6 ? 6 : gcar->man, mi = 3 + mm, cc = gcar->carb > 2 ? 2 : gcar->carb, ci = 10 + cc, k, n;
    i16 ex = 128 - art_w[e] / 2, ey = cy + 14, pf = eng_lf[e], py = ey + eng_r[e] - art_h[mi], cf, cyy;       /* pf, cf: colunas nativas relativas ao motor */
    bay_nd = 0;
    if (bay_pr[0]) {
        art_put(e, ex, ey);
        if (dots) { bay_add(0, 0, ex + AX(eng_bxf[e]), ey + eng_byf[e]); bay_add(0, 1, ex + AX(eng_wxf[e]), ey + eng_wyf[e]); }
        else art_put(18, ex + AX(eng_wxf[e]), ey + eng_wyf[e]);
    }
    if (bay_pr[1]) {
        art_put(mi, ex + AX(pf), py);
        if (dots) { bay_add(1, 0, ex + AX(pf + 10), py + 3); bay_add(1, 1, ex + AX(pf + pw_f[mm] - 10), py + 3); }
    }
    if (bay_pr[2]) for (k = 0; k < mp_n[mm]; k++) {
        cf = pf + pc_f[mm][k] + 1 + k - cw_f[cc] / 2; cyy = py + 9 - art_h[ci];
        art_put(ci, ex + AX(cf), cyy);
        if (dots) { bay_add(2, 2 * k, ex + AX(cf + 9), cyy + 6); bay_add(2, 2 * k + 1, ex + AX(cf + cw_f[cc] - 13), cyy + 6); }
    }
    for (k = 0; k < bay_nd; k++) {                      /* parafusos e furos por cima de tudo */
        n = bay_dsl[k];
        bay_dot(bay_dx[k], bay_dy[k], bay_s[n][bay_dix[k]], n == 0 && bay_dix[k] == 1);
        bay_hot[3 + k].x0 = bay_dx[k] - 1; bay_hot[3 + k].y0 = bay_dy[k]; bay_hot[3 + k].x1 = bay_dx[k] + (n == 0 && bay_dix[k] == 1 ? 14 : 8); bay_hot[3 + k].y1 = bay_dy[k] + 11; bay_hot[3 + k].id = 3 + k;
    }
}
static void bay_bubble(const char *m) {                 /* balao de fala no alto da janela */
    u8 n = 0, x;
    while (m[n]) n++;
    x = BW_X + BW_W / 2 - 3 * n;
    push_fill(x - 4, BW_Y + 6, 6 * n + 8, 14, HC_W); push_fill(x - 3, BW_Y + 7, 6 * n + 6, 12, 3); gtext(x, BW_Y + 9, m, 0);
}
static void bay_click(u8 d) {                           /* clique no parafuso/fiacao d */
    u8 sl = bay_dsl[d], ix = bay_dix[d], n = bay_nsl(sl), i, id, all, *sp = &bay_s[sl][ix];
    if (sl == 0 && ix == 1) {                           /* fiacao: um clique */
        if (*sp == 0 && bay_pr[1]) { bay_msg = "TAKE THE MANIFOLD OFF FIRST"; return; }
        *sp = *sp ? 0 : 4;
    } else switch (*sp) {
    case 0:                                             /* comeca a soltar */
        if (sl == 1 && bay_pr[2]) { bay_msg = "TAKE THE CARBS OFF FIRST"; return; }
        if (sl == 0 && bay_pr[1]) { bay_msg = "TAKE THE MANIFOLD OFF FIRST"; return; }
        *sp = 1; break;
    case 1: *sp = 4; break;                             /* parafuso sai */
    case 4: *sp = 3; break;                             /* o parafuso volta */
    case 3: case 2:                                     /* apertando: o encaixe de baixo precisa estar pronto */
        if (sl == 1 && !bay_tight(0)) { bay_msg = "BOLT THE ENGINE FIRST"; return; }
        if (sl == 2 && !bay_tight(1)) { bay_msg = "BOLT THE MANIFOLD FIRST"; return; }
        *sp = *sp == 3 ? 2 : 0; break;
    }
    for (i = 0, all = 1; i < n; i++) if (bay_s[sl][i] != 4) all = 0;
    if (all && bay_pr[sl]) {                            /* todos os parafusos fora: a peca some e vai para as reservas */
        id = part_find(bay_cat[sl], *car_var(gcar, bay_cat[sl]), car_maker(gcar));
        if (id == 255 || nspare >= MAXSPARE) { bay_msg = id == 255 ? "CAN'T TAKE THIS ONE OFF" : "NO ROOM FOR MORE SPARES"; *sp = 0; return; }
        spare[nspare++] = id; bay_pr[sl] = 0;
    }
}
/* TUNING (original): janela inteira da LIB2 (item 71, 160x175) na pagina 1; setas (cima = atrasar, baixo = adiantar) e DONE; a correia dentada gira conforme o ponto */
#define TX 48
#define TY 8
static Hot tune_hot[3] = {{TX + 84, TY + 46, TX + 98, TY + 70, 0}, {TX + 84, TY + 78, TX + 98, TY + 100, 1}, {TX + 62, TY + 146, TX + 98, TY + 162, 2}};
static void tune_blit(u16 sx, u16 sy, i16 dx, i16 dy, u8 w, u8 h, u8 cmd) {
    vc_w(0, sx); vc_w(2, sy); vc_w(4, dx); vc_w(6, dy); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = cmd;
    vdp_cmd15();
}
static void car_tune_win(void) {
    static const char *const nm[3] = {"RETARD THE TIMING", "ADVANCE THE TIMING", "DONE TUNING"};
    char b[10]; u8 n, k;
    cmd_n = 0; cp = cmd_buf; hub_restore(BW_X, BW_Y, BW_W, BW_H); vdp_wait();             /* apaga a janela do motor (a TUNING e mais alta e mais estreita) */
    g_group = 69; load_assets(); art_grp_cur = 69;
    for (;;) {
        tune_blit(0, 256, TX, TY, 160, 175, 0xD0);                                          /* HMMM: a arte inteira */
        tune_blit(164 + 6 * ((gcar->trim + 8) & 3), 256, TX + 71, TY + 53, 5, 29, 0x90);    /* LMMM IMP: correia dentada na fase do ponto */
        cmd_n = 0; cp = cmd_buf;
        gtext(TX + 9, TY + 146, "TIMING", 1);
        n = 0; if (gcar->trim < 0) b[n++] = '-'; else if (gcar->trim) b[n++] = '+'; put_num(b, &n, gcar->trim < 0 ? -gcar->trim : gcar->trim); b[n] = 0;
        gtext(TX + 9, TY + 156, b, 1);
        gtext(TX + 117, TY + 146, "POWER", 1);
        n = 0; put_num(b, &n, car_power(gcar) / 100); b[n] = 0;
        gtext(TX + 117, TY + 156, b, 1);
        vdp_wait(); cmd_n = 0; cp = cmd_buf;
        hs = tune_hot; hn = 3; hnames = nm; lab_mode = 3; pick_tick = 0; cur_ymax = 210;
        k = pick_hot(1);
        lab_mode = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187;
        if (k >= 2) { cmd_n = 0; cp = cmd_buf; hub_restore(TX, TY, 160, 175); vdp_wait(); return; }     /* devolve a garagem por baixo da janela alta */
        if (k == 1 && gcar->trim < 8) gcar->trim++;
        if (k == 0 && gcar->trim > -8) gcar->trim--;
    }
}
static void part_art(u8 cat, i16 cx, i16 cy) {          /* a peca instalada na categoria cat, centrada em (cx, cy) */
    u8 v = *car_var(gcar, cat), i = cat == 0 ? (v > 2 ? 2 : v) : cat == 1 ? 19 + (v > 3 ? 3 : v) : cat == 2 ? 10 + (v > 2 ? 2 : v) : cat == 3 ? 3 + (v > 6 ? 6 : v) : 23 + (v > 3 ? 3 : v);
    art_put(i, (cx - art_w[i] / 2) & ~1, cy - art_h[i] / 2);
}
#define PX 8                                            /* a janela das pecas usa a tela toda: os motores sao largos */
#define PW 240
static void parts_win(void) {
    static const char *const nm[7] = {"THE ENGINE", "THE TRANSMISSION", "THE CARBURETOR", "THE MANIFOLD", "THE TIRES", "LIST THE SPARE PARTS", "BACK TO THE ENGINE"};
    static const char *const lab[5] = {"ENGINE", "TRANS", "CARB", "MANIFOLD", "TIRES"}, *const plu[5] = {"ENGINES", "TRANSMISSIONS", "CARBURETORS", "MANIFOLDS", "TIRES"};
#ifndef PARTS_CAT
#define PARTS_CAT 0                                      /* (teste: categoria inicial) */
#endif
    u8 cat = PARTS_CAT, k, i, r, old, bub = 0, id, n, fit = car_maker(gcar), sl; char b[30];
#ifdef PARTS_CUR_X
    cur_x = PARTS_CUR_X; cur_y = PARTS_CUR_Y;
#endif
    for (;;) {
        win_frame(PX, PW);
        for (i = 0; i < 5; i++) red_btn(i, PX + 8, BW_Y + 8 + 19 * i, 58, lab[i]);
        sl = cat == 0 ? 0 : cat == 3 ? 1 : cat == 2 ? 2 : 3;                   /* encaixe do motor (3 = fora do motor: cambio e pneus) */
        if (sl == 3 || bay_pr[sl]) part_art(cat, PX + 156, BW_Y + 66);
        id = part_find(cat, *car_var(gcar, cat), fit);
        n = 0; if (sl == 3 || bay_pr[sl]) { if (id != 255) put_str(b, &n, PARTN(id)); } else put_str(b, &n, "(EMPTY)"); b[n] = 0;
        gtext(PX + 156 - 3 * n, BW_Y + 9, b, 1);
        if (bub) {                                       /* balao de fala: nao ha peca reserva */
            n = 0; if (bub == 2) put_str(b, &n, "UNBOLT THE OLD ONE FIRST"); else { put_str(b, &n, "NO SPARE "); put_str(b, &n, plu[cat]); } b[n] = 0;
            push_fill(PX + 156 - 3 * n - 4, BW_Y + 6, 6 * n + 8, 14, HC_W); push_fill(PX + 156 - 3 * n - 3, BW_Y + 7, 6 * n + 6, 12, 3); gtext(PX + 156 - 3 * n, BW_Y + 9, b, 0);
        }
        red_btn(5, PX + 8, BW_BTN, 100, "CHOOSE"); red_btn(6, PX + 132, BW_BTN, 100, "DONE");
        k = bay_pick(7, nm);
        if (k < 5) cat = k;                              /* clicar numa secao abre a lista das pecas reservas dela; SELECT repete a lista da secao atual */
        else if (k != 5) return;
        sl = cat == 0 ? 0 : cat == 3 ? 1 : cat == 2 ? 2 : 3;
        bub = 0;
        if (sl < 3 && bay_pr[sl]) { bub = 2; continue; }  /* peca do motor: primeiro desaparafusar a antiga (clique nos parafusos) */
        gsn = 0;
        for (i = 0; i < nspare; i++) if (part_cat[spare[i]] == cat && (part_mk[spare[i]] & fit)) gsp[gsn++] = i;
        if (!gsn) { bub = 1; continue; }
        ui_get = spare_cat_get; ui_nohint = 0;
        r = ui_list(cat_label[cat], gsn, 0);             /* lista das pecas disponiveis (X ou ESC cancela) */
        if (r == 255) continue;
        r = gsp[r];
        old = part_find(cat, *car_var(gcar, cat), fit);
        *car_var(gcar, cat) = part_var[spare[r]];
        if (cat == 0) gcar->wear = 0;
        if (cat == 4) gcar->tp = 0;
        if (sl < 3) { spare[r] = spare[--nspare]; bay_pr[sl] = 1; for (i = 0; i < 6; i++) bay_s[sl][i] = 3; if (sl == 0) bay_s[0][1] = 4; }   /* a peca nova aparece solta: aparafusar na janela do motor */
        else if (old != 255) spare[r] = old; else spare[r] = spare[--nspare];
    }
}
static void engine_bay(void) BK {
    u8 k;
    if (!nown) { ui_note("NO CAR YET", "BUY ONE FIRST!"); return; }
    gcar = &gc[cur]; art_grp_cur = 0; if (bay_bad != cur) bay_reset(); bay_msg = 0;
#ifdef FORCE_PARTS
    gcar->eng = FORCE_PARTS >> 8; gcar->man = (FORCE_PARTS >> 4) & 15; gcar->carb = FORCE_PARTS & 15;     /* testes: motor, coletor e carburador (0x2 5 2 = motor 78, coletor 104, carburador 86) */
#endif
#ifdef TEST_SPARES
    spare[0] = 0; spare[1] = 6; nspare = 2;                  /* testes: dois motores GM de reserva */
#endif
#ifdef BAY_CUR_X
    cur_x = BAY_CUR_X; cur_y = BAY_CUR_Y;                  /* so para testes automatizados */
#endif
#ifdef TEST_TUNE
    car_tune_win();
#endif
    for (;;) {
        win_frame(BW_X, BW_W);
        bay_engine(BW_Y + 26, 1);
        if (bay_msg) { bay_bubble(bay_msg); bay_msg = 0; }
        red_btn(0, BW_X + 10, BW_BTN, 52, "TUNE"); red_btn(1, BW_X + 66, BW_BTN, 52, "PARTS"); red_btn(2, BW_X + 122, BW_BTN, 52, "DONE");
        k = bay_pick(3 + bay_nd, bay_nm);
        if (k >= 3 && k < 13) bay_click(k - 3);
        else if (k == 0) { if (bay_ok()) car_tune_win(); else bay_msg = "TIGHTEN ALL THE BOLTS FIRST"; }
        else if (k == 1) parts_win();
        else if (k != 255) { bay_bad = bay_ok() ? 255 : cur; return; }         /* (255 = nada: so o roteiro de teste) */        /* DONE/ESC: se sobrou parafuso solto o carro nao anda */
    }
}
#endif
#pragma codeseg BANK2
#ifdef OLD_MENU
static void calendar_screen(void) {}
static void gas_station(void) {}
#else
static u8 gas_menu(void) {                     /* 1 = abasteceu */
    static char b1[26], b2[26];
    const char *it[3]; u8 n, gal = 20 - gc[cur].fuel, five, k, g; u16 cost;
    if (!gal) { ui_note("YOU'VE GOT", "A FULL TANK!"); return 0; }
    five = gal > 5 ? 5 : gal;
    n = 0; put_str(b1, &n, "FILL IT UP  $"); put_num(b1, &n, (gal + 2) / 3); b1[n] = 0;
    n = 0; put_num(b2, &n, five); put_str(b2, &n, " GALLONS  $"); put_num(b2, &n, (five + 2) / 3); b2[n] = 0;
    it[0] = b1; it[1] = b2; it[2] = "NO THANKS";
    k = ui_pick("GUS' GAS", it, 3, 0);
    if (k >= 2) return 0;
    g = k == 0 ? gal : five; cost = (g + 2) / 3;
    if (money < cost) { ui_note("GAS AIN'T FREE!", "YOU'RE BROKE"); return 0; }
    money -= cost; gc[cur].fuel += g;
    ui_note("FILL 'ER UP!", "TANK FILLED");
    return 1;
}
static const Hot gas_hot[2] = {{14, 90, 60, 172, 0}, {214, 201, 250, 211, 3}};
static void gas_station(void) {                /* o posto Gus' Gas (LIB2 item 90): o carro entra pela esquerda e para ao lado da bomba; clique na bomba para abastecer; sai para a frente */
    u8 id, r, p, done = 0; i16 xl;
    if (!nown) { ui_note("BUY A CAR FIRST!", "NOTHING TO FILL"); return; }
    id = gc[cur].id; cvar = gc[cur].roof | (gc[cur].bump << 1); cstk = gc[cur].stick; chead = 255;
    g_group = 243; load_assets();
    bar_news = 2; cur_ymax = 210; car_floor = 162; fr_n = 2;
    cmd_n = 0; cp = cmd_buf; hub_status(); band_save();
    car_art(id, gc[cur].tir); set_paint(gc[cur].paint);
    xl = 256 - car_w[id] - 2; if (xl > 50) xl = 50;
    car_slide(id, -(i16)car_w[id], xl);
    for (p = 0; p < 2; p++) { band_cmd(BAND_COPY_Y, p * 256 + BAND_Y); car_blit(p * 256, id, xl); }
    vdp_wait(); vdp_reg(2, 0x1F);
#ifdef GAS_CUR_X
    cur_x = GAS_CUR_X; cur_y = GAS_CUR_Y;
#endif
    while (!done) {
        hs = gas_hot; hn = 2; lab_mode = 1; pick_tick = 0;
        r = pick_hot(1);
        if (r == 254 || r == 3) break;
        done = gas_menu();
        cmd_n = 0; cp = cmd_buf; hub_restore(0, 0, 256, 190); hub_status(); car_blit(0, id, xl); vdp_wait();
    }
    car_slide(id, xl, 256);                                                   /* sai para a frente */
    lab_mode = 0; bar_news = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187; car_floor = CAR_FLOOR; fr_n = 0;
    load_garage(); band_save();
}
static void cfill(i16 xa, i16 y, i16 w, i16 h, u8 c);
static const Hot cal_hot[2] = {{0, 0, 255, 189, 0}, {214, 201, 250, 211, 3}};
static void calendar_screen(void) {            /* o calendario AGO/SET (LIB2 item 221) com a data de hoje marcada e o dia final (15/set) circulado na arte; qualquer clique/ESC/EXIT volta */
    static const u8 colx[7] = {35, 50, 63, 77, 91, 106, 120};
    u8 mo = day >= 31, dm = mo ? day - 30 : day + 1, first = mo ? 0 : 4, k = first + dm - 1, col = k % 7, row = k / 7;   /* agosto comeca na quinta (coluna 4), setembro no domingo */
    i16 cx, cy;
    g_group = 242; load_assets();
    cx = CAL_X0 + colx[col]; cy = CAL_Y0 + (mo ? 110 : 19) + row * 7 - (mo ? 0 : 0);
    cmd_n = 0; cp = cmd_buf;
    cfill(cx - 8, cy - 4, 17, 1, 9); cfill(cx - 8, cy + 6, 17, 1, 9); cfill(cx - 8, cy - 4, 1, 11, 9); cfill(cx + 8, cy - 4, 1, 11, 9);     /* moldura verde em volta de hoje */
    vdp_wait();
    bar_news = 2; cur_ymax = 210; hub_status();
    hs = cal_hot; hn = 2; lab_mode = 1; pick_tick = 0;
    pick_hot(1);
    lab_mode = 0; bar_news = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187;
    load_garage(); band_save();
}
#endif
static const char *const rd_name[6] = {"ROCK 'N' ROLL", "SLOW BLUES", "SWING JAZZ", "DOO-WOP", "SURF ROCK", "RADIO OFF"};
static char rd_buf[24];
static const char *radio_get(u8 i) {
    u8 n = 0;
    rd_buf[n++] = (i == 5 ? !mus_st : mus_st == i + 1) ? '>' : ' '; rd_buf[n++] = ' ';
    put_str(rd_buf, &n, rd_name[i]); rd_buf[n] = 0;
    return rd_buf;
}
static void radio(void) {                                              /* o radio da prateleira: 5 estacoes ou desligar */
    u8 r;
    ui_get = radio_get; r = ui_list("CATCH SOME TUNES", 6, mus_st ? mus_st - 1 : 5);
    if (r != 255) mus_set(r == 5 ? 0 : r + 1);
}
#ifndef OLD_MENU
static const Hot news_hot[5] = {{0, 0, 255, 31, 0}, {0, 32, 255, 66, 1}, {0, 67, 255, 135, 2}, {132, 201, 208, 211, 2}, {214, 201, 250, 211, 3}};
static const char *const news_name[3] = {"AUTO PARTS", "USED CARS", "SELL SPARE PARTS"};
static const u8 news_top[5][2] = {{16, 30}, {30, 30}, {16, 16}, {30, 30}, {30, 30}};
static const char *const news_body[5][2][4] = {
    {{"Koufax fans eleven,", "LA beats Cincinnati", "before 50,000 fans.", "Final score: 5 to 2."}, {"Waves flatten beach", "houses. No injuries."}},
    {{"Teen dies in crash.", "Police warn racers."}, {"Radar patrols on", "all main roads."}},
    {{"Detroit says small", "imports will never", "sell in America.", "Quote of the day."}, {"Dealers shrug off", "the new Toyota:", "'just a toy.'", "Sales stay flat."}},
    {{"Lawrence of Arabia", "leads with ten."}, {"Test rocket lost at", "Cape Canaveral."}},
    {{"Advisers only, says", "the White House."}, {"Congress wants more", "answers on the war."}}};
static void newspaper(void) {                  /* a primeira pagina do "Los Angeles News" (LIB2 item 21): clique em AUTO PARTS / USED CARS; o titulo vende pecas avulsas */
    u8 r;
    u8 k, c, l;
    g_group = 190; load_assets(); g_group = 191 + day % 5; load_assets();
    ybase = 512; k = day % 5;                                    /* texto legivel (fonte da garagem) no lugar dos rabiscos, desenhado na copia (pagina 2) e copiado para a tela */
    for (c = 0; c < 2; c++)
        for (l = 0; l < 4 && news_body[k][c][l]; l++)
            if (news_top[k][c] + 2 + 9 * l + 8 <= 53) gtext(c ? 134 : 6, 136 + news_top[k][c] + 2 + 9 * l, news_body[k][c][l], 0);
    ybase = 0; hub_restore(0, 0, 256, 190);
    bar_news = 1; cur_ymax = 210;
    cmd_n = 0; cp = cmd_buf; hub_status();
    for (;;) {
        hs = news_hot; hn = 5; hnames = news_name; lab_mode = 1;
        r = pick_hot(1);
        if (r == 254 || r == 3) break;
        if (r == 0) parts_shop(); else if (r == 1) used_cars(); else sell_parts();
        cmd_n = 0; cp = cmd_buf; hub_restore(0, 0, 256, 190); hub_status();
    }
    lab_mode = 0; bar_news = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187;
    load_garage(); band_save();                  /* de volta a garagem: o hub redesenha o carro */
}
/* ---- Bob's: o carro do oponente (o mesmo desenho da garagem) chega sozinho pela esquerda, fica parado e, se ninguem o desafia, sai para a frente (direita) ---- */
static u8 bst, bpil, bcar;                              /* bst: 0 vazio, 2 parado */
static u16 bt; static i16 bxl;
static Hot bobs_hot[2];
static char bpn[14];
static const char *bobs_name[1];
static void cfill(i16 xa, i16 y, i16 w, i16 h, u8 c) {   /* retangulo de uma cor (LMMV) */
    vc_w(4, xa); vc_w(6, y); vc_w(8, w); vc_w(10, h); vc[12] = c; vc[13] = 0; vc[14] = 0x80; vdp_cmd15();
}
static void bob_show(void) {                            /* carro parado + cabeca nas paginas 0 e 1 (a 2 fica limpa: e' a copia para restaurar) */
    u8 p;
    for (p = 0; p < 2; p++) { band_cmd(BAND_COPY_Y, p * 256 + BAND_Y); car_blit(p * 256, bcar, bxl); }
    vdp_wait(); vdp_reg(2, 0x1F);
}
static void bobs_hotup(void) {
    bobs_hot[0].x0 = 255; bobs_hot[0].x1 = 0;
    if (bst == 2) {                                                        /* a cabeca do piloto (e a janela em volta) e clicavel */
        i16 hx = head_x(bcar, bxl) + (head_w[chead] >> 1), hy = head_y(bcar) + (head_h[chead] >> 1);
        bobs_hot[0].x0 = hx - 14; bobs_hot[0].x1 = hx + 14; bobs_hot[0].y0 = hy - 12; bobs_hot[0].y1 = hy + 12; bobs_hot[0].id = 0;
    }
}
static void bobs_tick(void) {
    u16 now = JIFFY; u8 p, n = 0; const char *q;
    if (bst == 0 && (i16)(now - bt) >= 0) {                                  /* chega um carro: piloto e carro sorteados */
        u8 c;
        p = (u8)(rnd16() % NPILOTS);
        if (p == bpil) p = (p + 1) % NPILOTS;
        pick_opp_car(p); bpil = p; bcar = sel_opp;
        q = PILN(p); while (q[n] && n < 12) { bpn[n] = q[n]; n++; } bpn[n] = 0;
        c = (u8)(rnd16() % 12); cvar = 0; cstk = 0; chead = p;
        car_art(bcar, (car_flags[bcar] >> 4) & 3); g_group = 220 + p; load_assets(); set_paint(c);
        bxl = (256 - car_w[bcar]) >> 1;
        car_slide(bcar, -(i16)car_w[bcar], bxl);                              /* entra pela esquerda, como na garagem */
        bst = 2; bob_show(); bt = JIFFY + (u16)jhz * (8 + rnd16() % 5);
    } else if (bst == 2 && (i16)(now - bt) >= 0) {                           /* ninguem desafiou: sai para a frente */
        bst = 0; bobs_hotup();
        car_slide(bcar, bxl, 256);
        for (p = 0; p < 2; p++) band_cmd(BAND_COPY_Y, p * 256 + BAND_Y);
        vdp_wait(); vdp_reg(2, 0x1F);
        bt = JIFFY + (u16)jhz * (2 + rnd16() % 3);
    }
    bobs_hotup();
}
static void bobs_repaint(void) { if (bst == 2) { car_blit(0, bcar, bxl); vdp_wait(); } }
static u8 bobs_front(void) {                   /* a lanchonete (LIB2 item 93): os oponentes chegam sozinhos; clique na cabeca do piloto para ver quem e e desafiar. 1 = a corrida vai comecar */
    u8 r, k; char cn[30]; const char *it[3];
    bobs_name[0] = bpn; bpn[0] = 0; car_floor = 157;
    g_group = 197; load_assets();
    cmd_n = 0; cp = cmd_buf; bar_news = 2; cur_ymax = 210; hub_status();
    band_save();                                                              /* faixa limpa do cenario (pagina 3), usada pelas animacoes */
    bst = 0; bpil = 255; bt = JIFFY + jhz; bobs_hot[1].x0 = 214; bobs_hot[1].y0 = 201; bobs_hot[1].x1 = 250; bobs_hot[1].y1 = 211; bobs_hot[1].id = 3;
#ifdef BOB_CUR_X
    cur_x = BOB_CUR_X; cur_y = BOB_CUR_Y;
#endif
    for (;;) {
        hs = bobs_hot; hn = 2; hnames = bobs_name; lab_mode = 2; pick_tick = bobs_tick;
        r = pick_hot(1);
        pick_tick = 0;
        if (r == 254 || r == 3) break;
        sel_pil = bpil; sel_opp = bcar;                                       /* quem e e com que carro */
        for (k = 0; k < 28 && CARN(sel_opp)[k]; k++) cn[k] = CARN(sel_opp)[k];
        cn[k] = 0; it[0] = cn; it[1] = "CHALLENGE"; it[2] = "NOT NOW";
        do { k = ui_pick(bpn, it, 3, 1); } while (k == 0);
        if (k == 1 && bobs_race(sel_pil)) { lab_mode = 0; bar_news = 0; cur_ymax = 187; car_floor = CAR_FLOOR; chead = 255; return 1; }
        cmd_n = 0; cp = cmd_buf; hub_restore(0, 0, 256, 190); hub_status(); bobs_repaint();
        if (bst == 2) bt = JIFFY + (u16)jhz * (8 + rnd16() % 5);              /* o tempo de espera recomeca */
    }
    lab_mode = 0; bar_news = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187; car_floor = CAR_FLOOR; chead = 255;
    load_garage(); band_save();
    return 0;
}
#endif
#ifdef OLD_MENU
static u8 bobs_front(void) { return bobs(); }
#endif
static void race_go(void) BK { mus_pause(); run_race(); mus_hold = 0; mus_due = JIFFY; cursor_init(); cmd_n = 0; cp = cmd_buf; }   /* a corrida usa a VRAM dos sprites: recria o cursor */
static void race_result(void) BK {              /* resultado da corrida: janela sobre a lanchonete (tempo e velocidade media) */
    static char tb1[22], tb2[22];
    const char *it[2]; u16 t10 = (u16)(((u32)race_ticks * 1687) >> 10), avg = race_ticks ? (u16)(sum_spd / race_ticks) : 0; u8 n;
    load_garage(); g_group = 197; load_assets();
    ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cmd_n = 0; cp = cmd_buf; hub_status();
    ui_dbg = 2; dbg_result = result;
    if (result == 3) { ui_note("YA JUMPED THE LIGHT,", "SPEEDY!"); ui_dbg = 7; return; }
    n = 0; put_str(tb1, &n, "TIME  "); put_num(tb1, &n, t10 / 10); tb1[n++] = '.'; put_num(tb1, &n, t10 % 10); put_str(tb1, &n, " SEC"); tb1[n] = 0;
    n = 0; put_str(tb2, &n, "AVERAGE  "); put_num(tb2, &n, avg); put_str(tb2, &n, " MPH"); tb2[n] = 0;
    it[0] = tb1; it[1] = tb2;
    ui_nohint = 0;
    ui_pick(result == 1 ? "YOU WIN!" : "YOU LOSE!", it, 2, 0);
    ui_dbg = 7;
}
#pragma codeseg BANK4
/* ---- salvar / carregar: SRAM do PAC / FM-PAC (qualquer slot com a SRAM liberada por 0x4D/0x69 em 0x5FFE/0x5FFF) ou arquivo STREETRO.SAV no disco (BDOS do Disk BASIC) ---- */
#define SV_OFS 0x1D00                          /* dentro dos 8 KB da SRAM do PAC, perto do fim (os primeiros bytes pertencem ao sistema de arquivos do PAC) */
#define SV_HDR 13
#define SV_SP (SV_HDR + 13)                    /* depois do nome (12 letras + 0) */
#define SV_GC (SV_SP + MAXSPARE)
#define SV_PN (SV_GC + sizeof(gc))                  /* 22 bytes pil_net + races_tot + king_st */
#define SV_N (SV_PN + 24 + 2)
static u8 sv_media, sv_slot, sv_buf[128], sv_chk[128], sv_fcb[37];     /* sv_media: 0 nenhum, 1 PAC, 2 disco */
#ifdef FAKE_DISK
/* BDOS falso (so para teste): arquivos STREETRO.SAV/.RNK em RAM; confere a sequencia de chamadas do DOS 1 (DTA antes de ler/gravar, FCB com nome em branco-preenchido e tamanho de registro 128) */
static u8 fk_file[2][256], fk_ex[2], fk_cr[2], fk_len[2], fk_dta_ok, fk_bad, fk_tramp[15]; static u16 fk_dta;
static void fake_bdos(void) {
    u8 *f = (u8 *)g_bk_src, i, id = f[9] == 'R', r = 0, k; static const char nm[9] = "STREETRO";
    g_val = 0;
    if (g_reg == 0x1A) { fk_dta = g_bk_src; fk_dta_ok = 1; return; }                     /* DE = DTA, nao FCB */
    for (i = 0; i < 8; i++) if (f[1 + i] != nm[i]) fk_bad = 1;
    if (f[10] != (id ? 'N' : 'A') || f[11] != (id ? 'K' : 'V')) fk_bad = 1;
    switch (g_reg) {
        case 0x1A: fk_dta = g_bk_src; fk_dta_ok = 1; g_val = 0; return;
        case 0x13: fk_ex[id] = 0; return;
        case 0x16: fk_ex[id] = 1; fk_len[id] = 0; f[32] = 0; return;
        case 0x0F: if (!fk_ex[id]) g_val = 0xFF; f[32] = 0; return;
        case 0x15: if (!fk_dta_ok || f[14] != 128) { fk_bad = 1; g_val = 0xFF; return; } r = f[32]; for (k = 0; k < 128; k++) fk_file[id][r * 128 + k] = ((u8 *)fk_dta)[k]; f[32] = r + 1; if (fk_len[id] < r + 1) fk_len[id] = r + 1; return;
        case 0x14: if (!fk_dta_ok || f[14] != 128) { fk_bad = 1; g_val = 0xFF; return; } r = f[32]; if (r >= fk_len[id]) { g_val = 1; return; } for (k = 0; k < 128; k++) ((u8 *)fk_dta)[k] = fk_file[id][r * 128 + k]; f[32] = r + 1; return;
        case 0x10: return;
        default: fk_bad = 1; g_val = 0xFF;
    }
}
static void fake_install(void) {                 /* JP 0xF37D -> trampolim em RAM: C = funcao, DE = FCB/DTA -> g_reg, g_bk_src; chama fake_bdos; A = g_val */
    u8 *t = fk_tramp; u16 a;
    *t++ = 0x79; *t++ = 0x32; a = (u16)&g_reg; *t++ = (u8)a; *t++ = a >> 8;
    *t++ = 0xED; *t++ = 0x53; a = (u16)&g_bk_src; *t++ = (u8)a; *t++ = a >> 8;
    *t++ = 0xCD; a = (u16)fake_bdos; *t++ = (u8)a; *t++ = a >> 8;
    *t++ = 0x3A; a = (u16)&g_val; *t++ = (u8)a; *t++ = a >> 8; *t++ = 0xC9;
    *(volatile u8 *)0xF37D = 0xC3; *(volatile u16 *)0xF37E = (u16)fk_tramp;
}
#endif
static void sv_probe(u8 sl) {
    if (sl == cart_slot) return;
    g_val = 0; g_bk_bank = sl; sr_exec();
    if (g_val == 1) { sv_media = 1; sv_slot = sl; }
}
static void sv_init(void) BK {
    u8 p, s;
    sv_media = 0;
#ifdef FAKE_DISK
    fake_install();
#endif
    get_cart_slot(); cart_slot = g_val;
    for (p = 0; p < 4 && !sv_media; p++) {
        if (*(volatile u8 *)(0xFCC1 + p) & 0x80) { for (s = 0; s < 4 && !sv_media; s++) sv_probe(0x80 | (s << 2) | p); }
        else sv_probe(p);
    }
#ifdef NO_PAC
    sv_media = 0;
#endif
    if (!sv_media && *(volatile u8 *)0xF37D == 0xC3) sv_media = 2;       /* Disk BASIC instalou o gancho do BDOS */
}
static u8 sv_sum(const u8 *b, u8 n) { u16 s = 0; u8 i; for (i = 0; i < n; i++) s += b[i]; return (u8)(s ^ (s >> 8)); }
static void sv_pack(void) {
    u8 i; const u8 *g = (const u8 *)gc;
    for (i = 0; i < 128; i++) sv_buf[i] = 0;
    sv_buf[0] = 'S'; sv_buf[1] = 'R'; sv_buf[2] = 3; sv_buf[3] = SV_N; sv_buf[4] = nown; sv_buf[5] = cur; sv_buf[6] = day; sv_buf[7] = nspare;
    sv_buf[8] = wins_drag; sv_buf[9] = wins_road; sv_buf[10] = sel_car; sv_buf[11] = (u8)money; sv_buf[12] = (u8)(money >> 8);
    for (i = 0; i < 13; i++) sv_buf[SV_HDR + i] = pname[i];
    for (i = 0; i < MAXSPARE; i++) sv_buf[SV_SP + i] = spare[i];
    for (i = 0; i < sizeof(gc); i++) sv_buf[SV_GC + i] = g[i];
    for (i = 0; i < 22; i++) sv_buf[SV_PN + i] = (u8)pil_net[i];
    sv_buf[SV_PN + 22] = races_tot; sv_buf[SV_PN + 23] = king_st;
    sv_buf[SV_N - 2] = sv_sum(sv_buf, SV_N - 2); sv_buf[SV_N - 1] = (u8)~sv_buf[SV_N - 2];
}
static u8 sv_ok(const u8 *b) {                  /* 1 se o bloco e uma gravacao valida e coerente */
    u8 i;
    if (b[0] != 'S' || b[1] != 'R' || b[2] != 3 || b[3] != SV_N) return 0;
    if (b[SV_N - 2] != sv_sum(b, SV_N - 2) || b[SV_N - 1] != (u8)~b[SV_N - 2]) return 0;
    if (b[4] > MAXCARS || b[7] > MAXSPARE || (b[4] && b[5] >= b[4]) || b[6] > 62) return 0;
    for (i = 0; i < b[4]; i++) if (b[SV_GC + i * sizeof(Car)] >= sizeof(car_w)) return 0;
    return 1;
}
static u8 sv_bdos(u8 fn, u8 *de) { g_reg = fn; g_bk_src = (u16)de; bd_exec(); return g_val; }
static void sv_fcb_init(u8 rk) {                  /* STREETRO.SAV (jogo salvo) ou STREETRO.RNK (ranking) */
    static const char nm[12] = "STREETRODSAV", rn[3] = "RNK";
    u8 i;
    for (i = 0; i < 37; i++) sv_fcb[i] = 0;
    for (i = 0; i < 11; i++) sv_fcb[1 + i] = nm[i + (i > 7)];
    if (rk) for (i = 0; i < 3; i++) sv_fcb[9 + i] = rn[i];
}
static u8 sv_io(u8 *buf, u8 wr, u8 n, u16 ofs, u8 rk) {   /* le/grava n bytes (PAC: em ofs; disco: registros de 128 bytes do arquivo SAV ou RNK); 1 = ok */
    if (sv_media == 1) { g_val = wr ? 2 : 1; g_bk_bank = sv_slot; g_bk_src = ofs; g_bk_dst = (u16)buf; g_bk_len = n; sr_exec(); return 1; }
    if (sv_media == 2) {
        u8 r = 0; u16 k;                                                  /* u16: com u8, k += 128 dava a volta em 256 e o laco do ranking (2 registros) nunca terminava */
        if (wr) { sv_fcb_init(rk); sv_bdos(0x13, sv_fcb); }              /* apaga o antigo */
        sv_fcb_init(rk);
        if (sv_bdos(wr ? 0x16 : 0x0F, sv_fcb)) return 0;
        sv_fcb[14] = 128;
        for (k = 0; k < n && !r; k += 128) { sv_bdos(0x1A, buf + k); r = sv_bdos(wr ? 0x15 : 0x14, sv_fcb); }   /* DTA = 128 bytes por vez na RAM */
        sv_bdos(0x10, sv_fcb);
        return r == 0;
    }
    return 0;
}
#define SV_IO(b, w) sv_io(b, w, SV_N, SV_OFS, 0)
static u8 sv_exists(void) BK { return sv_media && SV_IO(sv_buf, 0) && sv_ok(sv_buf); }
static u8 sv_save(void) BK {
    u8 i;
    if (!sv_media) return 0;
    sv_pack();
    if (!SV_IO(sv_buf, 1)) return 0;
    if (!SV_IO(sv_chk, 0)) return 0;                                      /* confere o que ficou gravado */
    for (i = 0; i < SV_N; i++) if (sv_chk[i] != sv_buf[i]) return 0;
    return 1;
}
static u8 sv_load(void) BK {
    u8 i; u8 *g = (u8 *)gc;
    if (!sv_exists()) return 0;
    bay_bad = 255;
    nown = sv_buf[4]; cur = sv_buf[5]; day = sv_buf[6]; nspare = sv_buf[7]; wins_drag = sv_buf[8]; wins_road = sv_buf[9]; sel_car = sv_buf[10];
    money = sv_buf[11] | ((u16)sv_buf[12] << 8);
    for (i = 0; i < 13; i++) pname[i] = sv_buf[SV_HDR + i];
    pname[12] = 0;
    for (i = 0; i < MAXSPARE; i++) spare[i] = sv_buf[SV_SP + i];
    for (i = 0; i < sizeof(gc); i++) g[i] = sv_buf[SV_GC + i];
    for (i = 0; i < 22; i++) pil_net[i] = (i8)sv_buf[SV_PN + i];
    races_tot = sv_buf[SV_PN + 22]; king_st = sv_buf[SV_PN + 23];
    offer_day = 255; game_end = 0;
    return 1;
}
/* ---- ranking "King Street Rodders": os 10 melhores bankrolls finais, guardados no mesmo meio do save (PAC+0x1B00 ou STREETRO.RNK); sem PAC nem disco vale so na sessao ---- */
#define RK_OFS 0x1B00
#define RK_N 155                                /* 'R','K', versao, 10 x (nome 13 + pontos 2), checksum 2 */
static u8 rk_blob[256];
static void rk_defaults(void) {
    static const char *const dn[10] = {"KING", "SLICK", "BIG AL", "DUCKTAIL", "RED", "VINNIE", "BOBBY", "MOUSE", "LEFTY", "ROOKIE"};
    static const u16 dp[10] = {2900, 2300, 1800, 1500, 1200, 1000, 800, 650, 500, 350};
    u8 i, k;
    for (i = 0; i < 10; i++) { for (k = 0; dn[i][k]; k++) rk_name[i][k] = dn[i][k]; rk_name[i][k] = 0; rk_pts[i] = dp[i]; }
}
static void rk_pack(void) {
    u8 i, k, *p = rk_blob + 3;
    rk_blob[0] = 'R'; rk_blob[1] = 'K'; rk_blob[2] = 1;
    for (i = 0; i < 10; i++) { for (k = 0; k < 13; k++) *p++ = rk_name[i][k]; *p++ = (u8)rk_pts[i]; *p++ = (u8)(rk_pts[i] >> 8); }
    rk_blob[RK_N - 2] = sv_sum(rk_blob, RK_N - 2); rk_blob[RK_N - 1] = (u8)~rk_blob[RK_N - 2];
}
static u8 rk_unpack(void) {                     /* 1 = bloco valido (copiado para a tabela) */
    u8 i, k; const u8 *p = rk_blob + 3;
    if (rk_blob[0] != 'R' || rk_blob[1] != 'K' || rk_blob[2] != 1 || rk_blob[RK_N - 2] != sv_sum(rk_blob, RK_N - 2) || rk_blob[RK_N - 1] != (u8)~rk_blob[RK_N - 2]) return 0;
    for (i = 0; i < 10; i++) { for (k = 0; k < 13; k++) rk_name[i][k] = *p++; rk_name[i][12] = 0; rk_pts[i] = p[0] | ((u16)p[1] << 8); p += 2; }
    return 1;
}
static void rk_init(void) BK {                  /* no boot: tabela padrao, ou a gravada */
    rk_defaults();
    if (sv_media && sv_io(rk_blob, 0, RK_N, RK_OFS, 1)) rk_unpack();
    else rk_defaults();
}
static u8 rk_add(void) BK {                     /* poe o jogador (nome, dinheiro final) na tabela; devolve a posicao 0-9 ou 255 se nao entrou */
    u8 pos, i, k;
    for (pos = 0; pos < 10 && money <= rk_pts[pos]; pos++) ;
    if (pos >= 10) return 255;
    for (i = 9; i > pos; i--) { for (k = 0; k < 13; k++) rk_name[i][k] = rk_name[i - 1][k]; rk_pts[i] = rk_pts[i - 1]; }
    for (k = 0; k < 12 && pname[k]; k++) rk_name[pos][k] = pname[k];
    if (!k) { static const char d[7] = "SPEEDY"; for (k = 0; k < 6; k++) rk_name[pos][k] = d[k]; }
    rk_name[pos][k] = 0; rk_pts[pos] = money;
    if (sv_media) { rk_pack(); sv_io(rk_blob, 1, RK_N, RK_OFS, 1); }
    return pos;
}
#pragma codeseg BANK2
#ifdef OLD_MENU
static void name_screen(void) BK {}
#else
/* ---- nome do jogador: carteira DMV (LIB2 item 16, grupo 244) com teclado na tela (mouse) e o teclado do MSX (buffer da BIOS) ---- */
static Hot khot[29];                           /* 26 letras (ids 0-25), espaco 26, apaga 27, pronto 28 */
static u8 nm_n;
static void nm_show(void) {
    cmd_n = 0; cp = cmd_buf;
    hub_restore(112, 58, 82, 11);                                       /* a linha NAME: da carteira, limpa da copia na pagina 2 */
    gtext(114, 60, pname, 0);
    vdp_wait(); cmd_n = 0; cp = cmd_buf;
}
static void nm_key(u8 c) {                     /* 0..25 letra, 26 espaco, 27 apaga */
    if (c == 27) { if (nm_n) pname[--nm_n] = 0; }
    else if (nm_n < 12) pname[nm_n++] = c == 26 ? ' ' : 'A' + c;
    nm_show();
}
static void nm_tick(void) {
    key_get(); { u8 c = g_val;
        if (c >= 'a' && c <= 'z') c -= 32;
        if (c >= 'A' && c <= 'Z') nm_key(c - 'A');
        else if (c == ' ') nm_key(26);
        else if (c == 8 || c == 127) nm_key(27);
        else if (c == 13) pick_abort = 1;
    }
}
static void name_screen(void) BK {
    static const char kl[27] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    u8 i, r, x, y, k; char b[2];
    u8 rg1 = *(volatile u8 *)0xF3E0;
    clip_l = 0; clip_r = 256; ybase = 0;
    vdp_reg(1, rg1 & ~0x40); g_palp = (u16)pal_garage; pal_load2(); g_group = 244; load_assets(); font_init(); vdp_reg(2, 0x1F); vdp_reg(1, rg1 | 0x40);
    cmd_n = 0; cp = cmd_buf;
    for (i = 0; i < 26; i++) {                                                   /* teclas: duas fileiras de 13 */
        x = 4 + 19 * (i % 13); y = 157 + 13 * (i / 13);
        khot[i].x0 = x; khot[i].y0 = y; khot[i].x1 = x + 17; khot[i].y1 = y + 11; khot[i].id = i;
        push_fill(x, y, 18, 12, HC_K); push_fill(x + 1, y + 1, 16, 10, HC_W);
        b[0] = kl[i]; b[1] = 0; gtext(x + 6, y + 2, b, 0);
    }
    for (r = 0; r < 3; r++) {                                                    /* espaco, apaga, pronto */
        static const u8 bx[3] = {4, 93, 162}, bw[3] = {86, 66, 90};
        static const char *const bl[3] = {"SPACE", "DEL", "DONE"};
        x = bx[r]; y = 183; k = 26 + r;
        khot[k].x0 = x; khot[k].y0 = y; khot[k].x1 = x + bw[r] - 1; khot[k].y1 = y + 11; khot[k].id = k;
        push_fill(x, y, bw[r], 12, HC_K); push_fill(x + 1, y + 1, bw[r] - 2, 10, HC_W);
        gtext(x + (bw[r] - 6 * (r == 0 ? 5 : r == 1 ? 3 : 4)) / 2, y + 2, bl[r], 0);
    }
    gtext(4, 199, "TYPE YOUR NAME - RETURN WHEN DONE", 1);
    vdp_wait(); cmd_n = 0; cp = cmd_buf;
    nm_n = 0; while (pname[nm_n]) nm_n++;
    nm_show();
    hs = khot; hn = 29; lab_mode = 1; pick_tick = nm_tick; cur_ymax = 210;
    for (;;) {
        k = pick_hot(0);                                                         /* sem 'voltar' por teclado: BS apaga, nao sai */
        if (k >= 253 || k == 28) break;                                          /* RETURN ou DONE */
        nm_key(k);
    }
    pick_tick = 0; lab_mode = 0; cur_ymax = 187; if (cur_y > 187) cur_y = 187;
    if (!pname[0]) { static const char d[7] = "SPEEDY"; for (i = 0; i < 7; i++) pname[i] = d[i]; }
}
#endif
static void howto(void) {                        /* lista de comandos (primeira vez no jogo e na porta da garagem) */
    static const char *const how[6] = {"UP = GAS   DOWN = BRAKE", "LEFT / RIGHT = STEER", "SPACE OR BUTTON A = SHIFT UP", "SHIFT OR BUTTON B = SHIFT DOWN",
                                       "DON'T SHIFT BEFORE THE GREEN!", "ESC = BACK / STOP CRUISING"};
    ui_nohint = 0;
    ui_pick("HOW TO DRIVE", how, 6, 0);
}
static void hub_draw(void) BK {                    /* monta a garagem: cena, cursor, barra de status e o carro */
    load_garage();
    ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cursor_init();
    cmd_n = 0; cp = cmd_buf; hub_status();
    band_save(); draw_car();
}
static u8 hub(void) BK {                       /* 0 = ir ao Bob's (corrida), 1 = sair para o titulo */
    static const char *const yn[2] = {"YES", "NO"};
    u8 r;
    hub_draw();
    if (pm_on) { ui_note(pmb[0], pmb[1]); pm_on = 0; cmd_n = 0; cp = cmd_buf; hub_status(); }
#ifdef TEST_SAVE
    money = 777; day = 9; wins_drag = 5; pname[0] = 'Z'; pname[1] = 'E'; pname[2] = 'D'; pname[3] = 0; dbg_t1 = sv_save(); ask_load = 0;
#endif
#ifdef TEST_CYCLE
    money = 777; day = 9; pname[0] = 'Z'; pname[1] = 0; dbg_t1 = sv_save(); money = 1; day = 0; pname[0] = 0; dbg_t1 |= sv_load() << 1;
    dbg_t2 = money; dbg_t3 = pname[0] | ((u16)day << 8); money = 1700; rk_add(); rk_pts[3] = 0; rk_init(); dbg_power = rk_pts[3];
#ifdef FAKE_DISK
    dbg_t1 |= (u16)fk_bad << 4;
#endif
    ask_load = 0;
#endif
#ifdef TEST_LOAD
    dbg_t1 = sv_load(); dbg_t2 = pname[0] | ((u16)pname[2] << 8); ask_load = 0; cmd_n = 0; cp = cmd_buf; hub_status(); draw_car();
#endif
    if (ask_load) {                                     /* comeco do jogo: ha um jogo salvo? */
        ask_load = 0;
        if (sv_media && sv_exists()) {
            ui_nohint = 1;
            if (ui_pick("LOAD SAVED GAME?", yn, 2, 0) == 0 && sv_load()) { ask_name = 0; cmd_n = 0; cp = cmd_buf; hub_restore(0, 22, 256, 168); hub_status(); draw_car(); }
            ui_nohint = 0;
        }
    }
#ifdef BOLT_TEST
    engine_bay();
#endif
#ifdef TEST_HOW
    ask_name = 0; dbg_t1 = 0; howto(); dbg_t1 = 1;
#endif
    if (ask_name) { ask_name = 0; name_screen(); hub_draw(); }                       /* jogo novo: a carteira DMV pede o nome */
    for (;;) {
        if (game_end) return 1;
        if (!nown && money < 300 && !nspare) { game_end = 3; return 1; }
        r = scene_pick();
        if (r == 0) cars_screen();
        else if (r == 1) newspaper();
        else if (r == 2) calendar_screen();
        else if (r == 3) radio();
        else if (r == 7) body_shop();
        else if (r == 8) engine_bay();
        else if (r == 4) gas_station();
        else if (r == 5) {
            if (!nown) ui_note("BOBS DRIVE-IN", "BUY A CAR FIRST!");
            else if (gc[cur].brk || bay_bad == cur) ui_note("YOUR CAR'S NOT RUNNING,", "SPEEDY!");
            else if (gc[cur].fuel < 3) ui_note("YOU DON'T GOT ENOUGH GAS", "TO CRUISE DOWN TO BOBS");
            else if (street_trip() && bobs_front()) return 0;
        }
        else {                                          /* a porta: salvar, carregar ou sair */
            static const char *const dm[5] = {"SAVE GAME", "LOAD GAME", "HOW TO DRIVE", "CREDITS", "QUIT TO TITLE"};
            ui_nohint = 0;
            r = ui_pick("TIME TO QUIT", dm, 5, 0);
            if (r == 0) {
                if (!sv_media) ui_note("NO PAC OR DISK FOUND", "TO SAVE THE GAME");
                else if (bay_bad != 255) ui_note("FINISH THE ENGINE WORK", "BEFORE SAVING");
                else if (sv_save()) ui_note("GAME SAVED", sv_media == 1 ? "IN THE PAC." : "ON THE DISK.");
                else ui_note("COULDN'T SAVE THE GAME", "CHECK THE PAC OR DISK");
            } else if (r == 1) {
                if (!sv_media) ui_note("NO PAC OR DISK FOUND", "TO LOAD A GAME");
                else if (sv_load()) ui_note("GAME LOADED", "WELCOME BACK!");
                else ui_note("NO SAVED GAME FOUND", "");
            } else if (r == 2) howto();
            else if (r == 3) { credits(); hub_draw(); }
            else if (r == 4) { ui_nohint = 1; if (ui_pick("QUIT GAME?", yn, 2, 1) == 0) { ui_nohint = 0; return 1; } ui_nohint = 0; }
        }
        cmd_n = 0; cp = cmd_buf; hub_restore(0, 22, 256, 168); hub_status(); draw_car();
    }
}

#pragma codeseg BANK1
static void new_race(void) {
    u8 k = (u8)(JIFFY & 7);
    set_track(race_road);
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

/* janela esquerda (vidro lateral e coluna A, x < RACE_X0): pintada uma vez nas duas paginas (ceu, colinas, grama e a moldura do cockpit); so o resto e redesenhado a cada quadro */
static void left_static(void) {
    u8 pg, k;
    if (!OV_LEFT) return;                                   /* (a janela esquerda ja vem pronta na imagem do cockpit) */
    for (pg = 0; pg < 2; pg++) {
        i16 b = pg ? 256 : 0;
        clip_l = 0; clip_r = RACE_X0; ybase = b + WIN_Y;
        cmd_n = 0; cp = cmd_buf;
        push_fill(0, SKY_TOP, RACE_X0, 15, 0);
        vc_w(0, 0); vc_w(2, BG_Y); vc_w(4, 0); vc_w(6, ybase + SKY_TOP); vc_w(8, RACE_X0); vc_w(10, BG_H); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
        vdp_cmd15();
        push_fill(0, SKY_TOP + BG_H, RACE_X0, 129 - SKY_TOP - BG_H, 13);          /* grama ate o fim do vidro lateral (y 152 da tela) */
        vdp_wait();
        for (k = 0; k < OV_LEFT; k++) {
            i16 ox = ov_x[k] << 1, oy = ov_y[k];
            vc_w(0, ox); vc_w(2, 512 + WIN_Y + SKY_TOP + oy); vc_w(4, ox); vc_w(6, b + WIN_Y + SKY_TOP + oy);
            vc_w(8, ov_w[k] << 1); vc_w(10, ov_h[k]); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
            vdp_cmd15();
        }
    }
    vdp_wait();
}
#ifdef PROF
u16 prof[12];
#define PT(i) { u16 j_ = JIFFY; prof[i] += j_ - pj; pj = j_; }
#else
#define PT(i)
#endif
static void run_race(void) BK {
    u16 last_j, t, dj, j0, fr0, tickacc = 0, nt, start_j;
    u8 shown = 0, skip_held = 1;
    u16 base; u8 k;
#ifdef PROF
    u16 pj = JIFFY;
#endif
    dbg_frames = 0; dbg_t60 = 0; dbg_t100 = 0; dbg_top = 0;
    clip_l = VIEW_X0; clip_r = VIEW_X1;
    pal_load();
    new_race();
    init_static();
    left_static();
    clip_l = RACE_X0; clip_r = VIEW_X1;
    ybase = 256;
    tickacc = 0;
    last_j = JIFFY; j0 = last_j; fr0 = 0; start_j = last_j;
    for (;;) {
        t = JIFFY; dj = t - last_j; last_j = t;
        if (dj > 12) dj = 12;
#ifdef PROF
        pj = JIFFY; prof[11]++;
#endif
        read_input(); combine_input();
        handle_input();
        snd_engine(1);
        if (state == 0) {
            if (beep_stage == 0 && cd_t >= GO_J / 3) { beep_stage = 1; beep(254, 9); }
            else if (beep_stage == 1 && cd_t >= 2 * GO_J / 3) { beep_stage = 2; beep(254, 9); }
        }
        dbg_in = (g_t0 ? 1 : 0) | (up_held << 1) | (false_start << 2) | (gear << 4);
        if (state == 0) {
            cd_t += dj;
            if (false_start) { result = 3; state = 2; dbg_state = 2; race_silence(); return; }
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
                if (g_trig & 3) { if (!skip_held) { street_res = 0; psg(8, 0); psg(9, 0); psg(10, 0); beep_on = 0; return; } } else skip_held = 0;      /* o seletor pula a viagem (manual) */
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
                state = 2; dbg_state = 2; race_silence();
                return;                                     /* o resultado aparece no Bob's (race_result) */
            }
        }
        dbg_rpm = rpm; dbg_gear = gear; dbg_state = state; dbg_opos = opos; dbg_ospeed = vspd[1];
#ifdef TEST_MIRROR
        if (pos > 6) { opos = pos - 2 - (pos & 3) * 3; osub = sub; }          /* teste: oponente atras de voce, a 2-11 segmentos */
#endif
        PT(0)
        cmd_n = 0; cp = cmd_buf;
        project();
        PT(1)
        base = ybase; ybase = base + WIN_Y;                 /* janela da estrada: y relativo ao para-brisa */
        {                                                    /* horizonte: ceu preto + colinas e grama (item 267) alinhados ao fim da estrada; copia opaca (HMMM) so abaixo do vidro */
            i16 yh = Y[N_ROWS - 1] - BG_H, hh = BG_H, sy = 0;
            if (yh < SKY_TOP) { hh += yh - SKY_TOP; sy = SKY_TOP - yh; yh = SKY_TOP; }
            else if (yh > SKY_TOP) push_fill(0, SKY_TOP, 256, yh - SKY_TOP, 0);
#ifndef NO_BG
            if (hh > 0) {
                vc_w(0, RACE_X0); vc_w(2, BG_Y + sy); vc_w(4, RACE_X0); vc_w(6, ybase + yh);
                vc_w(8, 256 - RACE_X0); vc_w(10, hh); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;      /* HMMM */
                vdp_cmd15();
            }
#endif
        }
        push_fill(0, Y[N_ROWS - 1], 256, VIEW_BOT - Y[N_ROWS - 1], 13);     /* grama lisa: 1 retangulo (a pista e a borda amarela vem por cima) */
        PT(2)
        draw_road_asm();
        PT(3)
#ifndef NO_SCN
#ifdef PROF_SCN
        { u16 jj = JIFFY; draw_scn(); dbg_t2 += JIFFY - jj; dbg_t3++; }
#else
        draw_scn();
#endif
#endif
        PT(4)
        if (!street_mode) { draw_opp(); draw_lights(state == 0 || (state == 1 && (u16)(JIFFY - start_j) < 2 * jhz)); }
        PT(5)
#if HAVE_COCKPIT
#ifndef NO_OV
        /* moldura da esquerda (porta/coluna A) por cima da estrada: HMMM de retangulos justos da copia limpa (pagina 2).
           A estrada nao desce alem do para-brisa (VIEW_BOT), entao o painel/capo nunca e sobrescrito. */
        for (k = OV_LEFT; k < OV_N; k++) {
            i16 ox = ov_x[k] << 1, oy = ov_y[k];
            vc_w(0, ox); vc_w(2, 512 + WIN_Y + SKY_TOP + oy); vc_w(4, ox); vc_w(6, ybase + SKY_TOP + oy);
            vc_w(8, ov_w[k] << 1); vc_w(10, ov_h[k]); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
            vdp_cmd15();
        }
#endif
#endif
        PT(6)
        ybase = base;
        clip_l = 0; clip_r = 256;
#ifndef NO_HUD
        draw_hud_page();
#endif
        PT(7)
        clip_l = RACE_X0; clip_r = VIEW_X1;
        dbg_cmds = cmd_n; dbg_speed = speed; dbg_pos = pos;
        vdp_wait();
        PT(8)
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

#pragma codeseg BANK5
#include "assets_scn.h"
/* cenario da rua: por segmento da pista (scn_tab) ha arvores, placas e postes dos dois lados; tamanho e posicao seguem a largura da pista (hw) na linha do chao */
static void scn_blit(u16 sx, u16 sy, i16 x, i16 y, i16 w, i16 h, u8 opq) {   /* (x, y) = base do objeto, centrado em x; recorta na janela (nunca acima do vidro); opq: retangulo opaco (HMMM, x e largura pares) */
    i16 dx0 = x - (w >> 1), dx1, ty = y - h;
    if (opq) dx0 &= ~1;
    dx1 = dx0 + w;
    if (dx0 < clip_l) { sx += clip_l - dx0; dx0 = clip_l; }
    if (dx1 > clip_r) dx1 = clip_r;
    if (dx1 <= dx0) return;
    if (ty < SKY_TOP) { sy += SKY_TOP - ty; h -= SKY_TOP - ty; ty = SKY_TOP; }
    if (y > VIEW_BOT) h -= y - VIEW_BOT;
    if (h <= 0) return;
    vc_w(0, sx); vc_w(2, sy); vc_w(4, dx0); vc_w(6, ybase + ty);
    vc_w(8, (dx1 - dx0 + (opq ? 1 : 0)) & (opq ? ~1 : ~0)); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = opq ? 0xD0 : 0x98;   /* HMMM (opaco) ou LMMM com TIMP: cor 0 transparente */
    vdp_cmd15();
}
static void scn_pole(i16 x, i16 y, i16 h) {                                    /* poste da placa: cinza, da base ate a placa */
    i16 py = y - h, ph = h;
    if (x < clip_l || x >= clip_r) return;
    if (y > VIEW_BOT) ph -= y - VIEW_BOT;
    if (py < SKY_TOP) { ph -= SKY_TOP - py; py = SKY_TOP; }
    if (ph > 0) push_fill(x, py, 1 + (h >> 4), ph, 15);
}
/* retrovisor superior: vista traseira (mostra o que ficou para tras: a pista que foge para o horizonte, grama e as arvores/predios dos segmentos ja passados) */
static void rearview(i16 x0, i16 w, i16 wy, i16 h) {              /* espelho: retangulo (x0, wy) w x h (wy relativo a janela da estrada) */
    u8 j, n = 0, e, side, t; i16 x, ph, pw, yb, hz = (h * 10) / 21, cx = x0 + (w >> 1), dx, hh, ww;
    static const u8 rdx[4] = {26, 22, 18, 16}, rby[4] = {20, 15, 13, 12}, rh[4] = {8, 5, 3, 3}, rw[4] = {6, 4, 3, 2};
    push_fill(x0, wy, w, h, 0);                                              /* ceu */
    push_fill(x0, wy + hz, w, h - hz, 13);                                   /* grama */
    pw = (w * 17) / 61; push_fill(cx - (pw >> 1), wy + hz, pw, (h * 3) / 21 + 1, 0);                         /* pista (trapezio) */
    pw = (w * 27) / 61; push_fill(cx - (pw >> 1), wy + hz + (h * 3) / 21, pw, (h * 4) / 21 + 1, 0);
    pw = (w * 39) / 61; push_fill(cx - (pw >> 1), wy + hz + (h * 7) / 21, pw, h - hz - (h * 7) / 21, 0);
    if (pos & 1) push_fill(cx - 1, wy + hz + (h * 8) / 21, (w * 3) / 61 + 1, (h * 3) / 21, 7); else push_fill(cx, wy + hz + (h * 4) / 21, 1, (h * 2) / 21, 7);       /* tracejado */
    if (
#ifndef TEST_MIRROR
        !street_mode &&
#endif
        (pos > opos || (pos == opos && sub >= osub))) {            /* o oponente ficou para tras (ultrapassado): aparece no espelho, menor e mais perto do horizonte quanto mais longe */
        i16 d8 = (i16)(pos - opos) * 8 + (i16)sub - (i16)osub;                  /* distancia em 1/8 de segmento */
        if (d8 <= 112) {
            i16 cw = (w * 40) / (100 + 4 * d8) + 1, ch = (cw * 5) / 8 + 1, y2 = hz + ((h - 1 - hz) * 24) / (24 + (d8 >> 1)), half = (w * 39) / 122;
            i32 lat = (i32)(OLANE * 56) - P;
            x = cx + (i16)((lat * ((i32)half * (y2 - hz) / (h - hz))) / 900);
            push_fill(x - (cw >> 1), wy + y2 - ch, cw, ch, 12);                                    /* carro (cor 12: a pintura do oponente) */
            push_fill(x - (cw >> 2), wy + y2 - ch, (cw >> 1) + 1, (ch >> 1), 15);                 /* vidro traseiro (cinza claro: o carro pode ser preto) */
            push_fill(x - (cw >> 1), wy + y2 - 1, cw, 1, 7);                                       /* para-choque */
            push_fill(x - (cw >> 1), wy + y2 - 2, 1 + (cw >> 3), 1, 6); push_fill(x + (cw >> 1) - (cw >> 3) - 1, wy + y2 - 2, 1 + (cw >> 3), 1, 6);     /* lanternas */
        }
    }
    for (j = 0; j < 4 && n < 3; j++) {
        e = scn_tab[(u8)(pos - 1 - j)];
        for (side = 0; side < 2 && n < 3; side++) {
            t = (e >> (3 * side)) & 7;
            if (t != 1 && t != 6) continue;
            dx = (rdx[j] * w) / 61; ww = (rw[j] * w) / 61 + 1; hh = (rh[j] * h) / 21 + 1; yb = wy + (rby[j] * h) / 21;
            x = cx + (side ? dx : -dx) - (ww >> 1);
            push_fill(x, yb - hh, ww, hh, t == 1 ? 3 : 15);
            n++;
        }
    }
}
static void scn_side(u8 t, u8 side, u8 off, i16 xc, i16 y, i16 hw) {           /* um objeto do lado side (0 esquerda, 1 direita) na linha do chao y, largura da meia pista hw */
    i16 x, ph; u8 k;
#ifdef SCN_SKIP
    if ((SCN_SKIP >> t) & 1) return;                                   /* (teste de desempenho: pula tipos de objeto) */
#endif
    if (t == 5) {                                                      /* poste branco ao lado da pista */
        x = side ? xc + hw + (hw >> 2) : xc - hw - (hw >> 2);
        ph = (hw >> 4) + 2;
        if (y > VIEW_BOT) ph -= y - VIEW_BOT;
        if (ph > 0) push_fill(x, y - (hw >> 4) - 2, (hw >> 6) + 1, ph, 7);
        return;
    }
    if (t == 1) {                                                      /* arvore */
        x = off == 0 ? hw + (hw >> 2) + (hw >> 3) : off == 1 ? hw + (hw >> 1) + (hw >> 2) : (hw << 1) + (hw >> 2);
        k = hw >= 50 ? 3 : hw >= 34 ? 2 : hw >= 22 ? 1 : 0;
        while (k && tr_h[k] > y - SKY_TOP + 4) k--;
        scn_blit(tr_x[k], TREE_Y, side ? xc + x : xc - x, y, tr_w[k], tr_h[k], 0);
    } else if (t == 6) {                                               /* predio, mais afastado da pista */
        x = off == 0 ? 3 * hw + (hw >> 3) : off == 1 ? 3 * hw + (hw >> 1) + (hw >> 3) : (hw << 2) + (hw >> 3);
        k = hw >= 58 ? 3 : hw >= 38 ? 2 : hw >= 24 ? 1 : 0;
        while (k && bl_h[k] > y - SKY_TOP + 4) k--;
        scn_blit(bl_x[k], TREE_Y, side ? xc + x : xc - x, y, bl_w[k], bl_h[k], 1);
    } else {                                                           /* placas (num poste) */
        x = hw + (hw >> 1); if (side) x = xc + x; else x = xc - x;
        if (t == 2) { k = hw >= 110 ? 0 : hw >= 85 ? 1 : hw >= 60 ? 2 : hw >= 38 ? 3 : hw >= 22 ? 4 : 5; ph = sga_h[k] >> 1; scn_pole(x, y, ph + 2); scn_blit(sga_x[k], SIGA_Y, x, y - ph, sga_w[k], sga_h[k], 1); }
        else {
            k = hw >= 85 ? 0 : hw >= 60 ? 1 : hw >= 38 ? 2 : hw >= 22 ? 3 : 4;
            if (t == 3) { ph = sgb_h[k] >> 1; scn_pole(x, y, ph + 2); scn_blit(sgb_x[k], SIGB_Y, x, y - ph, sgb_w[k], sgb_h[k], 1); }
            else { ph = sgc_h[k] >> 1; scn_pole(x, y, ph + 2); scn_blit(sgc_x[k], SIGC_Y, x, y - ph, sgc_w[k], sgc_h[k], 1); }
        }
    }
}
/* portico "CITY LIMITS" (so na viagem de rua): barra vermelha entre dois postes, com a placa pendurada; fica acima do horizonte e sobe pelo para-brisa ao passar.
   Acima da janela da estrada (y < SKY_TOP) restaura-se o vidro da copia limpa (pagina 2) a cada quadro enquanto ele esta a vista, e depois repoe o retrovisor por cima. */
#define GNT_SEG 8
#define GNT_TOP (-4)
static u8 gnt_clr;
static void gnt_restore(i16 x, i16 w, i16 wy, i16 h) {
    vc_w(0, x); vc_w(2, 512 + WIN_Y + wy); vc_w(4, x); vc_w(6, ybase + wy); vc_w(8, w); vc_w(10, h); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
    vdp_cmd15();
}
static void gantry(void) {
    i16 r = GNT_SEG + 3 - (i16)pos, y, xc, hw, yt, bt, pw, xl, xr, fw, fh, fx, fy;
    u8 on = r >= 2 && r < N_ROWS;
    if (on) {                                                                      /* faixa amarela do limite da cidade: atravessa a pista na base do portico */
        y = Y[r - 1]; xc = XC[r - 1]; hw = 2 * (y - HZ) + ((y - HZ) >> 5);
        if (hw >= 10 && y < VIEW_BOT && y > SKY_TOP + 2) push_fill(xc - hw, y - (hw >> 4) - 1, 2 * hw, (hw >> 4) + 1, 8);
    }
    if (on) {
        y = Y[r - 1]; xc = XC[r - 1]; hw = 2 * (y - HZ) + ((y - HZ) >> 5);
        yt = y - ((hw * 3) >> 2);
        if (hw < 14 || yt < GNT_TOP || y >= VIEW_BOT) on = 0;
    }
    if (!on && !gnt_clr) return;
    gnt_restore(RACE_X0, 256 - RACE_X0, GNT_TOP, SKY_TOP - GNT_TOP);                 /* vidro limpo (os restos do quadro anterior nesta pagina) */
    if (on) {
        gnt_clr = 2;
        pw = (hw >> 5) + 1; bt = (hw >> 4) + 1; xl = xc - hw - (hw >> 2); xr = xc + hw + (hw >> 2);
        push_fill(xl, yt, pw, y - yt, 6); push_fill(xr, yt, pw, y - yt, 6);       /* postes */
        push_fill(xl, yt, xr - xl + pw, bt, 6);                                    /* barra */
        if (hw >= 50) { fw = 40; fh = 18; fx = 200; fy = TREE_Y; }
        else if (hw >= 26) { fw = 26; fh = 13; fx = 200; fy = TREE_Y + 19; }
        else { fw = 14; fh = 7; fx = 230; fy = TREE_Y + 19; }
        vc_w(0, fx); vc_w(2, fy); vc_w(4, (xc - (fw >> 1)) & ~1); vc_w(6, ybase + yt + bt + 1); vc_w(8, fw); vc_w(10, fh); vc[12] = 0; vc[13] = 0; vc[14] = 0xD0;
        vdp_cmd15();
        gnt_restore(192, 64, GNT_TOP, 24);                                         /* retrovisor por cima do portico */
    } else gnt_clr--;
}
static void draw_scn(void) BK {
    if (street_mode) gantry();
    u8 r, e, t;
    i16 y, hw;
    for (r = N_ROWS - 1; r >= 2; r--) {
        e = scn_tab[(u8)(pos + r - 3)];
        if (!(e & 63)) continue;
        y = Y[r - 1];
        if (y >= VIEW_BOT || y < SKY_TOP) continue;
        hw = 2 * (y - HZ) + ((y - HZ) >> 5);              /* = hwy_tab[y] (largura da meia pista na linha y), sem a tabela */
        if (hw < 12) continue;
        t = e & 7; if (t) scn_side(t, 0, e >> 6, XC[r - 1], y, hw);
        t = (e >> 3) & 7; if (t) scn_side(t, 1, e >> 6, XC[r - 1], y, hw);
    }
    rearview(194, 61, -2, 21);                                               /* retrovisor de cima */
    { i16 cl = clip_l; clip_l = 0; rearview(8, 24, 79, 21); clip_l = cl; }         /* retrovisor lateral esquerdo (interior x 8-31, y 103-123 da tela) */
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
    fm_detect();
#ifndef NO_SV_INIT
    sv_init();
    rk_init();
#endif
    mus_set(1);                                          /* estacao padrao: Rock 'n' Roll (em FM se houver MSX-MUSIC) */
    for (;;) {
        title_screen();
        new_game();
        while (!hub()) {                                /* hub: garagem; 0 = Bobs Drive-In */
            pal_load();
            race_go();
            race_result();
            settle_race();
        }
        if (game_end) end_screen();                    /* o jogo acabou: depois do aviso, volta ao titulo */
    }
}
typedef char sv_size_check[(SV_N <= 128) ? 1 : -1];
