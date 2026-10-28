#!/bin/bash
# Eski (eski_src = 8eb302b) ve yeni (../src) motoru karsilastirir: dinamik komut, statik komut/yazmac, dosya boyutu, tepe bellek, sure.
# Kullanim: ./kod_uretimi_karsilastir.sh [cikti_dizini]
cd "$(dirname "$0")"
OUT=${1:-/tmp/pm_kuk}; mkdir -p "$OUT"; F="-std=c++17 -O2 -ffp-contract=off"
g++ $F -o $OUT/pm_old eski_src/*.cpp && g++ $F -o $OUT/pm_new ../src/*.cpp || exit 1
g++ $F -Ieski_src -o $OUT/bench_old bench_motor.cpp eski_src/asm.cpp eski_src/ikili.cpp eski_src/vm.cpp
g++ $F -I../src -o $OUT/bench_new bench_motor.cpp ../src/asm.cpp ../src/ikili.cpp ../src/vm.cpp
g++ -O1 -o $OUT/maxrss maxrss.cpp
steps() { "$1" run "$2" --stats 2>&1 >/dev/null < /dev/null | sed -n 's/.*adim: \([0-9]*\).*/\1/p'; }
statik() { "$1" dump "$2" | sed -n 's/^func .*yazmac=\([0-9]*\), komut=\([0-9]*\)).*/\1 \2/p' | awk '{r+=$1; k+=$2} END {print r, k}'; }
size() { "$1" asm "$2" -o $OUT/x.pmb --strip | sed -n 's/^[^:]*: \([0-9]*\) bayt.*/\1/p'; }
wall() { local b=99; for i in 1 2 3 4 5; do local t0=$(date +%s.%N); "$@" > /dev/null < /dev/null; local t1=$(date +%s.%N); b=$(echo "if ($t1 - $t0 < $b) $t1 - $t0 else $b" | bc -l); done; printf "%.3f" $b; }
pct() { awk -v a="$1" -v b="$2" 'BEGIN { if (a == 0) print "-"; else printf "%+.1f%%", 100 * (b - a) / a }'; }
{
echo "### A) Yapisal olcumler (eski -> yeni, degisim)"
printf "%-26s %12s %12s %8s | %6s %6s | %6s %6s | %6s %6s\n" program "adim-eski" "adim-yeni" fark "kmt-e" "kmt-y" "yzm-e" "yzm-y" "B-eski" "B-yeni"
PROGS="deney_profil_yok"
for f in ../tests/fib.pma ../tests/gcd.pma ../tests/fact.pma ../tests/sieve.pma ../tests/sort.pma ../tests/strings.pma ../tests/matmul.pma ../tests/heap.pma ../tests/collatz.pma ../tests/opt_calls.pma fib35.pma loop_sum.pma sieve10m.pma floatloop.pma memsum.pma; do
  n=$(basename $f .pma); so=$(steps $OUT/pm_old $f); sn=$(steps $OUT/pm_new $f)
  read ro ko <<< "$(statik $OUT/pm_old $f)"; read rn kn <<< "$(statik $OUT/pm_new $f)"
  bo=$(size $OUT/pm_old $f); bn=$(size $OUT/pm_new $f)
  printf "%-26s %12s %12s %8s | %6s %6s | %6s %6s | %6s %6s\n" $n $so $sn "$(pct $so $sn)" $ko $kn $ro $rn $bo $bn
done
echo; echo "### B) Sure ve bellek (buyuk is yukleri; yurutme = proses ici en iyi 5 kosu, toplam = proses basindan sonuna en iyi 5 kosu, s)"
printf "%-12s %10s %10s %8s | %10s %10s %8s | %9s %9s\n" is "yurut-eski" "yurut-yeni" fark "toplam-e" "toplam-y" fark "RSS-e KiB" "RSS-y KiB"
for n in fib35 loop_sum sieve10m floatloop memsum; do
  ye=$($OUT/bench_old $n.pma 5); yn=$($OUT/bench_new $n.pma 5)
  te=$(wall $OUT/pm_old run $n.pma); tn=$(wall $OUT/pm_new run $n.pma)
  re=$($OUT/maxrss $OUT/pm_old run $n.pma); rn=$($OUT/maxrss $OUT/pm_new run $n.pma)
  printf "%-12s %10s %10s %8s | %10s %10s %8s | %9s %9s\n" $n $ye $yn "$(pct $ye $yn)" $te $tn "$(pct $te $tn)" $re $rn
done
} | tee $OUT/karsilastirma.txt
