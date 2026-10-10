#!/bin/bash
# Compila a logica do jogo (src/sim.c + dados) no PC e roda o autoplay de balanceamento.
cd "$(dirname "$0")/.."
mkdir -p out
gcc -O1 -Wall -Wno-unused -DHOST_BUILD $CFLAGS_EXTRA -I tests/host -I src -o out/balance tests/balance.c src/sim.c src/sim_ext.c src/studio.c src/db.c src/seg/seg_s3_b3.c src/seg/seg_s4_b3.c || exit 1
./out/balance "$@"
