#!/bin/bash
# V54 ablasyon (tam konfigurasyon): her satir TEK ozelligi kapatir; 3 tur ara ara; model hazir + CPU + RSS
cd "$(dirname "$0")"; OUT=ham/abl54; mkdir -p $OUT; source env_full.sh
run() { name=$1; shift; for r in 1 2 3; do env "$@" V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${name}_$r ../cpp/tek_hamle_v54 $ARGS; done; }
env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 ../cpp/tek_hamle_v54 $ARGS > /dev/null 2>&1
for rr in 1 2 3; do
 for v in "tam:" "tanh_kapali:V54_TANH=0" "kapi_kapali:V54_GATE=0" "utab_kapali:V54_UTAB=0" "topk_kapali:V54_TOPK=0" "reuse_kapali:V54_REUSE=0" "sparseq_kapali:V54_SPARSEQ=0" "U_yogun:V53H_U=0" "siki:V54_TANH=0 V54_GATE=0 V54_TOPK=0 V54_REUSE=0 V54_SPARSEQ=0 V53H_U=0"; do
   n=${v%%:*}; e=${v#*:}; env $e V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $OUT/${n}_$rr ../cpp/tek_hamle_v54 $ARGS; done
done
touch $OUT/DONE
