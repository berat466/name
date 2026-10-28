#!/bin/bash
# Parametre tabanli paketleyici (oz/paketle.pmy) = awk paketleyici: bayt-bayt esitlik denetimi.
PM=${1:-../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); AWK=../ai_testi2/paketle.awk
T=$(mktemp -d); ok=0; bad=0
LC_ALL=C awk -f $AWK paketle.sayi > $T/p.pmy                                   # onyukleme: awk ile
cmp -s $T/p.pmy paketle.pmy && ok=$((ok+1)) || { bad=$((bad+1)); echo "HATA: paketle.pmy eski"; }
run() { "$PM" run $T/p.pmy < "$1" > $T/o1 2> /dev/null; echo $?; }
# 1) depodaki tum gercek .sayi dosyalari
n=0; for f in $(find .. -name '*.sayi' -not -path '*/node_modules/*' | sort); do
  LC_ALL=C awk -f $AWK "$f" > $T/a 2>/dev/null; ra=$?
  "$PM" run $T/p.pmy < "$f" > $T/b 2>/dev/null; rb=$?
  if [ $ra = 0 ] && [ $rb = 0 ] && cmp -s $T/a $T/b; then ok=$((ok+1)); elif [ $ra != 0 ] && [ $rb != 0 ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "FARK: $f (awk rc=$ra pmy rc=$rb)"; fi; n=$((n+1)); done
# 2) kendi kaynagi (sabit nokta): paketle.pmy, paketle.sayi'yi yine paketle.pmy'e esit paketler
"$PM" run paketle.pmy < paketle.sayi > $T/self; cmp -s $T/self paketle.pmy && ok=$((ok+1)) || { bad=$((bad+1)); echo "HATA: sabit nokta"; }
# 3) rastgele gecerli girdiler (negatif, buyuk, yorum, bosluk cesitleri)
for s in 1 2 3 4 5 6 7 8; do
  awk -v seed=$s 'BEGIN { x = seed * 7919 + 13; for (l = 0; l < 200; l++) { line = ""; k = int((x = (x * 1664525 + 1013904223) % 4294967296) / 65536) % 8
    for (j = 0; j < k; j++) { x = (x * 1664525 + 1013904223) % 4294967296; t = int(x / 65536) % 4; x = (x * 1664525 + 1013904223) % 4294967296; m = int(x / 256) % (t == 3 ? 4000000000 : 100000)
      if (t == 0) tok = m; else if (t == 1) tok = "z" m; else if (t == 2) tok = "z-" m; else tok = m
      line = line (j ? (m % 3 == 0 ? "\t" : " ") : "") tok }
    if (k && m % 7 == 0) line = line " # yorum 5 z-2 xx"; print line } }' > $T/r.sayi
  LC_ALL=C awk -f $AWK $T/r.sayi > $T/a 2>/dev/null; "$PM" run $T/p.pmy < $T/r.sayi > $T/b 2>/dev/null
  cmp -s $T/a $T/b && ok=$((ok+1)) || { bad=$((bad+1)); echo "FARK: rastgele tohum $s"; } ; done
# 4) gecersiz girdiler her ikisinde de reddedilmeli
for bad_in in "5 -3" "5a" "z" "z-" "1 + 2" "z--4" "0x10" "3.5"; do
  echo "$bad_in" | LC_ALL=C awk -f $AWK > /dev/null 2>&1; ra=$?; echo "$bad_in" | "$PM" run $T/p.pmy > /dev/null 2>&1; rb=$?
  if [ $ra != 0 ] && [ $rb != 0 ]; then ok=$((ok+1)); else bad=$((bad+1)); echo "HATA: gecersiz '$bad_in' kabul (awk rc=$ra pmy rc=$rb)"; fi; done
rm -rf $T; echo "parametre tabanli paketleyici: gecen $ok, kalan $bad (gercek .sayi dosyasi: $n)"; [ $bad = 0 ]
