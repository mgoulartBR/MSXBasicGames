# Checklist de upload — Keen Dreams MSX2 0.9.103 (itch.io)

## 1. Página do jogo → Edit game → Uploads
1. Remova (ou marque como "Hide"/"Deprecated") `KEEN-0.9.98.ROM`, `KEEN-0.9.98-fontes.zip` e, se existirem, os da 0.9.99.
2. Envie de `1-arquivos-do-jogo/`:
   - `KEEN-0.9.103.ROM` — ASCII16, 2 MiB, **requer MSX2+** (SHA-256 `10e4e7068843826d41b4539e711b5af36f94fee82ece9945e0e8f97bb2087aee`)
   - `KEEN-0.9.103-fontes.zip` — fontes completas; compilar dá a mesma ROM (SHA-256 `b00309b7de012d59317d73e04dd584a4318c1febbae50f0f051be471f07267e4`)
   - `SHA256SUMS.txt` (opcional)
3. Em "Kind of project"/plataforma, mantenha como já estava (download). Na descrição da página, troque a menção de versão e acrescente "Requires MSX2+".

## 2. Devlog → Dashboard → Devlog → Create new post
- Título, tipo (Major Update or Launch), tags e idioma: no cabeçalho de `2-devlog/DEVLOG-POST.md` (copie o que está **abaixo da linha `(Content abaixo — Markdown)`** para o corpo; o cabeçalho acima não vai no post).
- Capa: `2-devlog/imagens/cover-16x9.png` (1280×720).
- Imagens do corpo: arraste cada arquivo de `2-devlog/imagens/` para o editor e troque os links `![...](imagens/...)` pelas imagens enviadas (o itch.io hospeda as imagens no próprio editor). Ordem no texto: `fps-0.9.99-vs-0.9.103-5.37MHz`, `fps-delta-0.9.99-vs-0.9.103`, `fps-0.9.75-vs-0.9.103-5.37MHz`, `fps-z80-1x-3.58MHz`, `fps-z80-2x-7.16MHz`, `fps-z80-3x-10.74MHz`, `fps-r800-7.16MHz`.
- Comentários: habilitados. Visibilidade: Published.

## 3. Comentário do byturn
Texto pronto em `3-comentario/RESPOSTA-BYTURN.md` (em inglês; copie só o trecho do `>`, sem os `>`).

## 4. Conferência
```
cd 1-arquivos-do-jogo && sha256sum -c SHA256SUMS.txt
```
Só openMSX; nenhum teste em hardware real (está dito no devlog).
