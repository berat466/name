#!/bin/bash
# is parcacigi olcekleme: OMP ve OpenBLAS ayni sayi; 3 tur
cd "$(dirname "$0")"; BIN=${1:-../cpp/tek_hamle_v53h}; CFG=${2:-full}; OUT=${3:-ham/olcek_v53}; MODE=${4:-"V53H_FAST=1 V53H_U=0"}; mkdir -p $OUT; source env_$CFG.sh
for T in 1 2 4 8 16; do for r in 1 2 3; do
  env OMP_NUM_THREADS=$T OPENBLAS_NUM_THREADS=$T V53H_LEAN=1 $MODE V53H_TEST=0 python3 calistir.py $OUT/${CFG}_T${T}_$r $BIN $ARGS; done; done
touch $OUT/DONE
