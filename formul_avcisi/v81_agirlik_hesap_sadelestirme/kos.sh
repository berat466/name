#!/bin/bash
# kos.sh <etiket> <ikili> [ek ortam...]  -- V78 tum hizli yol bayraklari acik; 10.001; tum program
cd /home/user/name/formul_avcisi/v54_baglantili_hesaplama; source env_hafif.sh
et=$1; bin=$2; shift 2
A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#${DATA:-/tmp/v59_10001}#")
env V75_STREAM=1 V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V72_UCSC=1 V73_RHS=1 V73_SYRK=1 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1 "$@" python3 calistir.py ../v81_agirlik_hesap_sadelestirme/ham/$et ../cpp/$bin $A >/dev/null 2>&1
f=../v81_agirlik_hesap_sadelestirme/ham/$et
echo "$et: $(cut -c1-70 $f.time | sed 's/VCSW.*//') hazir=$(grep -oE 'MODEL_HAZIR\|[0-9.]+' $f.txt | cut -d'|' -f2) $(grep -oE 'V53H_SONUC\|TEST CE=[0-9.]+ PPL=[0-9.]+' $f.txt) $(grep -oE 'VAL CE=[0-9.]+' $f.txt)"
