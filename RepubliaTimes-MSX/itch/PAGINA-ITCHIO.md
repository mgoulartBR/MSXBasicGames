# Texto da página do itch.io — The Republia Times (MSX) v0.4

## Configurações sugeridas
- **Title:** The Republia Times (MSX)
- **Project URL:** republia-times-msx
- **Short description / tagline:** Port não oficial para MSX1 do jogo de Lucas Pope — dirija o jornal de um regime e decida o que o povo lê.
- **Kind of project:** Downloadable
- **Classification:** Games
- **Release status:** In development (Beta)
- **Pricing:** No payment (grátis). Não cobre/aceite doações enquanto não houver permissão do autor.
- **Genre:** Simulation / Strategy
- **Tags:** msx, retro, homebrew, port, fan-port, 8-bit, z80, newsgame, lucas-pope, msxgl
- **Platforms:** marque apenas "Other" (ROM de MSX; roda em openMSX, blueMSX, MSX real com cartucho)
- **Uploads:** `RepubliaTimes-MSX-v0.4-itch.zip` (marcar como "Other"/Downloadable)
- **Capa:** `page/cover_630x500.png` — **Screenshots:** `page/shot1…shot6`
- **Community:** comentários ligados

## Descrição (PT-BR)

**The Republia Times (MSX)** é um port **não oficial** do jogo de Lucas Pope (2013) para MSX1, feito em C com MSXgl.

Você é o editor do jornal da nação de Republia. Escolha as notícias da manhã, monte a edição, e veja o que acontece com a lealdade e a audiência do povo. Todas as regras do original foram portadas 1:1: geração dos dias, cálculo de audiência, metas, caminho dos rebeldes e final.

### Controles
- **Teclado:** setas = mover · ESPAÇO = confirmar/avançar · outras teclas indicadas na tela
- **Joystick:** direcional + botão A / B
- **Mouse MSX:** passe o cursor para selecionar · botão esquerdo = A · botão direito = B (detectado automaticamente no título)

### Requisitos
- MSX1 (ou superior) com cartucho de **ROM mapeada de 64 KB**
- `…-ASCII8.rom` → mapper ASCII8 (USB-Mapper, openMSX, blueMSX)
- `…-Konami.rom` → mapper Konami (sem SCC)
- Em emulador: openMSX / blueMSX, escolha o tipo de ROM correspondente

### Status honesto
- Testado extensivamente em **emulador** (openMSX com C-BIOS, 50 Hz).
- Testes em MSX real (Sony HB-101 com USB-Mapper/PicoVerse) reportaram **imagens embaralhadas e travamentos** nas versões anteriores. A v0.4 traz reforços (autoteste de mapper no boot, tela de MAPPER ERROR), mas **ainda não está confirmada em hardware real**. Feedback é muito bem-vindo nos comentários (informe modelo do MSX, cartucho/flash e qual ROM usou).
- Áudio: música e efeitos pelo PSG.

### Créditos
- Jogo original: **Lucas Pope** — *The Republia Times*
- **MSX port by BigFive Studios**
- Biblioteca: MSXgl (CC BY-SA 4.0) · Compilador SDCC
- Este é um projeto de fã, sem afiliação com o autor original.

## Description (EN)

**The Republia Times (MSX)** is an **unofficial** MSX1 port of Lucas Pope's 2013 game, written in C with MSXgl.

Run the newspaper of the nation of Republia: pick the morning stories, lay out the edition, and watch how it shifts the people's loyalty and readership. Game rules are ported 1:1 from the original.

**Controls:** keyboard (arrows, SPACE), joystick, or MSX mouse (auto-detected on the title screen; left = A, right = B).
**Requirements:** MSX1+ with a 64 KB mapped-ROM cartridge (ASCII8 or Konami mapper), or openMSX/blueMSX.
**Status:** extensively tested in emulators; real-hardware reports (Sony HB-101) showed garbled graphics/hangs on earlier versions and v0.4 is not yet confirmed on real machines. Feedback welcome.
**Credits:** original game by Lucas Pope · MSX port by BigFive Studios · MSXgl (CC BY-SA 4.0).

## Changelog v0.4
- Navegação suave: textos não são mais redesenhados a cada movimento (só cor da seleção; painel com debounce)
- Suporte a mouse com ponteiro e textos "clique"
- Autoteste de mapper no boot + variante Konami
- Rotina de interrupção reescrita (preserva todos os registradores)
- Zeragem de RAM não inicializada
