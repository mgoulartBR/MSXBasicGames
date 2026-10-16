#include "game.h"
#include "gfx.h"
#include "text.h"
#include "gen/blob.h"
#include "../src/sp_logic.h"
#include "../src/tricks.h"

#define JIFFY   (*(volatile u16*)0xFC9E)         // BIOS 60 Hz (50 Hz on PAL) tick counter

Keys g_Held, g_Press;
u8 g_TrickDone[3];

static SP_Table g_T;
static u8 s_Angle;

//-----------------------------------------------------------------------------
// Input: joystick 1 and keyboard (arrows, SPACE = fire, ESC = back)
void Input_Poll(void)
{
	Keys prev = g_Held;
	u8 joy = Joystick_Read(JOY_PORT_1);
	u8 k8 = Keyboard_Read(8), k7 = Keyboard_Read(7);

	g_Held.up    = !(joy & JOY_INPUT_DIR_UP)    || !(k8 & (1 << 5));
	g_Held.down  = !(joy & JOY_INPUT_DIR_DOWN)  || !(k8 & (1 << 6));
	g_Held.left  = !(joy & JOY_INPUT_DIR_LEFT)  || !(k8 & (1 << 4));
	g_Held.right = !(joy & JOY_INPUT_DIR_RIGHT) || !(k8 & (1 << 7));
	g_Held.fire  = !(joy & JOY_INPUT_TRIGGER_A) || !(k8 & 1) || !(k7 & (1 << 7));    // SPACE or RETURN
	g_Held.back  = !(joy & JOY_INPUT_TRIGGER_B) || !(k7 & (1 << 2));                 // ESC
	g_Press.up    = g_Held.up    && !prev.up;
	g_Press.down  = g_Held.down  && !prev.down;
	g_Press.left  = g_Held.left  && !prev.left;
	g_Press.right = g_Held.right && !prev.right;
	g_Press.fire  = g_Held.fire  && !prev.fire;
	g_Press.back  = g_Held.back  && !prev.back;
}

void Frame(void)
{
	Halt();
	Gfx_Flush();                                 // the vblank has just started: run the plan made last frame
	Input_Poll();
}

void WaitFrames(u8 n) { while (n--) Frame(); }

//-----------------------------------------------------------------------------
// Messages over the table
static u8 CenterX(const c8* s, u8 charW) { return (u8)((256 - Text_Width(s) / 6 * charW) / 2) & 0xFE; }

static void Banner(const c8* big, const c8* small)
{
	Gfx_Flush();
	Gfx_Fill(30, 80, 196, 54, COL_BLACK);
	Gfx_Outline(30, 80, 196, 54, COL_GOLD);
	Text_DrawBig(CenterX(big, 12), 90, big, COL_WHITE, COL_BLACK);
	if (small) Text_Draw(CenterX(small, 6), 116, small, COL_CREAM, COL_BLACK);
	do Frame(); while (!g_Press.fire);
	Gfx_RestoreTable();                          // balls are redrawn by the caller
	g_T.moved = 0x3FF;
	Gfx_Balls(&g_T);
}

//-----------------------------------------------------------------------------
// One shot: aim, charge, roll.  Returns 1 if the player backed out.
static u8 Shoot(void)
{
	u8 power = 0, lit = 0, charging = 0, held = 0, steps, moving;
	u16 last;

	Gfx_Aim(&g_T, s_Angle, TRUE);
	for (;;)
	{
		Frame();
		if (g_Press.back) { Gfx_Aim(&g_T, s_Angle, FALSE); return 1; }
		if (g_Held.left || g_Held.right)
		{
			s_Angle += (g_Held.right ? 1 : -1) * ((held > 20) ? 2 : 1);
			if (held < 255) held++;
			Gfx_Aim(&g_T, s_Angle, TRUE);
		}
		else held = 0;

		if (g_Held.fire)
		{
			if (power < 252) power += 3;
			charging = 1;
			if ((u8)(((u16)power * 7) >> 8) != lit) { lit = (u8)(((u16)power * 7) >> 8); Hud_Power(power); }
		}
		else if (charging) break;
	}
	Gfx_Aim(&g_T, s_Angle, FALSE);
	Hud_Power(0);
	SP_Shoot(&g_T, s_Angle, power < 24 ? 24 : power);

	last = JIFFY;
	do
	{
		Frame();
		steps = (u8)(JIFFY - last); last = JIFFY;       // one step per elapsed 1/60 s, so a slow frame never slows the game
		if (steps == 0) steps = 1;
		if (steps > 3) steps = 3;
		moving = 1;
		while (steps-- && moving) moving = SP_Step(&g_T);
		Gfx_Balls(&g_T);
	} while (moving);
	Frame();                                           // flush the last draws
	return 0;
}

// Ball in hand: move the cue ball with the arrows, FIRE to put it down.
static void PlaceCueByHand(void)
{
	u8 x = SP_CUE_X, y = SP_CUE_Y, i, ok;
	Hud_Label("PLACE", "CUE");
	SP_Place(&g_T, 0, x, y); Gfx_Balls(&g_T);
	for (;;)
	{
		Frame();
		if (g_Held.left  && x > 20)  x -= 2;
		if (g_Held.right && x < 235) x += 2;
		if (g_Held.up    && y > 31)  y -= 2;
		if (g_Held.down  && y < 137) y += 2;
		if (g_Held.left || g_Held.right || g_Held.up || g_Held.down) { SP_Place(&g_T, 0, x, y); Gfx_Balls(&g_T); }
		if (g_Press.fire)
		{
			ok = 1;
			for (i = 1; i < SP_NUM_BALLS; i++)
				if (SP_ACTIVE(&g_T, i) && (u8)(g_T.px8[i] - x + 9) <= 18 && (u8)(g_T.py8[i] - y + 9) <= 18) ok = 0;
			if (ok) return;
		}
	}
}

//-----------------------------------------------------------------------------
// Helpers
static u8 Lowest(void)                           // lowest numbered object ball on the table, 0 = none
{
	u8 b;
	for (b = 1; b < SP_NUM_BALLS; b++) if (SP_ACTIVE(&g_T, b)) return b;
	return 0;
}

static u16 PocketedMask(void) { return (u16)g_T.pocketed_mask_lo | ((u16)g_T.pocketed_mask_hi << 8); }

static u8 CountBits(u16 m)
{
	u8 n = 0;
	m &= ~1u;                                    // ignore the cue ball
	while (m) { n += m & 1; m >>= 1; }
	return n;
}

static void ReturnCue(void) { SP_PlaceCue(&g_T); Gfx_Balls(&g_T); }

static void HudBase(void) { Hud_Score(0); Hud_Shots(0); Hud_Next(0); Hud_Power(0); Hud_Label(0, 0); }

//-----------------------------------------------------------------------------
// Pocket Game: pocket balls for points; a shot that pockets nothing (or scratches) costs one of 8 shots.
// 100 points a ball (squared for multi-ball shots) and +500 for each ball sunk in numerical sequence.
// 1P: reach the stage's target score.  2P: alternate shots, a set ends when the table is cleared, best of 5 sets.
static const u16 s_Targets[8] = { 3000, 4500, 6000, 8000, 10000, 12500, 15000, 18000 };
static u16 TargetOf(u8 stage) { return stage <= 8 ? s_Targets[stage - 1] : 18000u + (stage - 8) * 3000u; }

static u16 ShotPoints(u16 pm, u8 lowest, u8* streak)
{
	u8 n = CountBits(pm), b = lowest;
	u16 pts = (u16)n * n * 100;
	while (b && b < SP_NUM_BALLS && ((pm >> b) & 1))            // balls sunk in numerical sequence
	{
		(*streak)++;
		if (*streak >= 2) pts += 500;
		b++;
	}
	if (b == lowest) *streak = 0;                                // the lowest ball was not sunk: sequence broken
	return pts;
}

static void NewPocketRack(u8 stage)
{
	if (stage == 1) SP_Init(&g_T); else SP_RackScatter(&g_T, stage * 7);
	s_Angle = 128;
}

static void Pocket1(void)
{
	u16 score = 0;
	u8 stage = 1, shots = 8, streak = 0, quit = 0;

	while (!quit)
	{
		u16 target = TargetOf(stage);
		Screen_StageIntro(stage, target);
		Gfx_Show(FALSE); Gfx_LoadGame(); HudBase();
		NewPocketRack(stage);
		Gfx_Balls(&g_T);
		Hud_Score(score); Hud_Shots(shots); Hud_Next(Lowest()); Hud_Label("STAGE", 0);
		Text_DrawNum(80, 18, stage, 2, COL_CREAM, COL_BLACK);
		Gfx_Show(TRUE);

		for (;;)
		{
			u8 lowest = Lowest(), n;
			u16 pm, pts;
			if (Shoot()) { quit = 1; break; }
			pm = PocketedMask(); n = CountBits(pm);
			if (g_T.scratch) { ReturnCue(); streak = 0; }
			if (n && !g_T.scratch)
			{
				pts = ShotPoints(pm, lowest, &streak);
				score += pts;
			}
			else { if (shots) shots--; streak = 0; }
			Hud_Score(score); Hud_Shots(shots); Hud_Next(Lowest());

			if (score >= target)
			{
				Banner("STAGE CLEAR", "PRESS FIRE");
				stage++; shots = shots + 3 > 8 ? 8 : shots + 3;
				break;
			}
			if (!shots)
			{
				Banner("GAME OVER", "PRESS FIRE");
				quit = 1; break;
			}
			if (!Lowest())                                       // table cleared before the target: rack again
			{
				NewPocketRack(stage + 1);
				g_T.moved = 0x3FF; Gfx_RestoreTable(); Gfx_Balls(&g_T);
				Hud_Next(Lowest());
			}
		}
	}
}

static void Pocket2(void)
{
	u16 score[2];
	u8 cur = 0, sets[2], setNo = 0, streak = 0, quit = 0;
	c8 lab[3];

	sets[0] = sets[1] = 0;
	Gfx_Show(FALSE); Gfx_LoadGame(); HudBase();
	while (!quit && sets[0] < 3 && sets[1] < 3)
	{
		score[0] = score[1] = 0; setNo++;
		NewPocketRack(setNo == 1 ? 1 : setNo + 3);
		g_T.moved = 0x3FF; Gfx_RestoreTable(); Gfx_Balls(&g_T);
		for (;;)
		{
			u8 lowest = Lowest(), n;
			u16 pm;
			lab[0] = 'P'; lab[1] = '1' + cur; lab[2] = 0;
			Hud_Label(lab, "SET"); Text_DrawNum(80, 18, setNo, 1, COL_CREAM, COL_BLACK);
			Hud_Score(score[cur]); Hud_Shots(sets[cur]); Hud_Next(lowest);
			Gfx_Show(TRUE);
			if (Shoot()) { quit = 1; break; }
			pm = PocketedMask(); n = CountBits(pm);
			if (g_T.scratch) ReturnCue();
			if (n && !g_T.scratch) score[cur] += ShotPoints(pm, lowest, &streak);
			else { streak = 0; cur ^= 1; }                       // no ball: the other player
			if (!Lowest())
			{
				u8 w = score[0] >= score[1] ? 0 : 1;
				sets[w]++;
				Banner(w ? "P2 WINS SET" : "P1 WINS SET", "PRESS FIRE");
				cur = w ^ 1;
				break;
			}
		}
	}
	if (!quit) Banner(sets[0] > sets[1] ? "P1 WINS" : "P2 WINS", "MATCH OVER");
}

void Game_Pocket(u8 players) { if (players == 1) Pocket1(); else Pocket2(); }

//-----------------------------------------------------------------------------
// Nine ball: hit the lowest ball first, pocket the 9 on a legal shot to win the rack.  A foul gives the
// opponent the cue ball in hand.  First to raceTo racks wins.
void Game_Nine(u8 raceTo)
{
	static const u8 order[9] = { 1, 2, 3, 4, 9, 5, 6, 7, 8 };      // 1 at the apex, 9 in the middle
	u8 racks[2], cur = 0, quit = 0, rackNo = 0;
	c8 lab[7];

	racks[0] = racks[1] = 0;
	Gfx_Show(FALSE); Gfx_LoadGame(); HudBase();
	while (!quit && racks[0] < raceTo && racks[1] < raceTo)
	{
		u8 inHand = 0;
		rackNo++;
		SP_Rack(&g_T, order); s_Angle = 128;
		g_T.moved = 0x3FF; Gfx_RestoreTable(); Gfx_Balls(&g_T);
		for (;;)
		{
			u8 lowest = Lowest(), foul, nine, any;
			u16 pm;
			lab[0] = 'P'; lab[1] = '1' + cur; lab[2] = ' '; lab[3] = '0' + racks[0]; lab[4] = '-'; lab[5] = '0' + racks[1]; lab[6] = 0;
			if (raceTo > 9) { lab[3] = '0' + racks[0] / 10; lab[4] = '0' + racks[0] % 10; lab[5] = 0; }
			Hud_Label(lab, "RACE");
			Text_DrawNum(80, 18, raceTo, 2, COL_CREAM, COL_BLACK);
			Hud_Score(racks[cur]); Hud_Shots(0); Hud_Next(lowest);
			Gfx_Show(TRUE);
			if (inHand) { PlaceCueByHand(); inHand = 0; }
			if (Shoot()) { quit = 1; break; }
			pm = PocketedMask();
			nine = (pm >> 9) & 1; any = CountBits(pm) != 0;
			foul = g_T.scratch || g_T.first_hit == 0xFF || g_T.first_hit != lowest;
			if (g_T.scratch) { SP_Remove(&g_T, 0); }
			if (nine && !foul)
			{
				racks[cur]++;
				Banner(cur ? "P2 WINS RACK" : "P1 WINS RACK", "PRESS FIRE");
				cur ^= 1;                                       // the loser breaks next
				break;
			}
			if (nine) { SP_Place(&g_T, 9, 82, SP_CUE_Y); }      // 9 pocketed on a foul: spot it again
			if (foul) { cur ^= 1; inHand = 1; if (!SP_ACTIVE(&g_T, 0)) SP_PlaceCue(&g_T); Hud_Label("FOUL", 0); WaitFrames(60); }
			else if (!any || nine) { cur ^= 1; }
			Gfx_Balls(&g_T);
		}
	}
	if (!quit) Banner(racks[0] > racks[1] ? "P1 WINS" : "P2 WINS", "MATCH OVER");
}

//-----------------------------------------------------------------------------
// Trick shots: each layout is solvable (verified with the game's physics); 3 attempts per trick.
static void LoadTrick(const Trick* t)
{
	u8 i;
	SP_Clear(&g_T);
	SP_Place(&g_T, 0, t->cx, t->cy);
	for (i = 0; i < t->n; i++) SP_Place(&g_T, i + 1, t->x[i], t->y[i]);
	g_T.moved = 0x3FF;
	Gfx_RestoreTable(); Gfx_Balls(&g_T);
	s_Angle = 128;
}

u8 Game_Trick(u8 index)
{
	const Trick* t = &g_Tricks[index];
	u8 tries = TRICK_TRIES, solved = 0, quit = 0;
	c8 lab[3];

	Gfx_Show(FALSE); Gfx_LoadGame(); HudBase();
	lab[0] = '0' + (index + 1) / 10; lab[1] = '0' + (index + 1) % 10; lab[2] = 0;
	while (tries && !solved && !quit)
	{
		LoadTrick(t);
		Hud_Label("TRICK", lab); Hud_Score(0); Hud_Shots(tries); Hud_Next(0);
		Gfx_Show(TRUE);
		if (Shoot()) { quit = 1; break; }
		if (!g_T.scratch && ((PocketedMask() >> 1) & t->targets) == t->targets) solved = 1;
		else tries--;
	}
	if (solved) { g_TrickDone[index >> 3] |= (u8)(1 << (index & 7)); Hud_Shots(tries); Banner("SOLVED!", "PRESS FIRE"); }
	else if (!quit) Banner("TRY AGAIN", "PRESS FIRE");
	return solved;
}
