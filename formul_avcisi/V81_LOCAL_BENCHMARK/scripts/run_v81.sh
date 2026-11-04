#!/bin/bash
# run_v81.sh <ikili> <veri_dizini> [ek ORTAM=DEGER ...]  -> stdout: program ciktisi; son satir: TH_WALL_SECONDS=...
set -e
HERE="$(cd "$(dirname "$0")" && pwd)"; source "$HERE/env_v81.sh"
BIN="$1"; DATA="$2"; shift 2
s=$(date +%s.%N)
env "$@" "$BIN" $TH_ARGS_ON "$DATA" $TH_ARGS_SON
e=$(date +%s.%N)
awk -v a="$s" -v b="$e" 'BEGIN{printf "TH_WALL_SECONDS=%.3f\n", b-a}'
