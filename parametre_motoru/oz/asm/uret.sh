#!/bin/bash
# asm.pma.in (+ ops.h'den ad/komut tablolari) -> asm.pma -> asm.pmb. Bu betik yalnizca bicim donusumu yapar.
cd "$(dirname "$0")"; PM=${1:-../../pm}
./tablolar.sh > tablolar.txt || exit 1
printf 'data KWS = %s\n' "$(printf 'memory\0entry\0data\0space\0func\0gt\0ge\0fgt\0fge\0main\0' | od -An -v -tu1 | tr -s ' \n' ' ')" >> tablolar.txt
awk 'FNR==NR{t=t $0 "\n"; next} /@TABLOLAR@/{printf "%s", t; next} {print}' tablolar.txt asm.pma.in > asm.pma || exit 1
if [ -n "$ASM2" ]; then $ASM2 asm.pma asm.pmb; else $PM asm asm.pma -o asm.pmb --strip; fi
