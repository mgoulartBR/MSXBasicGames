#!/bin/sh
set -eu
cd "$(dirname "$0")"
if command -v node >/dev/null 2>&1; then
  NODE=node
else
  NODE=../../tools/build/Node/node
fi
"$NODE" ../../engine/script/js/build.js "$@"
