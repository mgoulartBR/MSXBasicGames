#!/bin/bash
# Compila a logica do jogo (src/sim.c + dados) no PC e roda o autoplay de balanceamento.
cd "$(dirname "$0")/.."
mkdir -p out
gcc -O1 -Wall -Wno-unused $CFLAGS_EXTRA -I tests/host -I src -o out/balance tests/balance.c src/sim.c src/data/db_data_s3_b3.c || exit 1
./out/balance "$@"
