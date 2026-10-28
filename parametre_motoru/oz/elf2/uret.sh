#!/bin/bash
# gen2.pma.in + ops.h tablolari (opstab) + x86 sablonlari (sablonc: sablon.txt) + sabit veri (statik.sozluk) -> gen2.pma (doldur) -> gen2.pmb.
# awk/sed/od/tr yok: yalniz oz/arac araclari. ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPS=${OPS_H:-../../src/ops.h}
{ arac opstab < $OPS; { cat $OPS; echo "@@SABLON"; cat sablon.txt; } | arac sablonc; cat statik.sozluk; echo "@@SABLON"; cat gen2.pma.in; } | arac doldur > gen2.pma || exit 1
arac_asm gen2.pma gen2.pmb
