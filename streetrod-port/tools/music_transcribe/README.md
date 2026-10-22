# Transcrição do tema (áudio → linhas de sequenciador)

Ordem (rodar dentro desta pasta, com `in.wav` = gravação em mono 22,05 kHz: `ffmpeg -i musica.mp3 -ac 1 -ar 22050 in.wav`):
1. `grid.py` — acha o passo (≈0,145 s) e a fase da grade por força de onset.
2. `an2.py <offset> <passo>` — gera `cqt36.npy`/`times36.npy` (CQT de 36 bins/oitava) e imprime as notas dominantes por passo.
3. `loop.py` — descobre o laço (média das duas repetições) e escolhe até 4 candidatos por passo (remoção de harmônicos, filtro de vazamento) → `loop.json`.
4. `assign.py` — distribui em 3 vozes (melodia/baixo/harmonia) → `theme_notes.json`; `msx2/road/theme_data.py` é derivado dele.
Os valores (`STEP`, `OFF`, `S0`, `N`, `LOOP`) estão no topo de `loop.py`; ajuste-os para outra música. Dependências: `librosa`, `numpy`. Ver `PORTING_GUIDE.md` §10.3.
