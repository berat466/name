#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh; O=../v63_kuresel/ham
for v in 2001 5001 10001; do A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#")
 for m in a b c; do
  case $m in a) B=tek_hamle_v61; E="V63_ARROW=0";; b) B=tek_hamle_v63; E="V63_ARROW=1";; c) B=tek_hamle_v63; E="V63_ARROW=2";; esac
  D=""; if [ $v = 5001 ]; then mkdir -p /tmp/v63e_$m; D="V53H_DUMP=/tmp/v63e_$m"; fi
  env $E $D V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $O/r${v}_$m ../cpp/$B $A >/dev/null 2>&1; echo "$v $m rc=$? $(( $(date +%s) - $(cat /tmp/v63_t0) )) s" >> $O/ilerleme.txt
 done; done
