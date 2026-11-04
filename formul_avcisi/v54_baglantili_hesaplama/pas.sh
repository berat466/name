#!/bin/bash
# varsayilan vs OMP_WAIT_POLICY=passive vs 16 is parcacigi: 5 tur ara ara, iki konfigurasyon
cd "$(dirname "$0")"; OUT=ham/pas; mkdir -p $OUT
for cfg in full hafif; do source env_$cfg.sh
  for r in 1 2 3 4 5; do
    env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_varsayilan_$r ../cpp/tek_hamle_v54 $ARGS
    env OMP_WAIT_POLICY=passive V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_passive_$r ../cpp/tek_hamle_v54 $ARGS
    env OMP_NUM_THREADS=16 OPENBLAS_NUM_THREADS=16 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_T16_$r ../cpp/tek_hamle_v54 $ARGS
    env OMP_NUM_THREADS=16 OPENBLAS_NUM_THREADS=16 OMP_WAIT_POLICY=passive V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_T16passive_$r ../cpp/tek_hamle_v54 $ARGS
  done
done
touch $OUT/DONE
