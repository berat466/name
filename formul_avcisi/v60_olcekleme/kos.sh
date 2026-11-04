#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh
O=../v60_olcekleme/ham
for v in 2001 5001 10001; do
  A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#")
  for m in sp dn; do
    SPK=1; [ $m = dn ] && SPK=0
    D=""; [ $v != 10001 ] && { mkdir -p /tmp/v60d_${m}_$v; D="V53H_DUMP=/tmp/v60d_${m}_$v"; }
    env V60_SPK=$SPK V53H_HASH=1 $D V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $O/${m}$v ../cpp/tek_hamle_v60 $A >/dev/null 2>&1
    echo "$m $v rc=$? $(( $(date +%s) - $(cat /tmp/v60_t0) )) s" >> $O/ilerleme.txt
  done
done
