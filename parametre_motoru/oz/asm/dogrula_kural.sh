#!/bin/bash
# YENIDEN YAZMA KURALLARI (komut tablosunun REWRITE/PSEUDO bolumleri) denetimi: asm.pmb (kurallari tablodan okur) = C++ `pm asm` (kurallar elle yazilmis; BAGIMSIZ REFERANS).
# Her uc operandli komut / takma komut x operand deseni (yazmac/anlik b ve c) x anlik degerler (sinirlar dahil): ayni PMB1 bayt dizisi ya da ikisi de reddeder.
# Kullanim: dogrula_kural.sh [pm]   (ASM: baska asm.pmb)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); T=$(mktemp -d); A=${ASM:-$D/asm.pmb}
ok=0; bad=0; pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
IMMS="0 1 -1 2 5 31 63 64 100000 2147483647 -2147483648 2147483648 -2147483647 -2 3"
for op in add sub mul and or xor shl shr sar eq ne lt le gt ge div mod fadd fsub fmul fdiv feq flt fle fgt fge; do
  for pat in rr ri ir ii; do
    { echo "func main() {"; echo " ldi x, 3"; echo " ldi y, 4"
      for v in $IMMS; do
        case $pat in rr) b=x; c=y;; ri) b=x; c=$v;; ir) b=$v; c=y;; ii) b=$v; c=$v;; esac
        echo " $op r, $b, $c"; echo " host puti, r"
        [ $pat = rr ] && break
      done
      echo " halt"; echo "}"; } > $T/k.pma
    $PM asm $T/k.pma -o $T/c.pmb --strip > /dev/null 2>&1; r1=$?
    (ulimit -v 900000; timeout 60 $PM run $A < $T/k.pma > $T/p.pmb 2>/dev/null); r2=$?
    if [ $r1 = 0 ] && [ $r2 = 0 ] && cmp -s $T/c.pmb $T/p.pmb; then pass
    elif [ $r1 != 0 ] && [ $r2 != 0 ]; then pass
    else fail "$op $pat: C++ rc=$r1 parametre rc=$r2 $(cmp $T/c.pmb $T/p.pmb 2>&1 | head -c 50)"; fi
  done
done
# her imm degeri ayri programda (bir yeniden yazma sonrasinda hata baska komutu maskelemesin): sub/div/mod/lt/gt sinirlari
for op in sub div mod lt gt le ge add; do for v in $IMMS; do for pat in ri ir; do
  case $pat in ri) b=x; c=$v;; ir) b=$v; c=x;; esac
  printf 'func main() {\n ldi x, 3\n %s r, %s, %s\n host puti, r\n halt\n}\n' $op $b $c > $T/k.pma
  $PM asm $T/k.pma -o $T/c.pmb --strip > /dev/null 2>&1; r1=$?
  (ulimit -v 900000; timeout 60 $PM run $A < $T/k.pma > $T/p.pmb 2>/dev/null); r2=$?
  if [ $r1 = 0 ] && [ $r2 = 0 ] && cmp -s $T/c.pmb $T/p.pmb; then pass; elif [ $r1 != 0 ] && [ $r2 != 0 ]; then pass; else fail "$op $b,$c: C++ rc=$r1 parametre rc=$r2"; fi
done; done; done
# takma komut yanlis operand sayisi: ikisi de reddeder
for op in gt ge fgt fge; do for args in "r, x" "r, x, y, y"; do printf 'func main() {\n ldi x, 3\n %s %s\n halt\n}\n' $op "$args" > $T/k.pma; $PM asm $T/k.pma -o $T/c.pmb --strip > /dev/null 2>&1; r1=$?; (timeout 60 $PM run $A < $T/k.pma > $T/p.pmb 2>/dev/null); r2=$?; [ $r1 != 0 ] && [ $r2 != 0 ] && pass || fail "$op '$args': rc $r1/$r2"; done; done
echo "yeniden yazma kurallari: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
