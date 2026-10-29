#!/bin/bash
# valid.pma.in + kanonik komut tablosu (komut @@tab) -> valid.pma (doldur) -> valid.pmb. awk yok: yalniz oz/arac araclari.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu (src/ops.h degil)
{ { echo "@@tab"; cat $OPT; } | arac komut; echo "@@SABLON"; cat valid.pma.in; } | arac doldur > valid.pma || exit 1
arac_asm valid.pma valid.pmb
