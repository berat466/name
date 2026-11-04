#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_full.sh
for v in 2001 5001 6722; do
  A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v58_$v#"); mkdir -p /tmp/v58d_$v
  env V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 V53H_DUMP=/tmp/v58d_$v timeout 120 python3 calistir.py ../v58_olcekleme/ham/d$v ../cpp/tek_hamle_v54 $A > /dev/null 2>&1
  echo "dump $v rc=$? $(( $(date +%s) - $(cat /tmp/v58_t0) )) s" >> ../v58_olcekleme/ham/ilerleme2.txt
done
