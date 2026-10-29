#!/bin/bash
# asm.pma.in + kanonik komut tablosu (komut @@tab) -> asm.pma (doldur) -> asm.pmb. awk/sed/od yok: yalniz oz/arac araclari. ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPT=${OPT:-../komut/komutlar.pmk}   # KANONIK sayisal komut tablosu (src/ops.h degil)
{ { echo "@@tab"; cat $OPT; } | arac komut; echo "@@SABLON"; cat asm.pma.in; } | arac doldur > asm.pma || exit 1
arac_asm asm.pma asm.pmb
