#!/bin/bash
# valid.pma.in + ops.h tablolari (opstab) -> valid.pma (doldur) -> valid.pmb. awk yok: yalniz oz/arac araclari.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
OPS=${OPS_H:-../../src/ops.h}
{ arac opstab < $OPS; echo "@@SABLON"; cat valid.pma.in; } | arac doldur > valid.pma || exit 1
arac_asm valid.pma valid.pmb
