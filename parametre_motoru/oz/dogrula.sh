#!/bin/bash
# Parametre tabanli paketleyici (oz/paketle.pmy) = bagimsiz C++ referansi (oz/ref_paketle.cpp): bayt-bayt, TAM 64 bit.
PM=${1:-../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); T=$(mktemp -d); ok=0; bad=0
g++ -O2 -o $T/ref ref_paketle.cpp || exit 1; AWK=../ai_testi2/paketle.awk
LC_ALL=C awk -f $AWK paketle.sayi > $T/p0.pmy                                  # onyukleme: awk ile (yalniz kucuk sabitler gerekir)
cmp -s $T/p0.pmy paketle.pmy && ok=$((ok+1)) || { bad=$((bad+1)); echo "HATA: paketle.pmy kaynaktan uretilenle ayni degil"; }
P="$PM run $D/paketle.pmy"
same() { $T/ref < "$1" > $T/a 2>/dev/null; ra=$?; $P < "$1" > $T/b 2>/dev/null; rb=$?
  if [ $ra = 0 ] && [ $rb = 0 ] && cmp -s $T/a $T/b; then return 0; elif [ $ra != 0 ] && [ $rb != 0 ]; then return 0; fi; return 1; }
n=0; for f in $(find .. -name '*.sayi' | sort); do same $f && ok=$((ok+1)) || { bad=$((bad+1)); echo "FARK: $f"; }; n=$((n+1)); done
$P < paketle.sayi | cmp -s - paketle.pmy && ok=$((ok+1)) || { bad=$((bad+1)); echo "HATA: sabit nokta"; }
# 64 bit sinirlari: gecerli
printf '%s\n' "0 1 127 128 16383 16384 4294967295 4294967296 9223372036854775807 9223372036854775808 18446744073709551615" "z0 z1 z-1 z63 z-64 z64 z-65 z4611686018427387903 z-4611686018427387904 z9223372036854775807 z-9223372036854775808" > $T/s.txt
same $T/s.txt && ok=$((ok+1)) || { bad=$((bad+1)); echo "FARK: 64 bit sinirlari"; }
# rastgele 64 bit (gecerli)
for s in 1 2 3 4 5 6 7 8 9 10; do RANDOM=$s; : > $T/r.txt
  for i in $(seq 1 300); do v=$(( (RANDOM<<48) ^ (RANDOM<<33) ^ (RANDOM<<18) ^ (RANDOM<<3) ^ (RANDOM&7) )); k=$((RANDOM%5))
    case $k in 0) printf '%u ' $v;; 1) printf 'z%d ' $v;; 2) printf 'z%d ' $((-v));; 3) printf '%u\t' $((v>>(RANDOM%60)));; 4) printf '# not %d z-5\n' $v;; esac >> $T/r.txt; done
  same $T/r.txt && ok=$((ok+1)) || { bad=$((bad+1)); echo "FARK: rastgele 64 bit tohum $s"; }; done
# gecersiz/tasan: ikisi de reddetmeli
for b in "18446744073709551616" "99999999999999999999" "184467440737095516150" "z9223372036854775808" "z-9223372036854775809" "z99999999999999999999" "-5" "5a" "z" "z-" "1 + 2" "z--4" "0x10" "3.5" "5 -3" "z5z"; do
  echo "$b" > $T/i.txt; $T/ref < $T/i.txt > /dev/null 2>&1; ra=$?; $P < $T/i.txt > /dev/null 2>&1; rb=$?
  if [ $ra != 0 ] && [ $rb != 0 ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "HATA: gecersiz '$b' (ref rc=$ra pmy rc=$rb)"; fi; done
rm -rf $T; echo "parametre tabanli paketleyici (tam 64 bit): gecen $ok, kalan $bad (gercek .sayi: $n)"; [ $bad = 0 ]
