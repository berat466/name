#!/bin/bash
# Zaman olcumu: her konfigurasyon icin 1 isinma + N tur, 'once' (orijinal aritmetik yol, lean) ve 'sonra' (hizli-tam) ARDISIK/ARA ARA calistirilir
cd "$(dirname "$0")"; N=${1:-5}; mkdir -p ham/olc
for cfg in full hafif; do source env_$cfg.sh
  V53H_LEAN=1 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > /dev/null 2>&1
  V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > /dev/null 2>&1
  for r in $(seq 1 $N); do
    V53H_LEAN=1 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > ham/olc/${cfg}_once_$r.txt 2>&1
    V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > ham/olc/${cfg}_sonra_$r.txt 2>&1
    V53H_LEAN=1 V53H_FAST=1 V53H_U=1 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > ham/olc/${cfg}_sonraU1_$r.txt 2>&1
  done
  V53H_LEAN=1 V53H_SPLIT=1 V53H_TEST=0 ../cpp/tek_hamle_v53h $ARGS > ham/olc/${cfg}_once_split.txt 2>&1
done
touch ham/olc/DONE
