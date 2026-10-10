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
	g_Game.tower = 3; g_Game.sym[0] = 17; g_Game.sym[1] = 9; g_Game.sym[2] = 4; g_Game.alive[2] = 0; g_Game.gift_have[3] = 2; g_Game.gift_have[GIFT_DREAM] = 1; g_Game.gift_uses[1] = 2; g_Game.gift_uses[8] = 3;
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
	CHECK(memcmp(a.sym, g_Game.sym, 3) == 0 && memcmp(a.alive, g_Game.alive, 3) == 0, "Betty: simpatia e emissoras vivas");
	CHECK(memcmp(a.gift_have, g_Game.gift_have, NUM_GIFTS) == 0 && memcmp(a.gift_uses, g_Game.gift_uses, NUM_GIFTS) == 0, "Betty: estoque e usos dos presentes");
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
	// Betty: presentes, teto pelo Image, decaimento, pedido de casamento
	Sim_Init(21);
	{
		u8 r; i32 m0;
		g_Game.money = 50000; g_Game.image[0] = 20; g_Game.sym[0] = 0;
		CHECK(Sim_BuyGift(2) == 0 && g_Game.gift_have[2] == 1 && g_Game.money == 50000 - Sim_GiftCost(2), "comprar presente");
		g_Game.gift_have[2] = GIFT_MAX_STOCK; CHECK(Sim_BuyGift(2) == 2, "estoque cheio");
		g_Game.money = 1; CHECK(Sim_BuyGift(3) == 1, "sem dinheiro para o presente");
		g_Game.money = 50000;
		CHECK(Sim_GiveGift(0) == 1, "dar presente sem estoque");
		CHECK(Sim_GiveGift(GIFT_DREAM) == 3, "Dream trip so no casamento");
		{ u8 e0 = Sim_GiftEffect(2); CHECK(Sim_GiveGift(2) == 0 && g_Game.sym[0] == e0 && g_Game.last_gain == e0, "efeito pleno do 1o presente");
		  CHECK(Sim_GiftEffect(2) < e0, "efeito cai com o uso"); }
		g_Game.sym[0] = 19; g_Game.gift_have[5] = 1; r = Sim_GiveGift(5);
		CHECK(r == 0 && g_Game.sym[0] == 20, "ganho limitado pelo Image (teto)");
		g_Game.gift_have[5] = 1; CHECK(Sim_GiveGift(5) == 2 && g_Game.gift_have[5] == 1, "simpatia no teto: recusa sem consumir");
		// pedido de casamento
		g_Game.sym[0] = 99; CHECK(Sim_Propose() == 1, "pedir com simpatia < 100");
		g_Game.sym[0] = 100; g_Game.image[0] = 100; CHECK(Sim_Propose() == 2, "pedir com rivais vivos");
		g_Game.alive[1] = g_Game.alive[2] = 0; g_Game.gift_have[GIFT_DREAM] = 0; CHECK(Sim_Propose() == 3, "pedir sem Dream trip");
		g_Game.gift_have[GIFT_DREAM] = 1; m0 = g_Game.money; CHECK(Sim_Propose() == 0 && g_Game.won == 1 && g_Game.gift_have[GIFT_DREAM] == 0 && g_Game.money == m0, "casamento");
	}
	// decaimento diario sem presente; com presente nao decai
	Sim_Init(22);
	{
		int t; u8 s0;
		g_Game.image[0] = 50; g_Game.sym[0] = 10; s0 = g_Game.sym[0];
		for (t = 0; t < DAY_MINUTES; t++) Sim_Tick();
		CHECK(g_Game.sym[0] <= s0 - 1 || g_Game.sym[0] < s0, "sem presente a simpatia esfria no fim do dia");
	}
	// emissora rival falida: sai da disputa (Image 0), audiencia zero e o jogador passa a ter mais Image
	Sim_Init(23);
	{
		int t;
		g_Game.image[0] = 98; g_Game.image[1] = 1; g_Game.image[2] = 1; g_Game.sym[1] = 1; g_Game.sym[2] = 1; g_Game.alive[0] = 0;   // jogador fora: so os rivais disputam
		for (t = 0; t < DAY_MINUTES; t++) Sim_Tick();
		CHECK(!g_Game.alive[1] || !g_Game.alive[2], "rival com Image 0 faliu");
		CHECK(g_Game.image[0] + g_Game.image[1] + g_Game.image[2] == 100, "soma do Image continua 100");
		CHECK((!g_Game.alive[1] && g_Game.image[1] == 0) || g_Game.alive[1], "rival falido tem Image 0");
	}
	// jogador com Image 0 = fim de jogo
	Sim_Init(24);
	{
		int t;
		g_Game.image[0] = 1; g_Game.image[1] = 60; g_Game.image[2] = 39;
		for (t = 0; t < DAY_MINUTES && !g_Game.game_over; t++) Sim_Tick();
		CHECK(g_Game.game_over, "jogador sem Image perde");
	}
	// 0.7: dificuldade vai no codigo; Image baixo fecha o credito
	Sim_Init(31);
	{
		char c[SAVE_CHARS + 2];
		g_Diff = 2; Sim_Init(31); Sim_SaveCode(c);
		g_Diff = 0; CHECK(Sim_LoadCode(c) == 0 && g_Diff == 2, "dificuldade restaurada pelo codigo");
		g_Diff = 0; Sim_Init(31); CHECK(g_Game.money == 3500, "easy: caixa 3500");
		g_Diff = 2; Sim_Init(31); CHECK(g_Game.money == 2000, "hard: caixa 2000");
		g_Diff = 1; Sim_Init(31);
		g_Game.image[0] = IMAGE_LOW - 1; CHECK(Sim_CreditLimit() == 0 && Sim_Borrow(100) == 1, "Image baixo: sem credito");
		g_Game.image[0] = IMAGE_LOW;     CHECK(Sim_CreditLimit() > 0 && Sim_Borrow(100) == 0, "Image 20: credito aberto");
	}
	// 0.9: torres (Corretor)
	g_Diff = 1; Sim_Init(41);
	{
		u8 r0 = Sim_Reach(0), i;
		i32 m0;
		g_Game.money = 20000; m0 = g_Game.money;
		CHECK(Sim_TowerCost() == 1500 && Sim_BuyTower() == 0 && g_Game.tower == 1 && g_Game.money == m0 - 1500, "comprar torre 1");
		CHECK(Sim_Reach(0) > r0, "torre aumenta o alcance");
		for (i = 0; i < 10; i++) Sim_BuyTower();
		CHECK(g_Game.tower == TOWER_MAX && Sim_TowerCost() == 0 && Sim_BuyTower() == 2, "nivel maximo de torres");
		g_Game.money = 0; g_Game.tower = 0; CHECK(Sim_BuyTower() == 1 && g_Game.tower == 0, "torre sem dinheiro");
		g_Game.tower = 2; g_Game.money = 5000;
		{ i32 a = g_Game.money; u16 t; for (t = 0; t < DAY_MINUTES; t++) Sim_Tick(); CHECK(a - g_Game.money >= DAILY_UPKEEP + 2 * TOWER_UPKEEP - 200, "manutencao das torres"); }
	}
	// 1.0: producao propria (Roteiros + Estudios)
	g_Diff = 1; Sim_Init(51);
	{
		u16 d; u8 m; char c[SAVE_CHARS + 2];
		g_Game.money = 20000;
		CHECK(Sim_StartProd(1) == 2, "produzir sem roteiro");
		CHECK(Sim_BuyScript(0) == 0 && g_Game.pstate == 1 && g_Game.money < 20000, "comprar roteiro");
		CHECK(Sim_BuyScript(1) == 2, "so um roteiro por vez");
		CHECK(Sim_StartProd(2) == 0 && g_Game.pstate == 2 && g_Game.pdays == 4, "iniciar producao (high = 4 dias)");
		CHECK(Sim_BuyScript(1) == 2, "roteiro durante producao");
		Sim_SaveCode(c); CHECK(Sim_LoadCode(c) == 0 && g_Game.pstate == 2 && g_Game.pdays == 4, "producao em andamento sobrevive ao save");
		for (d = 0; d < 4 * DAY_MINUTES; d++) { if (g_Game.pstate == 0) break; Sim_Tick(); }
		CHECK(g_Game.pstate == 0 && g_Game.own[0].used && g_Game.owned[DB_NUM_MOVIES], "producao termina e entra na biblioteca");
		CHECK(g_Game.own[0].q >= 70 && g_Game.own[0].q <= 95, "qualidade high 70..95");
		m = DB_NUM_MOVIES;
		CHECK(Mov(m)->blocks == g_Game.own[0].blocks && Sim_Quality(m) >= 60, "Mov() devolve a producao");
		CHECK(Sim_MoviePrice(m) > 0 && Sim_MovieValue(m) > 0, "producao tem preco e revenda");
		Sim_SaveCode(c); g_Game.own[0].used = 0; g_Game.owned[m] = 0;
		CHECK(Sim_LoadCode(c) == 0 && g_Game.own[0].used && g_Game.owned[m], "producao pronta sobrevive ao save");
		CHECK(strlen(Mov(m)->title) > 3, "titulo da producao reconstruido");
		Sim_OwnFree(m); CHECK(!g_Game.own[0].used && !g_Game.owned[m], "liberar producao");
		{ u8 k; g_Game.money = 50000; for (k = 0; k < NUM_OWN; k++) { g_Game.own[k].used = 1; } g_Game.pstate = 1; CHECK(Sim_StartProd(0) == 3, "biblioteca cheia impede filmar"); }
	}
	printf(fails ? "RESULT: %d FAIL(S)\n" : "RESULT: ALL PASS\n", fails);
	return fails != 0;
}
