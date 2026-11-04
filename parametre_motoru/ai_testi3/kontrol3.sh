#!/bin/bash
# Yapisal tablo (PMY1) denetimi: referans tablolar -> beklenen cikti (bagimsiz awk kehaneti), yorumlayici = makine kodu (3 kip), .pmy = .pmb
PM=${1:-../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM")
G=../ai_testi2/gizli_dogrular; T=$(mktemp -d); pass=0; fail=0
ok() { pass=$((pass+1)); }; bad() { fail=$((fail+1)); echo "HATA: $1"; }
pack() { LC_ALL=C awk -f paketle.awk "$1" > "$2"; }
exp_awk() { awk -f $G/$1.awk; }
check() {   # etiket dosya.sayi beklenen [pm bayraklari]
  local label=$1 f=$2 exp=$3; shift 3
  pack $f $T/x.pmy || { bad "$label paketleme"; return; }
  "$PM" run $T/x.pmy "$@" --kare-rapor > $T/o1 2> $T/e1; local r1=$?
  [ $r1 = 0 ] && [ "$(tr -s ' \n' ' ' < $T/o1)" = "$(echo "$exp" | tr -s ' \n' ' ')" ] && ok || bad "$label: cikti '$(head -c 80 $T/o1)' beklenen '$exp' rc=$r1"
  for pr in "" "--native-promote 0" "--native-promote 100000"; do
    "$PM" run $T/x.pmy "$@" --native-strict $pr --kare-rapor > $T/o2 2> $T/e2; cmp -s $T/o1 $T/o2 && cmp -s $T/e1 $T/e2 && ok || bad "$label: makine kodu farkli ($pr)"
  done
  "$PM" yapisal $T/x.pmy -o $T/x.pmb --strip > /dev/null && "$PM" run $T/x.pmb "$@" --kare-rapor > $T/o3 2> $T/e3; cmp -s $T/o1 $T/o3 && cmp -s $T/e1 $T/e3 && ok || bad "$label: .pmb farkli"
}
check A ref/A.sayi "$(exp_awk a)"
check C ref/C.sayi "$(exp_awk c)"
check T1 ref/T1.sayi "$(printf '6765\n0 1 2 3 4 5 6 7 8 9')"
for e in 99 1 2 3; do
  awk -v ppm=$T/px$e.txt -f $G/b.awk $G/ev$e.girdi > $T/b$e.exp
  check "B/ev$e" ref/B.sayi "$(cat $T/b$e.exp)" --girdi $G/ev$e.girdi --ekran $T/s.ppm
  h=$(head -3 $T/s.ppm | wc -c); tail -c +$((h+1)) $T/s.ppm | od -An -v -tu1 | tr -s ' \n' '\n' | sed '/^$/d' > $T/got.txt; cmp -s $T/got.txt $T/px$e.txt && ok || bad "B/ev$e: piksel farki"
done
# bozuk tablolar reddedilmeli, cokmemeli
printf 'PMY2' > $T/b1.pmy; "$PM" run $T/b1.pmy > /dev/null 2>&1; [ $? = 1 ] && ok || bad "yanlis baslik"
LC_ALL=C awk -f paketle.awk <(echo "80 77 89 49 0 0 0 0 0 0 0 0 0 0 0 0 0 1 99 0 0") > $T/b2.pmy; "$PM" run $T/b2.pmy > /dev/null 2>&1; [ $? = 1 ] && ok || bad "bilinmeyen komut kodu"
LC_ALL=C awk -f paketle.awk <(echo "80 77 89 49 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 5") > $T/b3.pmy; "$PM" run $T/b3.pmy > /dev/null 2>&1; [ $? = 1 ] && ok || bad "fazla bayt"
rm -rf $T
echo "yapisal tablo denetimi: gecen $pass, kalan $fail"; [ $fail = 0 ]
