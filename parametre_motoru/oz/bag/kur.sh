#!/bin/bash
# Onyukleme (BIR KEZ pm kullanir): parametre tablolarindan bagimsiz araclari uretir.
#   gen2.pmb (ELF uretici) --[pm run gen2.pmb]--> gen2.elf ; sonra tum araclar gen2.elf ile uretilir (pm'siz): coz.elf, yaz.elf, valid.elf
# Kullanim: kur.sh [pm]   Cikti: oz/bag/{gen2,coz,yaz,valid}.elf (git'e girmez; yeniden uretilebilir)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); O=$D/..; T=$(mktemp -d)
$PM liste $O/elf2/gen2.pmb > $T/gen2.l || exit 1
$PM run $O/elf2/gen2.pmb < $T/gen2.l > gen2.elf || exit 1; chmod +x gen2.elf
# bundan sonrasi pm'siz: gen2.elf kendi kendini yeniden uretir (sabit nokta) ve diger araclari uretir
./gen2.elf < $T/gen2.l | cmp -s - gen2.elf || { echo "kur: sabit nokta bozuk"; exit 1; }
$PM yapisal $O/coz.pmy -o $T/coz.pmb --strip > /dev/null && $PM liste $T/coz.pmb > $T/coz.l && ./gen2.elf < $T/coz.l > coz.elf || exit 1
$PM yapisal $O/yaz.pmy -o $T/yaz.pmb --strip > /dev/null && $PM liste $T/yaz.pmb > $T/yaz.l && ./gen2.elf < $T/yaz.l > yaz.elf || exit 1
$PM liste $O/dogr/valid.pmb > $T/valid.l && ./gen2.elf < $T/valid.l > valid.elf || exit 1
chmod +x coz.elf yaz.elf valid.elf; rm -rf $T
ls -l gen2.elf coz.elf yaz.elf valid.elf | awk '{print $5, $9}'
