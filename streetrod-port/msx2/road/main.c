/* Prototipo da estrada pseudo-3D de Street Rod para MSX2 (Screen 5, V9938).
 * Mede o custo de ~23 linhas de projeção por frame desenhadas com HMMV. */
#include <stdint.h>
#include "tables.h"

#ifndef STEP_SH
#define STEP_SH 1       /* log2 de scanlines por grupo de retangulos da pista */
#endif
#define STEP (1 << STEP_SH)
#ifndef VSYNC
#define VSYNC 1         /* 1 = troca de pagina sincronizada com o vblank */
#endif
#ifndef KERB_MERGE
#define KERB_MERGE 1    /* 1 = zebra desenhada como um retangulo largo sob a pista */
#endif
#ifndef VIEW_BOT
#define VIEW_BOT 112    /* altura da janela da estrada */
#endif
#ifndef N_ROWS
#define N_ROWS 15       /* linhas projetadas (original: 14 distantes + proximas) */
#endif
#ifndef DRAW_MODE
#define DRAW_MODE 0     /* bit0: zebras separadas, bit1: gramado por scanline (cada pixel pintado 1x) */
#endif
#ifndef VIEW_X0
#define VIEW_X0 0
#endif
#ifndef VIEW_X1
#define VIEW_X1 256
#endif
#ifndef JIFFY_HZ
#define JIFFY_HZ 60
#endif
#define HZ 36
#define CMD_MAX 700

typedef int16_t i16; typedef uint16_t u16; typedef uint8_t u8; typedef int32_t i32;

/* variaveis lidas pelo script de teste (enderecos fixos) */
volatile __at(0xF000) u16 dbg_frames;
volatile __at(0xF002) u16 dbg_cmds;
volatile __at(0xF004) u16 dbg_fps10;
volatile __at(0xF006) u16 dbg_speed;
volatile __at(0xF008) u16 dbg_pos;
volatile __at(0xF00A) u16 dbg_t60;      /* jiffies desde o inicio ate 60 mph */
volatile __at(0xF00C) u16 dbg_t100;
volatile __at(0xF00E) u16 dbg_top;
volatile __at(0xFC9E) u16 JIFFY;

u8 cmd_buf[CMD_MAX * 11];
u16 cmd_n; u8 *cp; u16 g_ybase;
u8 g_reg, g_val, g_stick, g_sh, g_sgn, g_fc;
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
extern void read_stick(void);

#define ybase g_ybase              /* 0 ou 256: pagina de desenho */

static void vdp_reg(u8 r, u8 v) { g_reg = r; g_val = v; vdp_reg_g(); }

static void push_fill(i16 x, i16 y, i16 w, i16 h, u8 c) {
    g_fx = x; g_fy = y; g_fw = w; g_fh = h; g_fc = c; fill_g();
}

/* ---- estado do jogo ---- */
u16 pos;  u8 sub;
i16 P;                      /* lateral da camera (0 = centro da pista) */
static u16 rpm; static u8 gear; static u16 speed;
static u8 tick_frac;

i16 XC[N_ROWS], Y[N_ROWS];
u8 PAR[N_ROWS]; u8 g_b8;
i16 xoff_ahead;
extern void project_asm(void);

static u8 step_of(u16 v) {
    u8 i = 0;
    while (i < 15 && v > step_thr[i]) i++;
    return i + 1;
}

/* um tick de fisica: o original roda a 72,826/12 = 6,0688 Hz (tempo exibido = ticks / 6,0688) */
#define PHYS_HZ_X256 ((u16)(6.0688 * 256 + 0.5))
static void physics_tick(void) {
    u16 hi, ac;
    u8 s;
    i16 steer, target;
    hi = pt_hi0[gear];
    s = 0;
    if (rpm >= hi) { s = 1; if (rpm >= pt_hi1[gear]) s = 2; }
    ac = pt_acc[gear * 3 + s];
    rpm += ac;
    if (rpm > 5300) rpm = 5300;
    speed = rpm / pt_ratio[gear];
    if (gear < PT_NGEARS) {
        u16 lim = gear ? (u16)((PT_REDLINE < 5300 ? PT_REDLINE : 5300) / 100 * 97) : 2900;
        if (rpm >= lim) { gear++; rpm = speed * pt_ratio[gear]; }
    }
    /* piloto automatico: segue o centro da pista alguns segmentos a frente */
    target = xoff_ahead >> 1;
    steer = (target - P) >> 3;
    if (steer > 30) steer = 30; if (steer < -30) steer = -30;
    P += steer;
    P -= ((i16)curve_tab[pos & 255] * (i16)(speed >> 2)) >> 4;   /* forca centrifuga */
    if (P > 1400) P = 1400; if (P < -1400) P = -1400;
}

/* avanco na pista por frame: o original anda step(speed) subunidades por frame a ~8,09 fps (limite do jogo) */
static void advance(u16 dj) {
    static u16 adv_acc;
    u8 st = step_of(speed);
    adv_acc += st * dj * ((u16)(8.09 * 256 / JIFFY_HZ + 0.5));
    while (adv_acc >= 256) { adv_acc -= 256; sub++; if (sub >= 8) { sub = 0; pos++; } }
}

static void project(void) { project_asm(); }

static void draw_strip(u8 r) {          /* faixa entre a linha r (distante) e r-1 (proxima) */
    i16 yf = Y[r], yn = Y[r - 1], yt, yb, dy, sl;
    u8 p = PAR[r - 1];
    if (yn <= yf) return;
    yt = yf < 0 ? 0 : yf;
    yb = yn > VIEW_BOT ? VIEW_BOT : yn;
    if (yb <= yt) return;
    s_rc = p ? 5 : 4; s_kc = p ? 6 : 7; s_gc = p ? 3 : 2; s_mode = DRAW_MODE;
    if (!(DRAW_MODE & 2)) push_fill(0, yt, 256, yb - yt, s_gc);
    dy = yn - yf;
    g_sh = 12; g_b = rdy_tab[dy];
    g_a = XC[r - 1] - XC[r]; mul12(); sl = g_r;
    s_xlf = XC[r] << 4;
    if (yt != yf) s_xlf += sl * (yt - yf);
    s_sl2 = sl << STEP_SH;
    s_yt = yt; s_rem = yb - yt;
    strip_run();
}

static void draw_car(void) {
    push_fill(100, 96, 56, 14, 9);
    push_fill(110, 88, 36, 8, 10);
    push_fill(96, 104, 12, 8, 0);
    push_fill(148, 104, 12, 8, 0);
}

/* ---- HUD: digitos 3x5 em escala 2x2 ---- */
static const u8 font[10][5] = {
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
static void draw_digit(i16 x, i16 y, u8 d, u8 c) {
    u8 row, col;
    push_fill(x, y, 8, 12, 0);
    for (row = 0; row < 5; row++)
        for (col = 0; col < 3; col++)
            if (font[d][row] & (4 >> col)) push_fill(x + col * 2, y + row * 2, 2, 2, c);
}
static void draw_num(i16 x, i16 y, u16 v, u8 digits) {
    u8 i;
    for (i = 0; i < digits; i++) { draw_digit(x + (digits - 1 - i) * 8, y, v % 10, 8); v /= 10; }
}

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

static void init_static(void) {
    u8 pg;
    clip_l = 0; clip_r = 256;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        cmd_n = 0; cp = cmd_buf;
        push_fill(0, 0, 256, 212, 0);
        push_fill(0, VIEW_BOT, 256, 212 - VIEW_BOT, 11);
        push_fill(6, 150, 204, 14, 0);
        vdp_wait();
    }
    clip_l = VIEW_X0; clip_r = VIEW_X1;
}

void main(void) {
    u16 last_j, t, dj, j0, fr0, tickacc = 0, nt, start_j;
    u8 shown = 0, k;
    dbg_frames = 0; dbg_t60 = 0; dbg_t100 = 0; dbg_top = 0;
    clip_l = VIEW_X0; clip_r = VIEW_X1;
#ifdef DRY
    g_dry = 1;
#endif
    set_screen5();
    vdp_reg(8, *(volatile u8 *)0xFFE7 | 0x02);          /* sprites off: comandos do VDP mais rapidos */
    pal_load();
    gear = 0; rpm = 1500; pos = 0; sub = 0; P = 0;
    init_static();
    ybase = 256;
    vdp_reg(2, 0x1F);
    last_j = JIFFY; j0 = last_j; fr0 = 0; start_j = last_j;
    for (;;) {
        t = JIFFY; dj = t - last_j; last_j = t;
        if (dj > 12) dj = 12;
#ifndef NOSTICK
        read_stick();
#endif
        /* fisica a 6,0688 Hz: acumula 6,0688/JIFFY_HZ ticks por jiffy (8.8) */
        tickacc += dj * (u16)(6.0688 * 256 / JIFFY_HZ + 0.5);
        nt = tickacc >> 8; tickacc &= 255;
        advance(dj);
#ifndef NOPHYS
        while (nt--) physics_tick();
#endif
        if (!dbg_t60 && speed >= 60) dbg_t60 = (u16)(JIFFY - start_j);
        if (!dbg_t100 && speed >= 100) dbg_t100 = (u16)(JIFFY - start_j);
        if (speed > dbg_top) dbg_top = speed;
        cmd_n = 0; cp = cmd_buf;
#ifndef NOPROJ
        project();
#endif
        /* ceu: de y=0 ate o topo da faixa mais distante */
        push_fill(0, 0, 256, Y[N_ROWS - 1] < 0 ? 0 : Y[N_ROWS - 1], 1);
#ifndef NODRAW
        draw_road_asm();
#endif
        draw_car();
        clip_l = 0; clip_r = 256;
        push_fill(8, 152, 200, 10, 0);
        push_fill(8, 152, speed * 2, 10, 13);
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
        if ((u16)(JIFFY - j0) >= 60) {
            dbg_fps10 = (u16)(((u16)(dbg_frames - fr0) * 600u) / (u16)(JIFFY - j0));
            fr0 = dbg_frames; j0 = JIFFY;
            draw_hud();
            ybase = shown ? 0 : 256;
        }
    }
}
