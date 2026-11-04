#!/bin/bash
# PMY1 tabanli ELF ureticisi (oz/elf/elf.pmy) denetimi: uretilen bagimsiz ELF, pm motoruyla ayni cikti ve cikis koduna sahip olmali.
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0; desteklenmeyen=0; n=0; yakit_bagimli=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
cmpone() {   # etiket pmb stdin_dosyasi
  $PM liste $2 > $T/l.txt 2>/dev/null || return
  if ! $PM run $D/elf.pmy < $T/l.txt > $T/p.elf 2>/dev/null; then desteklenmeyen=$((desteklenmeyen+1)); return; fi
  chmod +x $T/p.elf; n=$((n+1))
  timeout 20 $PM run $2 < $3 > $T/o1 2> $T/e1; r1=$?
  if grep -q "TRAP: FUEL" $T/e1 || [ $r1 = 124 ]; then yakit_bagimli=$((yakit_bagimli+1)); n=$((n-1)); return; fi
  timeout 20 $T/p.elf < $3 > $T/o2 2> $T/e2; r2=$?
  [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && pass || fail "$1: pm rc=$r1 elf rc=$r2 cikti $(cmp $T/o1 $T/o2 2>&1 | head -c 60)"
  if [ $r1 = 2 ]; then [ "$(head -c 11 $T/e1 | cut -d' ' -f1-2)" = "$(head -c 11 $T/e2 | cut -d' ' -f1-2)" ] && pass || fail "$1: tuzak adi farkli ($(head -c 20 $T/e1) | $(head -c 20 $T/e2))"; fi
}
: > $T/bos
for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' 2>/dev/null | sort); do $PM asm $f -o $T/k.pmb --strip > /dev/null 2>&1 || continue; cmpone "$f" $T/k.pmb $T/bos; done
# rastgele (desteklenen altkumeden) programlar
for s in $(seq 1 ${2:-60}); do awk -v seed=$s -f rastgele_pma.awk > $T/r.pma; $PM asm $T/r.pma -o $T/r.pmb --strip > /dev/null 2>&1 || { fail "rastgele $s derlenmedi"; continue; }; cmpone "rastgele$s" $T/r.pmb $T/bos; done
# bagimsizlik: tek PT_LOAD, yorumlayici/dinamik yok, motor kopyasi yok
$PM run $D/elf.pmy < $T/l.txt > $T/p.elf 2>/dev/null; chmod +x $T/p.elf
readelf -lW $T/p.elf 2>/dev/null | grep -qE "INTERP|DYNAMIC" && fail "ELF dinamik/yorumlayicili" || pass
[ "$(readelf -lW $T/p.elf 2>/dev/null | grep -c ' LOAD ')" = 1 ] && pass || fail "tek LOAD degil"
grep -qa -e "PMB1" -e "libstdc" -e "GLIBC" $T/p.elf && fail "ELF icinde motor/libc izi" || pass
rm -rf $T; echo "ELF uretici: gecen $ok, kalan $bad (karsilastirilan program: $n, ureticinin reddettigi/desteklenmeyen: $desteklenmeyen, yakit-tuzagina bagli atlanan: $yakit_bagimli)"; [ $bad = 0 ]
