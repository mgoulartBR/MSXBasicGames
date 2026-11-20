# Keen Dreams MSX2 — o que fizemos de 0.9.62 até 0.9.98 (resumo detalhado)

Este documento resume o trabalho de otimização de desempenho feito sobre a conversão MSX2/MSX2+ de Keen Dreams (MSXgl + SDCC, ROM ASCII16 de 2 MiB). O registro técnico completo, com todas as medições, está em `RELATORIO.md`; a pasta `release-0.9.98/` tem o pacote para publicar (ROM, fontes, notas, resposta ao comentário do itch.io, somas SHA-256).

**Ressalva geral:** tudo foi medido no **openMSX** (C-BIOS MSX2+), com o Z80 forçado a 3,58 / 5,37 / 7,16 MHz. **Nada foi testado em hardware real**, e o openMSX não reproduz todas as esperas de E/S do VDP de uma máquina de verdade. Os números de fps são métricas de bancada (renders por segundo em roteiros repetíveis), não garantia de desempenho em um MSX real.

---

## 1. De onde veio o pedido

A mensagem recebida (itch.io, byturn) dizia: (1) os atores são desenhados em **dois passes `LMMM`** (AND com a máscara + OR com a cor) e um único `LMMM` com `TIMP` faria o mesmo em um passe, dobrando a velocidade de desenho e liberando VRAM; (2) projéteis pequenos e rápidos poderiam ser **sprites de hardware**, cortando comandos de VDP por quadro.

### Veredito (medido, não suposto)
- **Sugestão 1 — correta.** Com uma ROM de prova de conceito que só trocava AND+OR por TIMP, o tempo de VDP ocupado por quadro caiu de 405 → 179 ms (−56%) e os comandos de 639 → 306 numa cena dominada por atores. Onde o custo é restauração de fundo/rolagem, o TIMP não ajuda (−2%).
- **Sugestão 2 — válida, mas limitada pela arte e pela RAM.** O Keen sozinho usa 8 sprites na mesma linha de varredura em 54 dos 56 quadros (limite do V9958: 8 por linha); um protótipo de página única com só fundo e Keen chegou a 13,4 fps (teto teórico) mas não se realiza no jogo completo (HUD, itens, primeiro plano, RAM cheia). Sprites de hardware seguem onde já existem (Keen, cenoura, tomate).

## 2. Linha do tempo das versões

| Versão | O que mudou | Efeito medido |
|---|---|---|
| **0.9.63-timp** | Conversão para TIMP: paleta (índice 9 → "segundo preto"), atlas dos atores reescrito pelo próprio VDP no carregamento, herói em passe único, `FastEnemy`. Cobertura parcial (Hill 28/39, Melon 44/52, Bridge 66/75, Rapids 13/70, Pass 14/84, City 27/50, Apple 13/49, Grove 13/79, Bay 0/64, Swamp 11/81, Boobus 1/60, Castle 12/66). | `HeroBitmap` 21,7 → 13,4 ms; `ActorDraw` 6,3 → 4,8 ms |
| 0.9.64 … 0.9.66 | HUD em Z80; **física do Keen a 35 Hz**; chamadas de inimigos inativos eliminadas | menos CPU em Hill |
| 0.9.67 … 0.9.73 | Diagnóstico do `RingScene`, varreduras em Z80 (`ScanWin`, `SlotScan`, `HillSelect`, `HWUpload`), HUD movido em 1 comando; **0.9.68**: troca de página/rolagem na interrupção de vblank | Hill a 7,16 MHz: 22,75 → 29,05 fps |
| 0.9.74 / 0.9.75 | Correção de HUD fantasma (Apple) e de ruído em sprites | correções |
| **0.9.76 / 0.9.77** | Barra de HUD fixa no rodapé por interrupção de linha; fallback automático abaixo de ~5 MHz | barra com 0 pixels diferentes |
| 0.9.78 … 0.9.87 | Atributos do herói, `CombatTick` de Hill, travessias de banco, `Columns`, `ActorsDraw`/`HillSelect`, Grove, Castle, `BonusList`, `Mod40`, `LineISR` mais tarde | Hill a 5,37 MHz: 18,85 → 30,4 fps |
| **0.9.88-fix** | **Corrige um congelamento grave** (corrida `BarOn`/`LineISR`, 0.9.76–0.9.87) | fase 5 sem travar |
| 0.9.89 / 0.9.90 | Listas de atores em Z80 (`ActorNext`, `ObjTouch`); **`Rows` em Z80** | fase 6: 4,7 → 6,1 fps |
| 0.9.91-flip | Desenho não espera o VDP antes do flip; `SoundISR` adia o flip se o VDP está ocupado | ≈ +1–2% |
| 0.9.92-bombas | Laço de bombas de City/Apple em Z80 | fase 6 +15%, fase 7 +8% |
| **0.9.93-pass** | `PassTick`/`PassCollect` com `ActorNext` | **fase 5 +26%** |
| **0.9.94-listas** | `ObjList` (chaves/bombas, Z80, banco fixo) e `ActorNext` nos laços de coleta | **fase 8 +78%**, fase 12 +38%, fase 4 +14% |
| 0.9.95-boobus | Passe de chão do chefe só quando precisa | fase 11 +12% |
| **0.9.96-chefe-timp** | **Chefe de Boobus em TIMP de passe único** (metade dos pixels e dos bytes de textura) | **fase 11 +42%** |
| 0.9.97-scan | Listas de bônus/picapes próximos com `ScanWin` | fase 2 +11% |
| **0.9.98-carrinhos** *(atual)* | `CartStep` em Z80 com refill por carrinho | fase 2 +4% |

### Fps por fase a 5,37 MHz (roteiro de posição controlada; renders/s)
| Fase | 0.9.75 | 0.9.98 |
|---|---|---|
| 1 Hill | 9,1 | 14,7 |
| 2 Mines | 6,8 | 8,8 |
| 3 Bridge | 5,1 | 6,3 |
| 4 Rapids | 6,8 | 8,9 |
| 5 Pass | 10,1 | 14,4 |
| 6 City | 4,6 | 6,6 |
| 7 Apple | 6,7 | 12,4 |
| 8 Grove | 3,7 | 17,4 |
| 9 Bay | 8,7 | 11,9 |
| 10 Swamp | 7,0 | 11,2 |
| 11 Boobus | 4,6 | 7,2 |
| 12 Castle | 6,3 | 10,9 |

O teletransporte refaz o anel a cada ponto, então os valores absolutos são menores do que andando de verdade; vale a comparação entre versões (trajetórias variam ±10% entre ROMs). Andando 40 s (0.9.98): a 5,37 MHz Hill 39,6, Grove 35,9, Castle 22,2, Apple 18,6, Bridge 15,8, City 14,3, Rapids 13,6, Bay 12,1, Mines 12,1, Pass 11,4, Boobus 6,1; a 7,16 MHz Hill 44,1, Grove 41,9, Apple 29,8. A 3,58 MHz o jogo roda lento (Hill 21,6; demais 3,5–17) e a barra de HUD fica desligada. A fase 10 não foi medida andando (o roteiro de entrada não inicia a fase).

## 3. Como verificamos (método)

Cada otimização foi aceita só com **equivalência demonstrada** e medida antes/depois:
- **Oráculos no emulador** (Tcl no openMSX; `mkprof.py`): breakpoint na entrada, recomputa a regra original no retorno e compara — `BTDIAG` (bombas), `OLORC` (`ObjList`), `MTORC` (busca de tile), `CSORC` (carrinhos), `FLIPCHK` (nenhum flip sobre comando de VDP em execução), `BBORC`/`bbcheck2.py` (desenho do chefe, por modelo de VRAM), `SNAP`/`SNAPA` (instantâneos de estado por tick entre ROM antiga e nova, com o Keen parado e posições `BENCH_PLACE`), `VARS` (placar e itens). Em todas as versões que entraram: **0 divergências**.
- **Comparação de fluxos de comandos de VDP** (`cdcmp.py`): `Columns` e `Rows` em 11 fases.
- **Modelos exaustivos** em Python (todas as palavras × deslocamentos; todo `player_x`/`player_y`; todas as janelas de bônus/picapes das 12 fases).
- **Regressão**: `attrcheck`, barra de HUD com 0 pixels diferentes (NTSC/PAL, 5,37 e 7,16 MHz), volta ao mapa, fases andando 50 s invulnerável sem congelar, capturas com renders > 0.
- **Reprodutibilidade**: os dois pacotes (`KEEN-0.9.98.ROM` e as fontes) foram conferidos — compilar `KEEN-0.9.98-fontes.zip` dá a mesma ROM (SHA-256 `370434e5…89221a9`).
- **Roteiro de posição controlada** (`BENCH_GOD=2`, `BENCH_CKPT`): teletransporta o Keen a pontos fixos e mede renders/s por segmento — repetível, mas caótico por segmento (±5–10%); por isso os ganhos pequenos foram confirmados também pelo perfil por chamada.

## 4. Resultados negativos (também fazem parte do trabalho)

- **Laço de bombas em C**: sem ganho — e o oráculo achou um **bug do SDCC** (IY reaproveitado depois de carregado com `&player_x`). O ganho veio ao escrever o laço em Z80.
- **Busca de tile em dois níveis** (`Shift9`): correta, mas sem ganho (custo fixo da preparação anula a economia).
- **Sobreposição VDP/lógica**: teto medido ≈ 2%.
- **Chefe da fase 11 com `HMMM` no retângulo opaco**: equivalente pixel a pixel, mas **mais lento** (43 → 62 ms): dividir 4 comandos em 14–20 custa mais que os pixels poupados. O que funcionou foi o TIMP em passe único (0.9.96).
- **Cobertura do TIMP nas fases 4–12**: a CPU só espera o VDP 4–9% do tempo nessas fases; mesmo eliminando todo o AND/OR, o ganho seria ≤ 4–9%, e a conversão exigiria refazer o empacotador do atlas (as 4 páginas de VRAM estão cheias e os retângulos de máscara/cor se sobrepõem). Não valeu.
- Teclado (premissa errada), barra no topo (inviável), página única (teto que não se realiza): medidos e descartados.

## 5. O que ainda poderia render
1. `CartContact` (≈ 3% da fase 2) em Z80: ≈ +2%.
2. `ActorNext` em `AppleTick`/`AppleCollect`/`CityCollect`: não coube no banco 50 (estouro de 175 bytes); exigiria liberar espaço lá.
3. Dados streamed ainda em AND/OR (Brocco, carrinhos, Tater, City, Apple, Castle) convertidos para TIMP, onde a espera pelo VDP é alta.
4. Fases 3 e 11: dominadas por `VDP_CommandWait`/`VPF` (anel, envio de texturas), sem ponto único.
5. Teste em MSX2+/turbo R de verdade — pode mudar onde vale gastar esforço.

## 6. Pendências e limites conhecidos
- **Não testado em hardware real.**
- Capturas isoladas mostram, às vezes, um quadro anômalo da barra de HUD (1 736 pixels diferentes); aparece em ROMs antigas e novas, em fases diferentes; é aleatório por temporização, **sem causa conhecida**.
- A física do Keen a 35 Hz (desde a 0.9.65) altera sutilmente o comportamento.
- O banco fixo tem pouca folga (dezenas de bytes) e a RAM termina em 0xEFFD (limite 0xF000).
- Abaixo de ~5 MHz a barra de HUD é desativada automaticamente.

## 7. Arquivos
- `release-0.9.98/` — `KEEN-0.9.98.ROM`, `KEEN-0.9.98-fontes.zip`, `NOTAS-DA-VERSAO.md` (PT + EN), `RESPOSTA-ITCHIO.md`, `SHA256SUMS.txt`.
- `RELATORIO.md` — registro técnico completo (cada versão, método, tabelas, negativos).
- `0.9.63-timp/` … `0.9.98-carrinhos/` — ROM e fontes alteradas de cada versão.
- Ferramentas: `mkprof.py` (perfil e oráculos), `runp.sh`, `ckst.sh`/`ckall.sh`/`trackall.sh`/`ckpts.json`, `attrcheck.py`, `barchk.py`, `cdcmp.py`, `retmap.py`, `bbcheck2.py`, `pcsym2.py`, `cs2sym.py`, `timpwhy.py`.
