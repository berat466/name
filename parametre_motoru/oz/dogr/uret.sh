#!/bin/bash
# valid.pma.in + tablolar.txt -> valid.pma -> valid.pmb (yalniz bicim donusumu). ASM2 tanimliysa "$ASM2 girdi.pma cikti.pmb" ile derler (pm'siz yol).
cd "$(dirname "$0")"; PM=${1:-../../pm}
awk 'FNR==NR{t=t $0 "\n"; next} /@TABLOLAR@/{printf "%s", t; next} {print}' tablolar.txt valid.pma.in > valid.pma || exit 1
if [ -n "$ASM2" ]; then $ASM2 valid.pma valid.pmb; else $PM asm valid.pma -o valid.pmb --strip; fi
