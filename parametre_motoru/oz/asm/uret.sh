#!/bin/bash
# asm.pma.in + ops.h tablolari (opstab) -> asm.pma (doldur) -> asm.pmb. awk/sed/od yok: yalniz oz/arac araclari. ARAC (ELF) ya da PM (onyukleme) gerekir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPS=${OPS_H:-../../src/ops.h}
{ arac opstab < $OPS; echo "@@SABLON"; cat asm.pma.in; } | arac doldur > asm.pma || exit 1
arac_asm asm.pma asm.pmb
