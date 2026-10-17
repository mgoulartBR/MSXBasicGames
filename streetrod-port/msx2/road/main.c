/* Prototipo da estrada pseudo-3D de Street Rod para MSX2 (Screen 5, V9938).
 * Mede o custo de ~23 linhas de projeção por frame desenhadas com HMMV. */
#include <stdint.h>
#include "tables.h"

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
#define VIEW_BOT 80     /* altura da janela da estrada */
#endif
#ifndef N_ROWS
#define N_ROWS 15       /* linhas projetadas (original: 14 distantes + proximas) */
#endif
#ifndef DRAW_MODE
#define DRAW_MODE 2     /* bit0: zebras separadas, bit1: gramado por scanline (cada pixel pintado 1x) */
#endif
#ifndef VIEW_X0
#define VIEW_X0 32
#endif
#ifndef VIEW_X1
#define VIEW_X1 224
#endif
#ifndef JIFFY_HZ
#define JIFFY_HZ 60
#endif
#define HZ 36
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
volatile __at(0xFC9E) u16 JIFFY;

u8 cmd_buf[CMD_MAX * 11];
u16 cmd_n; u8 *cp; u16 g_ybase;
u8 g_trig, g_row6;
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
#define GO_J (JIFFY_HZ * 3)     /* semaforo: 3 s ate o verde */
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
#define ADV_K ((u16)(8.09 * 256 / JIFFY_HZ + 0.5))
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

static void draw_opp(void) {
    i16 d = (i16)(opos - pos), y, xc, hw, w, h, x;
    u8 r;
    if (d < 2 || d >= N_ROWS - 1) return;
    r = (u8)d;
    y = Y[r - 1];
    if (y < 0 || y > 250) return;
    xc = XC[r - 1];
    hw = hwy_tab[y];
    x = xc + ((hw * OLANE) >> 4);
    w = hw >> 2; if (w < 4) w = 4;
    h = (w >> 1) + (w >> 2) + 1;
    push_fill_v(x - (w >> 1), y - h, w, h, 14);                      /* corpo */
    push_fill_v(x - (w >> 2), y - h - (h >> 1), w >> 1, (h >> 1) + 1, 15);   /* cabine */
    push_fill_v(x - (w >> 1) - 2, y - (h >> 1), 2, h >> 1, 0);       /* rodas */
    push_fill_v(x + (w >> 1), y - (h >> 1), 2, h >> 1, 0);
}

static void draw_car(void) {
    push_fill(100, 96, 56, 14, 9);
    push_fill(110, 88, 36, 8, 10);
    push_fill(96, 104, 12, 8, 0);
    push_fill(148, 104, 12, 8, 0);
}

static void draw_lights(void) {         /* semaforo: vermelho, vermelho, verde */
    u8 lit = cd_t < GO_J / 3 ? 0 : cd_t < 2 * GO_J / 3 ? 1 : cd_t < GO_J ? 2 : 3;
    push_fill(104, 4, 10, 10, lit >= 1 ? 6 : 11);
    push_fill(123, 4, 10, 10, lit >= 2 ? 6 : 11);
    push_fill(142, 4, 10, 10, lit >= 3 ? 13 : 11);
}

/* ---- HUD: digitos e letras 3x5 em escala 2x2 ---- */
static const u8 font[10][5] = {
    {7,5,5,5,7},{2,6,2,2,7},{7,1,7,4,7},{7,1,7,1,7},{5,5,7,1,1},
    {7,4,7,1,7},{7,4,7,5,7},{7,1,1,1,1},{7,5,7,5,7},{7,5,7,1,7}};
static const u8 letters[26][5] = {
    {2,5,7,5,5},{6,5,6,5,6},{3,4,4,4,3},{6,5,5,5,6},{7,4,6,4,7},{7,4,6,4,4},{3,4,5,5,3},{5,5,7,5,5},{7,2,2,2,7},
    {1,1,1,5,2},{5,5,6,5,5},{4,4,4,4,7},{5,7,7,5,5},{6,5,5,5,5},{2,5,5,5,2},{6,5,6,4,4},{2,5,5,6,3},{6,5,6,5,5},
    {3,4,2,1,6},{7,2,2,2,2},{5,5,5,5,7},{5,5,5,5,2},{5,5,7,7,5},{5,5,2,5,5},{5,5,2,2,2},{7,1,2,4,7}};
static void draw_glyph(i16 x, i16 y, const u8 *g, u8 c) {
    u8 row, col;
    push_fill(x, y, 8, 12, 0);
    for (row = 0; row < 5; row++)
        for (col = 0; col < 3; col++)
            if (g[row] & (4 >> col)) push_fill(x + col * 2, y + row * 2, 2, 2, c);
}
static void draw_digit(i16 x, i16 y, u8 d, u8 c) { draw_glyph(x, y, font[d], c); }
static void draw_num(i16 x, i16 y, u16 v, u8 digits) {
    u8 i;
    for (i = 0; i < digits; i++) { draw_digit(x + (digits - 1 - i) * 8, y, v % 10, 8); v /= 10; }
}
static void draw_text(i16 x, i16 y, const char *s, u8 c) {
    for (; *s; s++, x += 8) {
        if (*s >= 'A' && *s <= 'Z') draw_glyph(x, y, letters[*s - 'A'], c);
        else if (*s == '.') push_fill(x + 2, y + 8, 2, 2, c);
    }
}

static void draw_gear(void) {
    u8 pg;
    clip_l = 0; clip_r = 256;
    for (pg = 0; pg < 2; pg++) {
        ybase = pg ? 256 : 0;
        draw_digit(224, 120, gear, 13);
        vdp_wait();
    }
    clip_l = VIEW_X0; clip_r = VIEW_X1;
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

static void draw_t10(i16 x, i16 y, u16 t10, u8 c) {
    draw_num(x, y, t10 / 10, 3);
    push_fill(x + 24, y + 8, 2, 2, c);
    draw_digit(x + 28, y, t10 % 10, 8);
}

static void show_result(void) {               /* tela de resultado, nas duas paginas */
    u8 pg;
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

static void new_race(void) {
    gear = 0; rpm = 1500; speed = 0; pos = 0; sub = 0; P = -300;
    vgear[1] = 0; vrpm[1] = 1500; vspd[1] = 0; opos = OPP_START; osub = 0;
    launched[0] = launched[1] = 0; adv_acc[0] = adv_acc[1] = 0;
    state = 0; cd_t = 0; result = 0; false_start = 0; race_ticks = 0; sum_spd = 0;
    opp_wait = OPP_B * 3 + 6; gear_changed = 1; go_held = 1;
    dbg_t60 = dbg_t100 = dbg_top = 0;
}

void main(void) {
    u16 last_j, t, dj, j0, fr0, tickacc = 0, nt, start_j;
    u8 shown = 0;
    dbg_frames = 0; dbg_t60 = 0; dbg_t100 = 0; dbg_top = 0;
    clip_l = VIEW_X0; clip_r = VIEW_X1;
#ifdef DRY
    g_dry = 1;
#endif
    set_screen5();
    vdp_reg(8, *(volatile u8 *)0xFFE7 | 0x02);          /* sprites off: comandos do VDP mais rapidos */
    pal_load();
    new_race();
    init_static();
    ybase = 256;
    vdp_reg(2, 0x1F);
    last_j = JIFFY; j0 = last_j; fr0 = 0; start_j = last_j;
    for (;;) {
        t = JIFFY; dj = t - last_j; last_j = t;
        if (dj > 12) dj = 12;
        read_input();
        if (state == 2) {                                   /* resultado: espera ESPACO / botao A */
            u8 go = (g_trig & 3) != 0;
            if (go && !go_held) { new_race(); init_static(); tickacc = 0; last_j = JIFFY; start_j = last_j; ybase = 256; vdp_reg(2, 0x1F); continue; }
            go_held = go;
            continue;
        }
        handle_input();
        if (state == 0) {
            cd_t += dj;
            if (false_start) { result = 3; state = 2; show_result(); continue; }
            if (cd_t >= GO_J) { state = 1; launched[0] = 1; start_j = JIFFY; }
        } else {
            if (opp_wait > dj) opp_wait -= dj; else { opp_wait = 0; launched[1] = 1; }
        }
        /* fisica a 6,0688 Hz: acumula 6,0688/JIFFY_HZ ticks por jiffy (8.8) */
        tickacc += dj * (u16)(6.0688 * 256 / JIFFY_HZ + 0.5);
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
        push_fill(0, 0, 256, Y[N_ROWS - 1] < 0 ? 0 : Y[N_ROWS - 1], 1);
        draw_road_asm();
        draw_opp();
        if (state == 0) draw_lights();
        draw_car();
        clip_l = 0; clip_r = 256;
        push_fill(8, 152, 200, 10, 0);
        push_fill(8, 152, speed + (speed >> 1), 10, 13);
        push_fill(8, 168, 200, 8, 0);
        push_fill(8, 168, rpm / 30, 8, rpm > 4700 ? 6 : 8);
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
        if (gear_changed) { gear_changed = 0; { u16 sv = ybase; draw_gear(); ybase = sv; } }
        if ((u16)(JIFFY - j0) >= 60) {
            dbg_fps10 = (u16)(((u16)(dbg_frames - fr0) * 600u) / (u16)(JIFFY - j0));
            fr0 = dbg_frames; j0 = JIFFY;
            draw_hud();
            ybase = shown ? 0 : 256;
        }
    }
}
