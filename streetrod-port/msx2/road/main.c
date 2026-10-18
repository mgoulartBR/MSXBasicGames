/* Prototipo da estrada pseudo-3D de Street Rod para MSX2 (Screen 5, V9938).
 * Mede o custo de ~23 linhas de projeção por frame desenhadas com HMMV. */
#include <stdint.h>
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

typedef uint32_t u32; typedef int16_t i16; typedef uint16_t u16; typedef uint8_t u8; typedef int32_t i32;

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
volatile __at(0xFC9E) u16 JIFFY;

u8 cmd_buf[CMD_MAX * 11];
u16 cmd_n; u8 *cp; u16 g_ybase;
u8 g_trig, g_row6, g_st0, g_st1, g_t0, g_t1, g_t3;
u8 g_reg, g_val, g_stick, g_sh, g_sgn, g_fc;
u16 g_lzsrc, g_vlo, g_palp; u8 g_vhi, g_vpg, g_group;
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
#ifndef OPP_START
#define OPP_START 0            /* teste: posicao inicial do oponente (segmentos) */
#endif
/* powertrain dos 2 veiculos (0 = jogador, 1 = oponente), copiado das tabelas de carros (cars.h) ao iniciar a corrida */
static u16 pt_ratio[10], pt_hi0[10], pt_hi1[10], pt_acc[30], pt_redline[2];
static u8 pt_ngears[2], pt_auto[2], opp_a, opp_b, opp_set;
#define MAXCARS 4
static u16 money; static u8 day, nown, cur, own[MAXCARS];     /* estado do jogo */
#define MAXSPARE 12
static u8 spare[MAXSPARE], nspare;
static u8 m_sel;
static u8 offer_day = 255;
static u8 sel_car = DEF_CAR, sel_opp = DEF_OPP_CAR, sel_pil = DEF_OPP;
static void load_car(u8 slot, u8 id) {
    u8 i;
    for (i = 0; i < 5; i++) { pt_ratio[slot * 5 + i] = car_ratio[id * 5 + i]; pt_hi0[slot * 5 + i] = car_hi0[id * 5 + i]; pt_hi1[slot * 5 + i] = car_hi1[id * 5 + i]; }
    for (i = 0; i < 15; i++) pt_acc[slot * 15 + i] = car_acc[id * 15 + i];
    pt_ngears[slot] = car_ngears[id]; pt_redline[slot] = car_redline[id]; pt_auto[slot] = car_auto[id];
}
#define OPP_A opp_a
#define OPP_B opp_b
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

static void set_gear(u8 c, u8 ng) {    /* troca de marcha: a velocidade e mantida */
    u32 r;
    if (ng == vgear[c]) return;
    r = (u32)vspd[c] * pt_ratio[c * 5 + ng];
    if (ng && r > 5700) { if (ng > vgear[c]) { r = 5700; } else return; }   /* reducao com rotacao excessiva: recusada */
    vgear[c] = ng; vrpm[c] = (u16)r;
    if (c == 0) gear_changed = 1;
}

extern volatile u8 ev_space, ev_shift;
extern void install_hook(void);

static void combine_input(void) {
    u8 e;
    g_stick = g_st0 ? g_st0 : g_st1;
    g_trig = (g_t0 ? 1 : 0) | (g_t1 ? 2 : 0) | (g_t3 ? 4 : 0);
    e = ev_space; if (e) { ev_space = 0; g_trig |= 1; }       /* toque curto travado pela interrupcao */
    e = ev_shift; if (e) { ev_shift = 0; g_trig |= 4; }
}

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
    i16 gap = (i16)(opos - pos), cv = curve_tab[(opos + 3) & 255];
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
    P -= (i16)(((i32)curve_tab[pos & 255] * speed * dj) >> 9);   /* forca centrifuga */
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
static void vc_w(u8 i, i16 v) { vc[i] = (u8)v; vc[i + 1] = (u8)(v >> 8); }

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

static void show_title(void) {
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

static void wait_start(void) {            /* espera ESPACO / botao A (ignora tecla ja apertada no boot) */
    u8 held = 1, go;
    for (;;) {
        read_input(); combine_input();
        go = (g_trig & 3) != 0;
        if (go && !held) return;
        held = go;
    }
}

/* ---- menu de selecao: carro do jogador, carro e piloto do oponente ---- */
static const u8 paint_rb[8] = {0x50 | 5, 0x60, 0x10 | 6, 0x00 | 1, 0x70, 0x60 | 6, 0x70, 0x00 | 5};   /* R<<4|B da cor da pintura do oponente */
static const u8 paint_g[8]  = {0, 0, 2, 4, 6, 6, 3, 5};

static void menu_row(u8 f) {
    i16 y = 26 + f * 34;
    u8 sel = f == m_sel;
    push_fill(0, y, 256, 28, 0);
    draw_text(16, y, f == 0 ? "YOUR CAR" : f == 1 ? "OPPONENT CAR" : "OPPONENT DRIVER", sel ? 13 : 8);
    draw_text(16, y + 14, f == 0 ? car_name[sel_car] : f == 1 ? car_name[sel_opp] : pil_name[sel_pil], 7);
    if (sel) { push_fill(2, y, 2, 2, 13); push_fill(4, y + 2, 2, 2, 13); push_fill(6, y + 4, 2, 2, 13); push_fill(4, y + 6, 2, 2, 13); push_fill(2, y + 8, 2, 2, 13); }
}
static void menu_info(void) {
    push_fill(0, 134, 256, 28, 0);
    draw_text(16, 134, "GEARS", 8); draw_num(64, 134, car_ngears[sel_car], 1);
    draw_text(88, 134, "REDLINE", 8); draw_num(152, 134, car_redline[sel_car], 4);
    draw_text(16, 148, car_auto[sel_car] ? "AUTOMATIC" : "MANUAL SHIFT", 7);
}
static void menu_change(i16 d) {
    if (m_sel == 0) sel_car = (u8)((sel_car + NCARS + d) % NCARS);
    else if (m_sel == 1) sel_opp = (u8)((sel_opp + NCARS + d) % NCARS);
    else sel_pil = (u8)((sel_pil + NPILOTS + d) % NPILOTS);
    menu_row(m_sel); if (m_sel == 0) menu_info();
    vdp_wait();
}
static void menu(void) {
    u8 prev = 0, f, go, held = 1;
    u16 t_hold = 0, t_rep = 0;
    clip_l = 0; clip_r = 256; ybase = 0; vdp_reg(2, 0x1F); dbg_state = 8;
    cmd_n = 0; cp = cmd_buf;
    push_fill(0, 0, 256, 212, 0);
    draw_text(72, 6, "BOBS DRIVE-IN", 13);
    m_sel = 1;                              /* o carro do jogador vem da garagem: campo 0 so informativo */
    for (f = 0; f < 3; f++) menu_row(f);
    menu_info();
    draw_text(16, 172, "UP DOWN  SELECT FIELD", 6);
    draw_text(16, 186, "LEFT RIGHT  CHANGE", 6);
    draw_text(16, 200, "SPACE  START RACE", 13);
    sel_car = own[cur];
    vdp_wait();
    for (;;) {
        u8 st, act = 0; i16 d = 0;
        read_input(); combine_input();
        st = g_stick; go = (g_trig & 3) != 0;
        if (go && !held) break;
        held = go;
        if (st != prev) { act = 1; prev = st; t_hold = JIFFY; t_rep = JIFFY; }
        else if (st && (u16)(JIFFY - t_hold) > jhz / 3 && (u16)(JIFFY - t_rep) >= jhz / 8) { act = 1; t_rep = JIFFY; }
        if (!act) continue;
        if (st == 1 && m_sel > 1) { f = m_sel; m_sel--; menu_row(f); menu_row(m_sel); vdp_wait(); }
        else if (st == 5 && m_sel < 2) { f = m_sel; m_sel++; menu_row(f); menu_row(m_sel); vdp_wait(); }
        else if (st == 3) d = 1;
        else if (st == 7) d = -1;
        if (d) menu_change(d);
    }
}

/* ---- estado do jogo e hub (garagem) ---- */
#ifndef START_MONEY
#define START_MONEY 1000
#endif
#define HC_W 4          /* cores da paleta da garagem (indices EGA) */
#define HC_Y 15
#define HC_G 13
#define HC_R 11
static void new_game(void) {
    money = START_MONEY; day = 0; nown = 0; cur = 0; sel_car = 0; offer_day = 255; nspare = 0;
#ifdef START_CAR
    own[0] = START_CAR; nown = 1; sel_car = START_CAR;                  /* so para testes automatizados */
#endif
}

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
    draw_text(8, 202, nown ? car_name[own[cur]] : "NO CAR - BUY ONE", HC_G);
    vdp_wait();
}
/* menu vertical sobre a cena: devolve o item escolhido (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_X 8
#define UI_W 240
/* lista rolavel (ate UI_VIS linhas visiveis); o texto de cada item vem de ui_get(i). Devolve o indice (ESPACO) ou 255 (SHIFT = voltar) */
#define UI_VIS 8
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
    for (i = top; i < top + vis; i++) ui_row(i, y0, top, sel);
    vdp_wait();
    for (;;) {
        u8 st, go, back, ns = sel;
        read_input(); combine_input();
        st = g_stick; go = (g_trig & 3) != 0; back = (g_trig & 4) != 0 || !(g_row6 & 1);
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
static u16 rnd16(void) { static u16 s = 0x1D2B; s = s * 25173u + 13849u; s ^= JIFFY; return s; }

static void cars_screen(void) {
    const char *it[MAXCARS]; u8 i, r;
    if (!nown) { ui_note("YOUR CARS", "BUY A CAR FIRST!"); return; }
    for (i = 0; i < nown; i++) it[i] = car_name[own[i]];
    r = ui_pick("YOUR CARS - SPACE TO DRIVE", it, nown, cur);
    if (r != 255) { cur = r; sel_car = own[cur]; }
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
static const char *offer_get(u8 i) { return (offer_mask >> i) & 1 ? "(GONE)" : fmt_price(car_name[offer[i]], offer_price(i)); }
static u8 try_pay(u16 price) {            /* confere o dinheiro; mensagem se faltar */
    if (money < price) { ui_note("NO GO, HOT SHOT", fmt_short(price - money)); return 0; }
    money -= price; return 1;
}
static void buy_car(u8 i) {
    static const char *const it[3] = {"BUY AT ASKING PRICE", "MAKE AN OFFER", "FORGET IT"};
    static const char *const dis[3] = {"10% LESS", "20% LESS", "30% LESS"};
    u16 price = offer_price(i); u8 r;
    r = ui_pick(car_name[offer[i]], it, 3, 0);
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
    own[nown++] = offer[i]; offer_mask |= 1 << i;
    if (nown == 1) { cur = 0; sel_car = own[0]; }
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
static const char *part_get(u8 i) { return fmt_price(part_name[plist[i]], PART_PRICE(plist[i])); }
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
            ui_note("IT'S YOURS!", part_name[plist[r]]);
        }
    }
}
static const char *spare_get(u8 i) { return fmt_price(part_name[spare[i]], PART_PRICE(spare[i]) / 2); }
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
static u8 hub(void) {                       /* 0 = ir ao Bob's (corrida), 1 = sair para o titulo */
    static const char *const items[4] = {"YOUR CARS", "NEWSPAPER", "BOBS DRIVE-IN", "QUIT GAME"};
    u8 sel = 2, r;
    load_garage();
    ybase = 0; clip_l = 0; clip_r = 256; vdp_reg(2, 0x1F);
    cmd_n = 0; cp = cmd_buf; hub_status();
    for (;;) {
        r = ui_pick("MAIN MENU", items, 4, sel);
        if (r == 255) continue;
        sel = r;
        if (r == 0) cars_screen();
        else if (r == 1) newspaper();
        else if (r == 2) { if (nown) return 0; ui_note("BOBS DRIVE-IN", "BUY A CAR FIRST!"); }
        else return 1;
        cmd_n = 0; cp = cmd_buf; hub_status();
    }
}

static void new_race(void) {
    u8 k = (u8)(JIFFY & 7);
    load_car(0, sel_car); load_car(1, sel_opp); opp_a = pil_a[sel_pil]; opp_b = pil_b[sel_pil]; opp_set = car_spr[sel_opp];
    g_reg = 12; g_val = paint_rb[k]; g_sh = paint_g[k]; pal_one();                    /* pintura do oponente (cor 12 da paleta) */
    gear = 0; rpm = 1500; speed = 0; pos = 0; sub = 0; P = -300;
    vgear[1] = 0; vrpm[1] = 1500; vspd[1] = 0; opos = OPP_START; osub = 0;
    launched[0] = launched[1] = 0; adv_acc[0] = adv_acc[1] = 0;
    state = 0; cd_t = 0; result = 0; false_start = 0; race_ticks = 0; sum_spd = 0;
    beep_stage = 0; light_rest = 0;
    opp_wait = OPP_B * 3 + 6; gear_changed = 1; go_held = 1; up_held = dn_held = 1;   /* tecla ainda apertada nao conta como troca */
    dbg_t60 = dbg_t100 = dbg_top = 0;
}

static void run_race(void) {
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
            opp_ai();
            if (state == 1) { race_ticks++; sum_spd += speed; }
        }
        if (state == 1) {
            if (opos == pos && (P - (OLANE * 56)) < 300 && (P - (OLANE * 56)) > -300) {   /* colisao */
                vrpm[0] -= vrpm[0] >> 4; vrpm[1] -= vrpm[1] >> 4;
            }
            if (!dbg_t60 && speed >= 60) dbg_t60 = (u16)(JIFFY - start_j);
            if (!dbg_t100 && speed >= 100) dbg_t100 = (u16)(JIFFY - start_j);
            if (speed > dbg_top) dbg_top = speed;
            if (pos >= RACE_LEN || opos >= RACE_LEN) {
                u16 pp = pos * 8 + sub, op = opos * 8 + osub;
                result = (pos >= RACE_LEN && pp >= op) ? 1 : 2;
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
        draw_opp();
        draw_lights(state == 0 || (state == 1 && (u16)(JIFFY - start_j) < 2 * jhz));
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

static void settle_race(void) {              /* efeito economico provisorio da corrida (o jornal, apostas e danos entram nos marcos seguintes) */
    if (result == 1) money += 50; else money = money > 25 ? money - 25 : 0;
    if (day < 60) day++;
    dbg_money = money; dbg_day = day;
}

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
            menu();                                     /* escolha do oponente */
            run_race();
            settle_race();
        }
    }
}
