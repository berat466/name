#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh
for v in 2001 5001 10001; do
  A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#")
  env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 timeout 200 python3 calistir.py ../v59_genisleyebilir/ham/A$v ../cpp/tek_hamle_v54 $A > /dev/null 2>&1
  echo "A $v rc=$? $(( $(date +%s) - $(cat /tmp/v59_t0) )) s" >> ../v59_genisleyebilir/ham/ilerleme.txt
done
