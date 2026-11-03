#!/bin/bash
# cfgopt.cx (gelistirme gorunumu) -> sayisal program: ifx.pmb ile PMA, pm'siz zincirle (asm.elf) PMB1. Kullanim: uret.sh [pm]   (cikti: cfgopt.pmb)
D=$(cd "$(dirname "$0")" && pwd); R=$D/../..; PM=${1:-$R/pm}; cd $D
TAB=$R/oz/komut/komutlar.pmk; B=${BAG:-$(mktemp -d)}; [ -x $B/asm.elf ] || $R/oz/bag/kur.sh $PM $B > /dev/null 2>&1
{ echo "@@imza"; cat $TAB; } | $B/komut.elf | tr '\n' ' ' > $D/.imza; echo >> $D/.imza
$PM ifade cfgopt.cx --pmx1 $D/.cfg.pmx > /dev/null 2>&1; { cat $TAB; cat $D/.imza; cat $D/.cfg.pmx; } | $PM run $R/oz/ifade/num/ifx.pmb > $D/.cfg.pma || exit 1
grep -aq '@@HATA' $D/.cfg.pma && { grep -a '@@HATA' $D/.cfg.pma | head -2; exit 1; }
$B/asm.elf < $D/.cfg.pma > cfgopt.pmb && rm -f $D/.cfg.pmx $D/.cfg.pma $D/.imza && echo "cfgopt.pmb: $(wc -c < cfgopt.pmb) bayt"
