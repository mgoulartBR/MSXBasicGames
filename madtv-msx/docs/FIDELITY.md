# Fidelidade ao Mad TV original (auditoria 2.10)

Fonte da comparacao: o manual oficial (resumo em `docs/RULES.md`) e as capturas do DOS em `original/dos/` (nao versionadas). Estados: **igual** (implementado como no manual/capturas), **simplificado** (existe, com regra propria ou reduzida), **falta**, **nao verificado** (nada em tela permite comparar; as formulas do original nao estao documentadas).

| Area | Original | Port | Estado |
|---|---|---|---|
| Dia | 17:00-01:00, 3 velocidades | idem | igual |
| Grade | 7 horas (18-00), noticias na hora cheia, bloco :05, anuncio :55, blocos de ate 4 | idem; 6 icones (opcoes, contratos, resumo, audiencias, borracha, slots) | igual |
| Grade de amanha | editar hoje e amanha | so hoje | **falta** |
| Limite de 4 entradas por genero | sim | nao implementado | **falta** |
| Image | 100 pts entre 3 emissoras; < 20 = crise; 0 = falencia | idem | igual |
| Sammy | Noticias +8, Producao propria +6, Cultura +4 | so o telejornal disputa (a cada 7 dias, +5/+3 e 300k) | simplificado |
| Audiencia / alcance | torres e satelite; contratos pelo numero absoluto | torres, cabo e satelite (6 niveis); formulas proprias | simplificado |
| Chefe (credito) | juros altos, humor melhor apos Sammy | credito/juros proprios | simplificado |
| Corretor | aluguel por troca de inquilino + comissao | so cobertura (torres/cabo/satelite) | **falta** (aluguel) |
| Noticias | 3 agencias, taxa diaria, noticia velha pesa | idem (frescor x qualidade) | igual |
| Agencia de filmes | compra direta **e leilao** (fecha 01:00) | so compra direta; rivais levam filmes de forma deterministica | **falta** (leilao) |
| Filmes | preco, ano, FSK, atualidade, critica, bilheteria, opiniao da Betty | tudo menos opiniao da Betty por filme; elenco por filme (estrelas) e extra do port | simplificado |
| Producao propria | roteiro -> estudio P/M/G -> lista de compras -> supermercado -> produzir; "No Trend" | roteiro + estudio com orcamento low/medium/high; sem lista de compras nem "No Trend" | simplificado / **falta** |
| Supermercado | 10 presentes, efeito cai com a repeticao | idem | igual |
| Betty | simpatia <= Image, pretendentes, pedido de casamento + viagem | idem | igual |
| Sabotagem | voce envia terrorista/oficial de justica aos rivais pela placa de avisos | so os rivais sabotam voce (fim do dia, quem esta na frente) | **falta** (acao do jogador) |
| Escritorio | grade, controle, graficos, extrato, mapa, quadro de saves | grade, resumo, audiencias, slots/codigo | **falta** (mapa, graficos historicos) |
| Salas dos rivais | espionagem | grade e audiencias de hoje | igual |
| Controles | botao esquerdo entra / direito sai | idem (mouse/teclado/joystick) | igual |
| Console | TVs com imagem (programa e noticias), dinheiro, Betty, hora | TVs com texto (no ar, Image, ticker); sem imagens nas TVs | simplificado |
| Salas | cenas ilustradas | todas as salas tem cena (build local com a arte do original; build publico usa telas de texto) | igual (local) |
| Formulas de audiencia, juros, IA dos rivais | nao documentadas | reconstruidas e balanceadas por autoplay | nao verificado |

Extras do port que o original nao tem: emissora escolhivel (MadTV/FunTV/SunTV), elenco por filme com estrelas, filmes "levados" pelos rivais, quatro slots de save em SRAM, ticker no console.

## Lacunas que mudariam a dinamica (nao entram em release sem decisao)
Grade de amanha, leilao, producao com lista de compras, sabotagem enviada pelo jogador, aluguel do corretor, limite por genero.
