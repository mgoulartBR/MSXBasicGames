# Roteiro de teste em MSX2 real (ou flash cart)

Nada do projeto foi testado em hardware real ate a 2.2: tudo foi verificado no openMSX (C-BIOS). Este roteiro diz o que conferir
e o que anotar. A tela **Escritorio > Options** mostra, no rodape, um bloco "This MSX": 50/60 Hz, FM ou so PSG, bit da SRAM e modo do mouse.
Anote esse bloco junto com o resultado.

Requisitos: MSX2 (ou MSX2+/turboR) com 128 KB de VRAM; cartucho/flash cart com mapper **ASCII8 e SRAM** de 8 KB para a ROM de 512 KB
(sem SRAM o jogo funciona, mas salva so por codigo). Mouse na porta 1 (opcional), MSX-MUSIC (opcional).

| # | Teste | Esperado |
|---|-------|----------|
| 1 | Ligar | Abertura (TV "MAD TV") e, ao apertar uma tecla, o titulo "Select a player" |
| 2 | Titulo > New game, andar de elevador com as setas | Predio navegavel, sem piscar; relogio no cabecalho |
| 3 | Escritorio > Options | Bloco "This MSX": frequencia correta (PAL 50 / NTSC 60), FM so se houver MSX-MUSIC, SRAM com um bit (10h/20h/40h/80h) |
| 4 | Options: desligar efeitos, trocar velocidade do mouse; desligar o MSX e ligar de novo | As opcoes voltam (so com SRAM) |
| 5 | Escritorio > Save/Load > Save to slot 1; desligar; Esc no titulo | Slot 1 aparece com dia/caixa e carrega; "Erase a slot" apaga com 2 OKs |
| 6 | Sem SRAM: Save/Load abre a tela do codigo (140 caracteres) | Codigo aparece; digitar de volta no titulo (Esc) restaura |
| 7 | Mouse (se houver): mover, clicar nas portas, botao direito = voltar | Cursor segue o mouse; clique entra na sala |
| 8 | Musica: com FM deve soar flauta/baixo/violao; sem FM, PSG | Ritmo ~96 BPM; nao precisa ser agradavel, so nao travar |
| 9 | Jogar um dia inteiro em velocidade 3 (TAB) | Dia termina, pausa com mensagem; nenhuma tela trava |
| 10 | Final: casar com a Betty (ou falir) | Tela final com cena e resumo; OK reinicia |

Em caso de falha anote: modelo do MSX, flash cart/mapper, o numero do teste e o bloco "This MSX".
