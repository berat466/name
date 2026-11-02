#!/bin/bash
# PMX1 sayi listesi -> PMA metni (+ ';;PMT1' tur notu satiri), tamamen SAYISAL zincir: ifx.pmb + komutlar.pmk (bolum 2/3/12) + OPSIG dokumu (komut.pmb @@imza).
# Kullanim: ifx.sh PM PMX1_DOSYASI [TABLO]   (stdout: PMA)
PM=$1; X=$2; D=$(cd "$(dirname "$0")" && pwd); TAB=${3:-$D/../../komut/komutlar.pmk}
IM=${IMZA:-$({ echo "@@imza"; cat $TAB; } | (ulimit -v 3000000; $PM run $D/../../arac/komut.pmb) | tr '\n' ' ')}
{ cat $TAB; echo "$IM"; cat $X; } | (ulimit -v 3000000; $PM run ${IFX:-$D/ifx.pmb})
