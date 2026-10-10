#!/bin/bash
# Testes de unidade da logica do jogo (compilada no PC)
cd "$(dirname "$0")/.."
mkdir -p out
gcc -O1 -Wall -Wno-unused -DHOST_BUILD -I tests/host -I src -o out/unit tests/saveload_test.c src/sim.c src/sim_ext.c src/studio.c src/db.c src/seg/seg_s3_b3.c src/seg/seg_s4_b3.c || exit 1
./out/unit
