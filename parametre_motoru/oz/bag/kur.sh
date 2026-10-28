#!/bin/bash
# Onyukleme (BIR KEZ pm kullanir): depodaki parametre tablolarindan (.pmb) bagimsiz araclari (ELF) uretir.
#   gen2.pmb --[pm run]--> gen2.elf ; sonrasi tum araclar gen2.elf ile uretilir (pm'siz). PMY1 kaynaklari (coz.pmy, yaz.pmy) yap.elf + asm.elf ile .pmb olur.
# Kullanim: kur.sh [pm] [cikti_dizini]   Cikti: {gen2,asm,valid,yap,doldur,opstab,fmtc,sablonc,coz,yaz}.elf (varsayilan oz/bag; git'e girmez)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); O=$D/..; PM=$(realpath "$PM"); OUT=${2:-$D}; mkdir -p "$OUT"; OUT=$(realpath "$OUT"); T=$(mktemp -d)
$PM liste $O/elf2/gen2.pmb > $T/gen2.l || exit 1
$PM run $O/elf2/gen2.pmb < $T/gen2.l > $OUT/gen2.elf || exit 1; chmod +x $OUT/gen2.elf
$OUT/gen2.elf < $T/gen2.l | cmp -s - $OUT/gen2.elf || { echo "kur: sabit nokta bozuk"; exit 1; }
mk() { $PM liste "$2" > $T/x.l && $OUT/gen2.elf < $T/x.l > $OUT/$1.elf || { echo "kur: $1 uretilemedi"; exit 1; }; chmod +x $OUT/$1.elf; }
mk asm $O/asm/asm.pmb; mk valid $O/dogr/valid.pmb; mk yap $O/yap/yap.pmb
for t in doldur opstab fmtc sablonc; do mk $t $O/arac/$t.pmb; done
# PMY1 kaynaklari: yap.elf (.pma) -> asm.elf (.pmb) -> gen2.elf (ELF)
for t in coz yaz; do $OUT/yap.elf < $O/$t.pmy > $T/$t.pma && $OUT/asm.elf < $T/$t.pma > $T/$t.pmb || { echo "kur: $t.pmy cevrilemedi"; exit 1; }; mk $t $T/$t.pmb; done
rm -rf $T
ls -l $OUT/*.elf | while read -r a b c d s rest; do echo "$s ${rest##* }"; done
