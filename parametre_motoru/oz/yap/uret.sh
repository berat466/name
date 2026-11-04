#!/bin/bash
# yap.pma.in + bicim tablosu (fmtc: fmt.txt) -> yap.pma (doldur) -> yap.pmb. awk/sed yok: yalniz oz/arac araclari.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ../arac/arac.sh
{ arac fmtc < fmt.txt; echo "@@SABLON"; cat yap.pma.in; } | arac doldur > yap.pma || exit 1
arac_asm yap.pma yap.pmb
