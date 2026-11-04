#!/bin/bash
# Ablasyon (tam konfigurasyon): her satir tek bir hizlandirma farkini kapatir; 3 tur, ARA ARA
cd "$(dirname "$0")"; mkdir -p ham/abl; source env_full.sh
H="V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0"
env $H ../cpp/tek_hamle_v53h $ARGS > /dev/null 2>&1
for r in 1 2 3; do
  env $H ../cpp/tek_hamle_v53h $ARGS > ham/abl/tam_$r.txt 2>&1
  env $H V53H_DENSEY0=1 ../cpp/tek_hamle_v53h $ARGS > ham/abl/yogunY0_$r.txt 2>&1
  env $H V53H_NOAVX=1 ../cpp/tek_hamle_v53h $ARGS > ham/abl/avxyok_$r.txt 2>&1
  env $H V53H_DENSEY0=1 V53H_NOAVX=1 ../cpp/tek_hamle_v53h $ARGS > ham/abl/yogunY0_avxyok_$r.txt 2>&1
done
touch ham/abl/DONE
