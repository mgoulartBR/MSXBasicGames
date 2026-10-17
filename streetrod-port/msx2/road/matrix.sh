#!/bin/sh
# Mede varias configuracoes. Cada linha: descricao|flags
cd "$(dirname "$0")"
while IFS='|' read -r name flags; do
  [ -z "$name" ] && continue
  r=$(./bench.sh "$flags" | sed 's/.*RESULT //' | sed 's/ speed.*//')
  echo "$name | $r"
done
