// Teste de unidade (PC): codigo de salvar/carregar, noticias, credito e venda. Usa a mesma src/sim.c do jogo.
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "sim.h"

static int fails;
#define CHECK(c, msg) do { if (!(c)) { printf("FAIL: %s\n", msg); fails++; } } while (0)

static void Churn(void)
{
	int i, t;
	for (i = 0; i < 6; i++) Sim_Buy((u8)(rand() % DB_NUM_MOVIES));
	for (i = 0; i < 3; i++) Sim_SignAd((u8)i);
	for (i = 0; i < NUM_SLOTS; i += 2) { int m; for (m = 0; m < DB_NUM_MOVIES; m++) if (g_Game.owned[m]) { Sim_PlaceMovie((u8)i, (u8)m); break; } }
	Sim_PlaceAd(1, 0);
	Sim_NewsPick(0); Sim_NewsToggle(1);
	Sim_Borrow(500);
	for (t = 0; t < 700; t++) Sim_Tick();      // ~1,5 dia
}

int main(void)
{
	char code[SAVE_CHARS + 2], code2[SAVE_CHARS + 2];
	Game a;
	u8 r;
	int i;

	Sim_Init(4321);
	Churn();
	Sim_SaveCode(code);
	CHECK(strlen(code) == SAVE_CHARS, "tamanho do codigo");
	a = g_Game;
	r = Sim_LoadCode(code);
	CHECK(r == 0, "load valido");
	CHECK(a.day == g_Game.day && a.t == g_Game.t, "dia/hora");
	CHECK(a.money == g_Game.money, "dinheiro");
	CHECK(a.debt == g_Game.debt, "divida");
	CHECK(memcmp(a.image, g_Game.image, 3) == 0, "image");
	CHECK(memcmp(a.owned, g_Game.owned, DB_NUM_MOVIES) == 0, "filmes possuidos");
	CHECK(memcmp(a.news_sub, g_Game.news_sub, DB_NUM_AGENCIES) == 0, "agencias");
	CHECK(memcmp(a.contract, g_Game.contract, sizeof a.contract) == 0, "contratos");
	CHECK(memcmp(a.slot[0], g_Game.slot[0], sizeof a.slot[0]) == 0, "grade do jogador (incl. part)");
	Sim_SaveCode(code2);
	CHECK(strcmp(code, code2) == 0, "save(load(save)) idempotente");

	// minusculas e O/I tolerados; sujeira detectada
	{ char low[SAVE_CHARS + 2]; for (i = 0; i <= SAVE_CHARS; i++) low[i] = (code[i] >= 'A' && code[i] <= 'Z') ? code[i] + 32 : code[i]; CHECK(Sim_LoadCode(low) == 0, "load minusculo"); }
	{ char bad[SAVE_CHARS + 2]; strcpy(bad, code); bad[10] = (bad[10] == 'A') ? 'B' : 'A'; CHECK(Sim_LoadCode(bad) == 2, "checksum detecta alteracao"); }
	{ char bad[SAVE_CHARS + 2]; strcpy(bad, code); bad[5] = '!'; CHECK(Sim_LoadCode(bad) == 1, "caractere invalido"); }
	CHECK(Sim_LoadCode("ABC") == 1, "tamanho invalido");
	// carregar de novo o original depois de falhas nao quebra o estado
	CHECK(Sim_LoadCode(code) == 0, "load apos falhas");

	// noticias: comprar item reduz caixa e preenche o telejornal; qualidade 0..100; frescor decresce
	Sim_Init(99);
	{
		i32 m0 = g_Game.money; u8 q0 = Sim_NewsQuality(), q1;
		int picked = 0;
		for (i = 0; i < NEWS_POOL; i++) if (g_Game.news_pool[i].idx != NONE) { CHECK(Sim_NewsPick((u8)i) == 0, "pick noticia"); picked++; }
		q1 = Sim_NewsQuality();
		CHECK(picked >= 1, "pool tinha noticias");
		CHECK(g_Game.money < m0, "noticia custa dinheiro");
		CHECK(q0 == 0 && q1 > 0 && q1 <= 100, "qualidade do telejornal");
		CHECK(Sim_NewsFresh(0) == 100 && Sim_NewsFresh(5) == 50 && Sim_NewsFresh(20) == 10, "frescor");
	}
	// pool de noticias sem buracos depois de comprar um item do meio
	Sim_Init(5);
	{
		int n0 = 0, n1 = 0, gap = 0;
		for (i = 0; i < NEWS_POOL; i++) n0 += g_Game.news_pool[i].idx != NONE;
		if (n0 >= 3) { CHECK(Sim_NewsPick(1) == 0, "pick do meio"); }
		for (i = 0; i < NEWS_POOL; i++) { if (g_Game.news_pool[i].idx != NONE) n1++; else if (i + 1 < NEWS_POOL && g_Game.news_pool[i + 1].idx != NONE) gap = 1; }
		CHECK(n1 == n0 - 1 && !gap, "pool compactado (sem buracos)");
	}
	// credito: limite, juros, quitacao
	Sim_Init(7);
	{
		i32 lim = Sim_CreditLimit();
		CHECK(lim == CREDIT_BASE + CREDIT_PER_IMAGE * g_Game.image[0], "limite de credito");
		CHECK(Sim_Borrow(lim + 1) == 1, "acima do limite recusado");
		CHECK(Sim_Borrow(1000) == 0 && g_Game.debt == 1000, "emprestimo");
		{ int t; i32 d0 = g_Game.debt; for (t = 0; t < DAY_MINUTES; t++) Sim_Tick(); CHECK(g_Game.debt == d0 + d0 * INTEREST_PCT / 100, "juros diarios"); }
		{ i32 d = g_Game.debt, mny = g_Game.money; u8 rr = Sim_Repay(d); CHECK(mny >= d ? (rr == 0 && g_Game.debt == 0) : rr == 1, "quitar (ou sem caixa)"); }
	}
	// venda: so se possui e fora da grade; valor <= metade do preco
	Sim_Init(11);
	{
		i32 m0;
		Sim_Buy(5);
		CHECK(Sim_MovieValue(5) <= Sim_MoviePrice(5) / 2, "revenda <= 50% do preco");
		Sim_PlaceMovie(0, 5);
		CHECK(Sim_Sell(5) == 2, "nao vende filme na grade");
		Sim_ClearSlot(0);
		m0 = g_Game.money;
		CHECK(Sim_Sell(5) == 0 && g_Game.money == m0 + Sim_MovieValue(5) && !g_Game.owned[5], "venda ok");
		CHECK(Sim_Sell(5) == 1, "nao vende o que nao possui");
	}
	printf(fails ? "RESULT: %d FAIL(S)\n" : "RESULT: ALL PASS\n", fails);
	return fails != 0;
}
