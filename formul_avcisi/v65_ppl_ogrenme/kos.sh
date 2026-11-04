#!/bin/bash
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh; O=../v65_ppl_ogrenme/ham
run() { v=$1; m=$2; shift 2; A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_$v#"); env "$@" V65_EVAL=1 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1 python3 calistir.py $O/${m}$v ../cpp/tek_hamle_v65 $A >/dev/null 2>&1; echo "$m$v rc=$? $(( $(date +%s) - $(cat /tmp/v65_t0) )) s" >> $O/ilerleme.txt; }
run 5001 B V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V65_CHECK=1
run 10001 B V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V65_CHECK=1
run 2001 A V63_ARROW=2
run 5001 A V63_ARROW=2
run 10001 A V63_ARROW=2
