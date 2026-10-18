# Street Rod MSX2 — desenho do jogo completo

Fonte: strings e dados de `SRSE.EXE`/`HOT_DATA` (ver `NOTES.md`). O original (1989, DOS) é um simulador de "hot rod" no verão de 1963 (calendário de
agosto/setembro): você compra e conserta carros velhos, troca peças, desafia pilotos no Bob's Drive-In (arrancada, corrida de estrada, "só por diversão"
ou apostando o *pink slip*) e tenta virar o **King of the County**. O jogo acaba se o dinheiro/tempo (verão) acabarem.

## Fluxo do jogo original (a partir das strings)
- **Título:** New Game / Old Game (Save/Load). Pede o nome do jogador (`What's his name?`).
- **Garagem** (cena `LIB2` item 32): *Your cars* (até N carros), *Car parts* (peças instaladas), *Spare parts* (peças avulsas).
  Ações: *Switch it* (trocar de carro), *Sell it*, *Fix it*/*Junk it* (carro quebrado: conserto ou sucata), *Change tires/transmission*, *Pop the hood*
  (motor/carburador/coletor), *Chop roof* ($70) / *Rebuild roof*, *Strip bumper*/*Restore bumper* ($15), *New paint job* ($20), *Stickers* ($5),
  *Car info*, *Calendar*, *Wanna tank?* (gasolina), *Catch some tunes* (rádio), *Time to quit*.
- **Rua (Hit the street):** dirigir pela cidade (`Cruising in town...`, consumo de gasolina, multa por excesso de velocidade, `Max speed`) até o **Bob's**.
- **Jornal (Check out the newspaper):** *Used Cars* e *Auto Parts* (catálogo de 43 peças com preço: motores GM/Ford/Chrysler, câmbios, carburadores,
  coletores, pneus). Compra/venda com negociação (`How about $X?`, `Will you take $X?`, `Sell parts`).
- **Bob's Drive-In:** *Look for opponent* / *Click to challenge*. Modos: **Drag race**, **Road race**, **Just for kicks**, **Pink slips**. Apostas de $10/25/50/100.
  O oponente aceita ou recusa conforme habilidade e dinheiro (tabelas do bloco 9/10/11 do `HOT_DATA`); pilotos Mike…The King (22).
- **Corrida:** resultado com premiação; pode quebrar motor/câmbio/pneus, perder o carro (pink slip), "Lucky you weren't killed, the car is totaled".
- **Fim:** acabou o dinheiro (`You're outta dough`), acabou o verão (`You're done fer the summer`), ou `You deserve to be King of the County!`.

## Marcos de implementação (MSX2)
| # | Marco | Conteúdo |
|---|---|---|
| M0 | **Pronto** | Corrida (física original, cockpit, ponteiros, oponente, semáforo, som), menu de teste, ROM ASCII16 de 64 KB |
| M1 | **Pronto** — Estado do jogo + hub | Dinheiro, data, carros possuídos; tela da garagem (arte original) com menu: garagem, rua, jornal, Bob's |
| M2 | Jornal | Carros usados (25) e catálogo de peças (43) com preços, compra/venda, limites de espaço |
| M3 | Garagem | Trocar de carro, vender, instalar peças (recalcula o *powertrain* por carro+peças), pintura/adereços, gasolina |
| M4 | Bob's | Escolha do oponente (22 pilotos), tipos de corrida, apostas, pink slip, aceita/recusa; corrida real |
| M5 | Consequências | Quebra de motor/câmbio/pneus, perda de carro, prêmios, calendário e fim de jogo |
| M6 | Rua | Cruzeiro até o Bob's com consumo de gasolina, multas |
| M7 | Acabamento | Telas do `LIB1`/`LIB2`, ranking, música, ajuste da pista de corrida de estrada |

## Restrições técnicas
- ROM ASCII16 (múltiplos de 16 KB); código ≤ 32 KB fixo, assets em bancos 2+ carregados pelo carregador da página 1.
- Estado do jogo em RAM; salvar/carregar (disco ou SRAM) fica para depois.
- Entrada: setas + ESPAÇO + SHIFT (e joystick); o mouse do original vira cursor de seleção em menus.
