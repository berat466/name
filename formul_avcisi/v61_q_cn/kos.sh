#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh; O=../v61_q_cn/ham
run() { # ad ikili v extra...
  local ad=$1 bin=$2 v=$3; shift 3; local A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#")
  env "$@" V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $O/$ad ../cpp/$bin $A >/dev/null 2>&1; echo "$ad rc=$? $(( $(date +%s) - $(cat /tmp/v61_t0) )) s" >> $O/ilerleme.txt; }
for v in 2001 5001; do for b in v60 v61; do mkdir -p /tmp/v61d_${b}_$v; run ${b}_$v tek_hamle_$b $v V53H_HASH=1 V53H_DUMP=/tmp/v61d_${b}_$v; done; done
for b in v60 v61; do run ${b}_10001h tek_hamle_$b 10001 V53H_HASH=1; done
for r in 1 2; do for b in v60 v61; do run ${b}_10001_$r tek_hamle_$b 10001 X=1; done; done
