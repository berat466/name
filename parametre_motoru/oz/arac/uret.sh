#!/bin/bash
# oz/arac araclarini (doldur, opstab, fmtc, sablonc, komut) .pma kaynaklarindan derler. ASM2 (asm.elf sarmalayicisi) ya da PM (pm asm) kullanir.
cd "$(dirname "$0")"; PM=${1:-${PM:-../../pm}}; . ./arac.sh
for t in doldur opstab fmtc sablonc komut; do arac_asm $t.pma $t.pmb || exit 1; done
