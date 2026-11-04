#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh; O=../v60_olcekleme/ham
A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_10001#")
for m in 0 1 0 1; do env V54_SPARSEQ=$m V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 python3 calistir.py $O/sq${m}_$RANDOM ../cpp/tek_hamle_v60 $A >/dev/null 2>&1; echo "sq $m $(( $(date +%s) - $(cat /tmp/v60_t0) )) s" >> $O/ilerleme3.txt; done
