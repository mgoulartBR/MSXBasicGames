#include "msxgl.h"
#include "platform/pvideo.h"
#include "platform/ctrl.h"
#include "bgame.h"

void main()
{
	Vid_Init();
	Input_Init();
	rng_seed(0x1234);
	run_new();
	joker_add(JK_JOKER);
	joker_add(JK_GREEDY_JOKER);
	g.blind = BLIND_SMALL;
	blind_start();
	Vid_Clear(COL_FELT);
	for (u8 i = 0; i < g.nHand; i++) Vid_Card(C_CELL(g.deck[g.hand[i]]), 8 + i * 28, 150);
	u16 sel = 0x1F;
	ScoreOut o;
	round_play(sel, &o);
	Vid_Text(8, 8, "score test:", TC_WHITE);
	Vid_Text(8, 20, g_Hands[o.type].name, TC_GOLD);
	Vid_Num(8, 32, (i32)o.total, TC_RED);
	for (u8 i = 0; i < o.n && i < 10; i++)
	{
		Vid_Num(8 + i * 24, 50, o.ev[i].kind, TC_BLUE);
		Vid_Num(8 + i * 24, 60, o.ev[i].val, TC_WHITE);
		Vid_Num(8 + i * 24, 70, o.ev[i].chips, TC_GOLD);
		Vid_Num(8 + i * 24, 80, o.ev[i].mult, TC_RED);
	}
	for (u8 i = 0; i < g.nJk; i++) Vid_Card(g_Jokers[g.jk[i].id].cell, 150 + i * 28, 8);
	while (1)
	{
		Vid_Sync();
		Input_Update();
		Vid_Cursor(in.mx, in.my, in.mouse);
	}
}
