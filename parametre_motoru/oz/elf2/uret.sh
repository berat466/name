#!/bin/bash
# gen2.pma.in + sablon.txt (+ ops.h'den komut tablolari, elf/T.txt'den puti/putc bayt bloklari) -> gen2.pma -> gen2.pmb
# Bu betik yalnizca sayi/bicim donusumu yapar; makine kodu mantigi gen2.pma ve sablon.txt icindedir.
cd "$(dirname "$0")"; PM=${1:-../../pm}
NOPS=$(grep -E '^  X\(' ../../src/ops.h | sed -E 's/.*X\([A-Z0-9]+, *"[a-z0-9]+", *([0-9]).*/\1/' | tr '\n' ' ')
MSGS=$(printf 'TRAP: DIV0\nTRAP: OOB\nTRAP: STACK\nTRAP: DEPTH\nTRAP: ALLOC\n' | od -An -v -tu1 | tr -s ' \n' ' ')
TN=$(tr ' ' '\n' < ../elf/T.txt | sed 's/^z//' | grep -v '^$')
blob() { echo "$TN" | sed -n "$(( $1 + 1 )),$(( $1 + $2 ))p" | awk '{printf "%d ", ($1<0?256+$1:$1)}'; }
awk -f sablon.awk ../../src/ops.h sablon.txt > tablolar.txt || exit 1
awk -v nops="$NOPS" -v msgs="$MSGS" -v puti="$(blob 494 82)" -v putc="$(blob 576 32)" 'FNR==NR{t=t $0 "\n"; next}
  /@TABLOLAR@/{printf "%s", t; next} {gsub(/@NOPS@/, nops); gsub(/@MSGS@/, msgs); gsub(/@PUTIB@/, puti); gsub(/@PUTCB@/, putc); print}' tablolar.txt gen2.pma.in > gen2.pma || exit 1
$PM asm gen2.pma -o gen2.pmb --strip
