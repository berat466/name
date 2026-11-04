#!/bin/bash
# A/B: V53 (897727b, tek_hamle_v53h hizli-tam) vs V54 varsayilan; 1 isinma + 5 tur ara ara; her koşu icin rusage
cd "$(dirname "$0")"; OUT=ham/ab54; mkdir -p $OUT
for cfg in full hafif; do source env_$cfg.sh
  V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS >/dev/null 2>&1; V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 ../cpp/tek_hamle_v54 $ARGS >/dev/null 2>&1
  for r in 1 2 3 4 5; do
    env V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_v53_$r ../cpp/tek_hamle_v53h $ARGS
    env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${cfg}_v54_$r ../cpp/tek_hamle_v54 $ARGS
  done
done
touch $OUT/DONE
