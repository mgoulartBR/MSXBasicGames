# Decisoes de portabilidade

```
Original system : Mad TV (1991, Rainbow Arts) - DOS/Amiga/ST, simulacao de gestao em tempo real
Data source     : banco do remake open source TVTower (XML) - ver LICENSES.md
MSX target      : MSX2 (V9938), 64 KB RAM minimo, cartucho ROM (nada testado em hardware real)

Video strategy  : Screen 5 (256x212, 16 cores), texto bitmap do MSXgl + barras com VDP HMMV.
                  Jogo de menus/telas estaticas: sem scroll; sprites de hardware so para cursor/personagens depois.
Audio strategy  : PSG (a definir; nenhuma implementacao ainda)
Input strategy  : teclado (setas) + joystick 1, ambos simultaneos. Mouse original -> cursor por setas/botao.
Memory strategy : tabelas const em ROM geradas por tools/convert_db.py; RAM so para estado do jogo.
ROM mapper      : ROM_32K ate ~24 KB de dados; migrar para ROM_ASCII16 quando passar (previsto: lotes de filmes por banco).

Features preserved : categorias de programacao, atributos de filme (critica/ritmo/bilheteria, FSK18, blocos).
Features adapted   : catalogo do TVTower reduzido a subconjunto curado (88 filmes, 24 anuncios, 24 noticias);
                     8 categorias (TVTower tem generos mais finos -> CATEGORIES em tools/convert_db.py);
                     preco de filme provisorio = media(criticas,ritmo,bilheteria) * price_mod (NAO e a formula do TVTower).
Features omitted   : (ainda nada implementado alem do navegador de filmes)

Known limitations  : textos so em ingles/ASCII; titulos com ${...} nao resolvidos sao descartados.
Performance issues : texto bitmap em Screen 5 e lento (medido abaixo).
```

## Medicoes (openMSX 19.1, C-BIOS MSX2, JIFFY = interrupcoes do VDP; build com `MADTV_PROF=1`)
| Operacao | JIFFY (max) |
|---|---|
| redraw de 2 linhas da lista | 7 |
| redraw da lista completa + cabecalho | 45 |
| painel de detalhes (texto + 3 barras) | 17 |

Uma troca de selecao custa ~24 jiffies (~0,4 s a 60 Hz) - lento para UI. Foi tentado `Print_SetVRAMFont`
(fonte copiada via VDP): 2 linhas 7->3, detalhes 17->10, mas a fonte so tem uma cor e `y=256` (pagina 1) foi
truncado para 0 pelo tipo `UY` de 8 bits; **nao adotado**. Proximos passos: fonte em VRAM com variantes de cor
(pagina 1, via VDP_CommandHMMM), ou font tile-based em Screen 4/2.

## Mapeamento TVTower -> categorias Mad TV (tools/convert_db.py)
Lovestory=Romance | Action=Acao,Aventura,Western | Monumental=Monumental,Historia | Comedy=Comedia |
Crime=Crime,Thriller,Misterio | Culture=Documentario | SciFi=Ficcao cientifica,Fantasia | Other=Animacao,Drama,Familia,Terror.
Flag X-rated (64) do TVTower -> FSK18 (confirmado empiricamente: 160/194 filmes do genero Erotic e 109 de Acao tem o bit).
