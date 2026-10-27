#!/bin/bash
# Test kosucusu (bash). Her tests/*.pma icin: dogrudan calistir, beklenen ciktiyla karsilastir;
# sonra ikiliye cevir (normal ve --strip), ayni ciktiyi vermesini dogrula.
PM=${1:-./pm}
cd "$(dirname "$0")"
PM=$(cd .. && realpath "$PM")
pass=0; fail=0; failed=""
T=$(mktemp -d)
check() { # ad kosul aciklama
  if [ "$2" = ok ]; then pass=$((pass+1)); else fail=$((fail+1)); failed="$failed\n  $1: $3"; fi
}
for f in *.pma; do
  b=${f%.pma}
  args=""; [ -f $b.args ] && args=$(cat $b.args)
  inp=/dev/null; [ -f $b.in ] && inp=$b.in
  rc_exp=0; [ -f $b.rc ] && rc_exp=$(cat $b.rc)
  "$PM" run $f $args < $inp > $T/out 2> $T/err; rc=$?
  [ "$rc" = "$rc_exp" ] && check $b ok || check $b bad "cikis kodu $rc (beklenen $rc_exp)"
  if [ -f $b.out ]; then cmp -s $T/out $b.out && check $b ok || check $b bad "cikti beklenenle ayni degil"
  else [ ! -s $T/out ] && check $b ok || check $b bad "beklenmeyen cikti"; fi
  if [ -f $b.errmatch ]; then grep -qF "$(cat $b.errmatch)" $T/err && check $b ok || check $b bad "stderr '$(cat $b.errmatch)' icermiyor: $(head -c 200 $T/err)"; fi
  if [ "$rc_exp" != 1 ]; then   # ikili tur
    for mode in "" "--strip"; do
      "$PM" asm $f -o $T/x.pmb $mode > /dev/null 2>&1 || { check $b bad "asm basarisiz ($mode)"; continue; }
      "$PM" run $T/x.pmb $args < $inp > $T/out2 2> $T/err2; rc2=$?
      [ "$rc2" = "$rc" ] && cmp -s $T/out $T/out2 && check $b ok || check $b bad "ikili tur ($mode) farkli: rc $rc2"
    done
  fi
done
rm -rf $T
echo "gecen: $pass, kalan: $fail"
[ $fail -eq 0 ] || { printf "BASARISIZ:$failed\n"; exit 1; }
echo "KATI ESIT: tum testler gecti"
