#!/bin/bash
# Onyukleme (BIR KEZ pm kullanir): parametre tablolarindan bagimsiz araclari uretir.
#   gen2.pmb (ELF uretici) --[pm run gen2.pmb]--> gen2.elf ; sonra tum araclar gen2.elf ile uretilir (pm'siz): asm, coz, yaz, valid
# Kullanim: kur.sh [pm] [cikti_dizini]   Cikti: {gen2,asm,coz,yaz,valid}.elf (varsayilan: oz/bag; git'e girmez; yeniden uretilebilir)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); O=$D/..; PM=$(realpath "$PM"); OUT=${2:-$D}; mkdir -p "$OUT"; OUT=$(realpath "$OUT"); T=$(mktemp -d)
$PM liste $O/elf2/gen2.pmb > $T/gen2.l || exit 1
$PM run $O/elf2/gen2.pmb < $T/gen2.l > $OUT/gen2.elf || exit 1; chmod +x $OUT/gen2.elf
# bundan sonrasi pm'siz: gen2.elf kendi kendini yeniden uretir (sabit nokta) ve diger araclari uretir
$OUT/gen2.elf < $T/gen2.l | cmp -s - $OUT/gen2.elf || { echo "kur: sabit nokta bozuk"; exit 1; }
$PM liste $O/asm/asm.pmb > $T/asm.l && $OUT/gen2.elf < $T/asm.l > $OUT/asm.elf || exit 1
$PM liste $O/coz.pmb > $T/coz.l && $OUT/gen2.elf < $T/coz.l > $OUT/coz.elf || exit 1
$PM liste $O/yaz.pmb > $T/yaz.l && $OUT/gen2.elf < $T/yaz.l > $OUT/yaz.elf || exit 1
$PM liste $O/dogr/valid.pmb > $T/valid.l && $OUT/gen2.elf < $T/valid.l > $OUT/valid.elf || exit 1
chmod +x $OUT/*.elf; rm -rf $T
ls -l $OUT/gen2.elf $OUT/asm.elf $OUT/coz.elf $OUT/yaz.elf $OUT/valid.elf | awk '{print $5, $9}'
