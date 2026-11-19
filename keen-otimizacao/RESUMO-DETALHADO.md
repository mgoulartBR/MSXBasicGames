# Keen Dreams MSX2 — o que fizemos de 0.9.62 até 0.9.92 (resumo detalhado)

Este documento resume o trabalho de otimização de desempenho feito sobre a conversão MSX2/MSX2+ de Keen Dreams (MSXgl + SDCC, ROM ASCII16 de 2 MiB). O registro técnico completo, com todas as medições, está em `RELATORIO.md`; aqui está a visão de conjunto, em ordem, com o que funcionou, o que não funcionou e o que falta.

**Ressalva geral:** tudo foi medido no **openMSX** (C-BIOS MSX2+), com o Z80 forçado a 3,58 / 5,37 / 7,16 MHz. **Nada foi testado em hardware real**, e o openMSX não reproduz todas as esperas de E/S do VDP de uma máquina de verdade. Os números de fps são métricas de bancada (renders por segundo em roteiros repetíveis), não garantia de desempenho em um MSX real.

---

## 1. De onde veio o pedido

A mensagem recebida (itch.io) dizia, em resumo: (1) os atores são desenhados em **dois passes `LMMM`** (AND com a máscara + OR com a cor) e um único `LMMM` com `TIMP` faria o mesmo em um passe, dobrando a velocidade de desenho e liberando VRAM; (2) projéteis pequenos e rápidos poderiam ser **sprites de hardware**, cortando comandos de VDP por quadro.

### Veredito (medido, não suposto)
- **Sugestão 1 — correta.** Com uma ROM de prova de conceito que só trocava AND+OR por TIMP, o tempo de VDP ocupado por quadro caiu de 405 → 179 ms (−56%) e os comandos de 639 → 306 num cenário dominado por atores. Onde o custo é restauração de fundo/rolagem, o TIMP não ajuda (−2%). Os pares AND/OR eram ~84% do tempo ativo do VDP no perfil original.
- **Sugestão 2 — válida, mas limitada pela arte e pela RAM.** O Keen sozinho usa 8 sprites na mesma linha de varredura em 54 dos 56 quadros (limite do V9958: 8 por linha), então sprites de hardware para os demais atores não cabem na maioria dos casos; um protótipo de página única com só o fundo e o Keen chegou a 13,4 fps (teto teórico) mas não se realiza no jogo completo (HUD, itens, primeiro plano e RAM persistente cheia). Sprites de hardware seguem sendo úteis só onde já existem (Keen, cenoura, tomate).

## 2. Linha do tempo das versões

| Versão | O que mudou | Efeito medido |
|---|---|---|
| **0.9.63-timp** | Conversão para TIMP: paleta (índice 9 → "segundo preto" 1), atlas dos atores reescrito pelo próprio VDP no carregamento (`TimpConvert`), herói em bitmap em passe único, `FastEnemy`. **Cobertura parcial** (quadros convertidos/mascarados: Hill 28/39, Melon 44/52, Bridge 66/75, Rapids 13/70, Pass 14/84, City 27/50, Apple 13/49, Grove 13/79, Bay 0/64, Swamp 11/81, **Boobus 1/60**, Castle 12/66). | `HeroBitmap` 21,7 → 13,4 ms; `ActorDraw` 6,3 → 4,8 ms |
| 0.9.64-hud-z80 | Laço de cópia do HUD em Z80 | menos CPU por quadro |
| 0.9.65-35hz | Física do Keen a 35 Hz (dois ticks de 70 Hz numa passada) | **muda levemente o comportamento da física** (gravidade/aceleração aplicadas como no original nos ticks ímpares); ver RELATORIO |
| 0.9.66-enemy-calls | Chamadas de inimigos inativos eliminadas | ganho de CPU em Hill |
| 0.9.67-draw → 0.9.73-ring | Diagnóstico do `RingScene`, varredura de listas em Z80 (`ScanWin`, `SlotScan`, `HillSelect`, `HWUpload` desenrolado), HUD movido em 1 comando | Hill a 7,16 MHz: 22,75 → 29,05 fps |
| 0.9.68-vblank | Troca de página/scroll feita na interrupção de vblank (`SoundISR`) | tira ~15% de espera por `Halt` |
| 0.9.74-hudfix / 0.9.75-vdpfix | Correção de HUD fantasma (Apple) e de ruído em sprites | correções |
| **0.9.76/0.9.77-barra** | Barra de HUD fixa no rodapé por interrupção de linha (`LineISR`), 8 linhas com espaçador; fallback automático abaixo de ~5 MHz | barra com 0 pixels diferentes nas capturas |
| 0.9.78 … 0.9.87 | Mais laços em Z80: atributos do herói, `CombatTick` de Hill, travessias de banco, `Columns`, `ActorsDraw`/`HillSelect`, Grove, Castle, `BonusList`, `Mod40`, `LineISR` armada mais tarde (linha 178) | Hill a 5,37 MHz: 18,85 → 30,4 fps |
| **0.9.88-fix** | **Corrige um defeito grave de congelamento** presente nas 0.9.76–0.9.87 (corrida entre `BarOn` e a `LineISR` gerava tempestade de interrupções, observada na fase 5) | fase 5 sem travar em 70 s |
| 0.9.89-lists / 0.9.90-rows | Listas de atores (Mines, Bridge, City, Rapids), `ObjTouch`; **`Rows` em Z80** | fase 6: 4,7 → 6,1 fps |
| **0.9.91-flip** | O desenho não espera mais o VDP antes de pedir o flip; o `SoundISR` adia o flip se o VDP ainda estiver ocupando (bit CE). Para caber no banco fixo, `VDP_USE_VRAM16K` desligado (≈190 bytes). | ≈ +1–2% (o adiamento nunca precisou disparar a 5,37 MHz) |
| **0.9.92-bombas** *(atual)* | Laço de bombas das fases 6 e 7 em Z80 | `CityObjectsTick` 0,91 → 0,42 ms; fase 6 6,26 → 7,21 fps, fase 7 10,89 → 11,72 fps (roteiro controlado) |

### Fps por fase a 5,37 MHz (roteiro de posição controlada, 0.9.75 → 0.9.88)
Hill 9,1 → 14,9; Mines 6,8 → 7,4; Bridge 5,1 → 6,1; Rapids 6,8 → 7,2; Pass 10,1 → 12,0; City 4,6 → 5,9; Apple 6,7 → 10,9; Grove 3,7 → 8,5; Bay 8,7 → 11,2; Swamp 7,0 → 11,0; Boobus 4,6 → 4,7; Castle 6,3 → 10,3. (O teletransporte refaz o anel a cada ponto, então os valores absolutos são menores que andando de verdade; vale a comparação entre ROMs.) Depois: 0.9.90 fase 6 em 6,1 e 0.9.92 em 7,2. Andando, em corridas de 50 s, Hill chega a ~30 fps a 5,37 MHz e ~37–38 fps a 7,16 MHz; as outras fases ficam entre 5 e 18 fps.

## 3. Como verificamos (método)

Cada otimização foi aceita só com **equivalência demonstrada** e medida antes/depois:
- **Oráculos no emulador** (Tcl no openMSX): breakpoint na entrada da rotina, recomputa a regra original no retorno e compara — `ActorNext`, `ObjTouch`, `BonusList`, `Mod40`, `GroveSelect`, bombas (`BENCH_BTDIAG`), busca de tile (`BENCH_MTORC`), flip (`BENCH_FLIPCHK`: nenhum flip sobre comando de VDP em execução). Em todos, **0 divergências** nas versões que entraram na ROM.
- **Comparação de fluxos de comandos de VDP**: ROM de diagnóstico que chama a versão C e a Z80 com os mesmos argumentos, registrando as escritas nas portas do VDP (`cdcmp.py`) — usada em `Columns` e `Rows` (11 fases).
- **Modelos exaustivos** em Python (todas as 65 536 palavras × 8 deslocamentos; todo `player_x`/`player_y` de 16 bits para as bombas).
- **Regressão**: `attrcheck` (tabelas de atributos de sprite sem corrupção), barra de HUD com 0 pixels diferentes (NTSC e PAL, 5,37 e 7,16 MHz), volta ao mapa, fases 1/5/12 andando 50 s invulnerável sem congelar, capturas com `renders > 0`.
- **Roteiro de posição controlada** (`BENCH_GOD`, `BENCH_CKPT`): teletransporta o Keen a pontos fixos e mede renders/s por segmento. É repetível mas caótico por segmento (±5%); por isso os ganhos pequenos foram confirmados também por perfil por chamada.

## 4. Resultados negativos (também fazem parte do trabalho)

- **Laço de bombas em C** (reescrito com testes sem sinal): sem ganho — e o oráculo achou um **bug do SDCC** (IY reaproveitado depois de carregado com `&player_x`, lendo o byte errado do spawn). O ganho veio só ao escrever o laço em Z80.
- **Busca de tile em dois níveis** (`Shift9`): correta (modelo exaustivo + 100 mil chamadas, 0 divergências) mas sem ganho — o custo fixo da preparação anula a economia nos offsets 0–3.
- **Sobreposição VDP/lógica**: o teto medido é ~2% (a maior parte da espera é dentro de `VDP_CommandSetupR32`, limite de vazão do VDP).
- **Chefe da fase 11 com `HMMM` no retângulo opaco**: equivalente pixel a pixel (modelo, 98 chamadas, 0 divergências) mas **mais lento** (43,4 → 61,8 ms por desenho): dividir 4 comandos em 14–20 custa mais que os pixels poupados.
- Passos como `Teclado` (premissa errada) e a barra no topo (inviável) também foram medidos e descartados — ver `RELATORIO.md`.

## 5. Onde a sugestão original ainda tem o que render

A mensagem estava certa, e a conversão para TIMP ficou **parcial**. O que resta, em ordem de interesse:
1. **Chefe da fase 11 (Boobus) em TIMP de passe único.** É o maior gargalo localizado do jogo hoje: `BoobusBitmap` ≈ 38% do tempo da fase (≈ 100 ms por desenho) com AND+OR sobre ~6–10 mil pixels e reenvio de ~6 KB de textura por `VPF` a cada troca de quadro. Passe único corta pela metade os pixels de `LMMM` e os bytes enviados. Exige recolorir os pretos opacos do chefe para o índice 1 (como nas outras fases) e um conversor específico para os dados "streamed".
2. **Cobertura do TIMP nas fases 4–12** (Rapids 13/70, Pass 14/84, Grove 13/79, Bay 0/64, Swamp 11/81, Castle 12/66…): gerar a imagem TIMP direto nos `build_*_assets.py`, onde o layout é conhecido, em vez de converter no lugar.
3. Dados "streamed" restantes: carrinhos (Mines), Tater, Brocco, City, Apple, Castle.
4. Sprites de hardware para projéteis (MSX2+): possível só onde o orçamento de 8 sprites por linha permitir.
5. `BoobusTick` (≈ 5% do quadro) em Z80, nos moldes do laço de bombas.
6. Anel/VDP em geral (fases 2, 3, 4, 6, 11 ≈ 5–9 fps): perfil plano, sem ponto único; qualquer ganho é pequeno.

## 6. Pendências e limites conhecidos
- **Não testado em hardware real.** Todo o fps é de emulação.
- Uma captura isolada da fase 5 (Keen morto, t = 15 s) mostrou o quadro com a configuração da barra em vez da do jogo; não reproduzi nas 4 repetições seguintes. Provavelmente transitório na transição de morte/`BarOff`; **não resolvido**.
- A 0.9.65-35hz altera sutilmente a física do Keen (35 Hz); vale registrar nas notas da versão.
- O banco fixo tem pouca folga (≈ 160 bytes após desligar `VDP_USE_VRAM16K`) e a RAM termina em 0xEFFD (limite 0xF000).
- Abaixo de ~5 MHz (3,58 MHz) a barra de HUD é desativada automaticamente e o HUD volta a ser desenhado no anel.

## 7. Arquivos
- `RELATORIO.md` — registro técnico completo (cada versão, método, tabelas, negativos).
- `0.9.63-timp/` … `0.9.92-bombas/` — ROM e fontes alteradas de cada versão (a ROM atual é `0.9.92-bombas/keen_msx_0.9.92-bombas.rom`, SHA-256 `4d9373582f90f81a952fba24be1bdbb8d915a2b0815cc42d358b096a07849c9a`).
- Ferramentas: `mkprof.py` (perfil e oráculos `BTDIAG`, `MTORC`, `FLIPCHK`, `BBORC`, `COLDIAG`), `runp.sh`, `ckst.sh`/`ckall.sh`/`trackall.sh`/`ckpts.json` (posição controlada), `attrcheck.py`, `barchk.py`, `cdcmp.py`, `retmap.py`, `bbcheck.py`, `pcsym2.py`.

## 8. Rascunho de notas para a próxima versão
> **Keen Dreams MSX2 0.9.92**: barra de HUD fixa no rodapé (MSX2+ a partir de ~5 MHz); desenho de atores em TIMP de passe único (parcial: Hill, Melon e Bridge bem cobertas); anel de cenário (`Columns`/`Rows`) e listas de atores/itens reescritos em Z80; flip de vblank sem espera de VDP; correção de congelamento da barra (0.9.76–0.9.87); laço de bombas de City/Apple em Z80. Hill passa de 30 fps a 5,37 MHz (openMSX); as demais fases ficam entre 5 e 18 fps. Pendente: cobertura TIMP das fases 4–12 e do chefe de Boobus.

## 9. Resposta sugerida à mensagem original
> Você estava certo: a troca AND+OR → TIMP corta o tempo de VDP dos atores quase pela metade (medi −56%) e libera VRAM. Já converti o herói, o atlas de Hill/Melon/Bridge e vários quadros das demais fases; a conversão ainda é parcial (o chefe de Boobus, os carrinhos, Rapids/Pass/Grove/Bay/Swamp/Castle têm dados próprios e estão na fila). Além disso, reescrevi em Z80 os laços do anel de cenário e das listas de atores, movi a troca de página/scroll para a interrupção de vblank e fixei a barra de HUD no rodapé; Hill foi de ~6 fps (Z80 a 3,58 MHz, 0.9.67) para ~30 fps a 5,37 MHz e ~37 fps a 7,16 MHz (openMSX, sem teste em hardware real). Sprites de hardware para projéteis fazem sentido onde cabem (limite de 8 por linha); o Keen já usa 8 na mesma linha em quase todos os quadros, então a margem é pequena. Obrigado pela dica — foi o ponto de partida de tudo isso.
