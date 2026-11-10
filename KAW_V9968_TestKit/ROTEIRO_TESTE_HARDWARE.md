# Roteiro de teste — KAW9990P1.ROM em outro emulador ou em MSX real (≈ 5 minutos)

Serve para blueMSX (com GFX9000), MAME (`msx2` + `gfx9000`) ou MSX2 com cartão GFX9000/V9990.
Arquivo: `KAW9990P1.ROM` (256 KiB, mapper **ASCII8**; confira o SHA-256 em `SHA256SUMS.txt`).
Máquina: MSX2/MSX2+ com V9990 (GFX9000). Mouse na porta 1 ou 2 é o ideal; sem mouse, use I/J/K/L e Z/X.

Anote **ok / falhou / observação** em cada linha e me mande o quadro no fim.

## 0. Antes de começar
| # | Passo | Esperado |
|---|---|---|
| 0.1 | Carregar a ROM como ASCII8 de 256 KiB | Carrega sem erro de mapper |
| 0.2 | Conferir que a **saída de vídeo é a do V9990** (no openMSX: `set videosource GFX9000`) | Ver o mapa; **não** uma tela azul de texto |
| 0.3 | (Opcional) Abrir **sem** o GFX9000 | Texto "NEEDS A V9990 (GFX9000). NONE FOUND ON THIS MSX." |

## 1. Boot e imagem (1 min)
| # | Passo | Esperado |
|---|---|---|
| 1.1 | Esperar o carregamento (barra de progresso, alguns segundos) | Chega ao mapa com castelo, 4 camponeses, rodapé com WOOD/GOLD/TIME |
| 1.2 | Olhar o mapa parado | Sem tremor ou faixas; cores normais; neblina escura nas áreas desconhecidas |
| 1.3 | Rolar o mapa com as **setas** (ou encostando na borda) | Rola suave, sem rasgos; o rodapé não treme |

## 2. Teclado (1 min) — foco do que ainda não foi testado em BIOS real
| # | Passo | Esperado |
|---|---|---|
| 2.1 | **I/J/K/L** | O cursor se move |
| 2.2 | **Z** sobre um camponês | Seleciona (anel e barra de vida aparecem; HUD mostra o camponês) |
| 2.3 | **A** | Seleciona todos |
| 2.4 | **X** no chão | Unidades andam até lá |
| 2.5 | **S** (parar) e **H** (manter posição) | Respondem uma vez por toque, sem repetir sozinhas |
| 2.6 | Segurar uma tecla (ex.: seta) e soltar | Não fica "presa" depois de soltar |

> Se algo do teclado repetir sozinho, falhar ou ficar preso: me diga. A causa provável é a linha `*((volatile u8*)0xF3F6)=255;` em `irq_p1` (`src/video.h`); basta removê-la.

## 3. Mouse (1 min)
| # | Passo | Esperado |
|---|---|---|
| 3.1 | Mover o mouse | A seta acompanha, sem pulos |
| 3.2 | Clique esquerdo curto em um camponês | Seleciona (um clique só, sem duplo) |
| 3.3 | Arrastar com o esquerdo | Caixa branca de seleção; ao soltar, seleciona os de dentro |
| 3.4 | Clique direito no chão e em uma árvore | Move / manda coletar madeira |
| 3.5 | Mouse na outra porta, ou **M** para redetectar | O jogo passa a usar o mouse |

## 4. Jogo e desempenho (2 min)
| # | Passo | Esperado |
|---|---|---|
| 4.1 | Selecionar os 4 camponeses (A) e mandar para longe | Andam fluidos; sem tremor; o relógio (TIME) avança ≈ 1 s por segundo real |
| 4.2 | Deixar coletar madeira e ouro um pouco | WOOD e GOLD aumentam; camponeses entram na mina e saem |
| 4.3 | Construir uma casa (rodapé HOU, ou **Q**, depois Z no terreno) | Fantasma verde/vermelho; obra com barra de progresso |
| 4.4 | Sentir a fluidez | Alvo: ≈ 30 quadros/s com poucas unidades (openMSX a 3,58 MHz: 29,7) |

## 5. O que me enviar
1. Plataforma e versão (ex.: "blueMSX 2.8.3", "MAME 0.264", "MSX2 Sony HB-F1XDJ + GFX9000").
2. O quadro das tabelas acima (ok/falhou).
3. Impressão de velocidade em uma frase ("rápido", "um pouco lento", "muito lento").
4. Se der: uma foto ou captura de tela do mapa com as unidades, e a do rodapé.
