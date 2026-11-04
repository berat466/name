#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh; O=../v64_kesintisiz/ham
for v in 2001 5001 10001; do A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#")
 for m in a1 b1 a2 b2; do
  case $m in a*) B=tek_hamle_v63; E="V63_ARROW=2";; b*) B=tek_hamle_v64; E="V63_ARROW=2 V64_IMPL=1 V64_SPQ=1";; esac
  env $E V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $O/r${v}_$m ../cpp/$B $A >/dev/null 2>&1; echo "$v $m rc=$? $(( $(date +%s) - $(cat /tmp/v64_t0) )) s" >> $O/ilerleme.txt
 done; done
