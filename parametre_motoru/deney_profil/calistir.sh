#!/bin/bash
# Profil deneyi: (1) VM mekanizma kapat/ac, (2) dinamik komut histogrami, (3) AOT (C++'a cevirme) ust sinir deneyi, (4) yerel C++.
# Kullanim: ./calistir.sh [cikti_dizini]   -- motor (src/) degistirilmez.
set -e
cd "$(dirname "$0")"
SRC=../src; OUT=${1:-/tmp/pm_profil}; mkdir -p "$OUT"; W="fib35 loop_sum sieve10m floatloop memsum"
b() { n=$1; shift; g++ -std=c++17 -O2 -ffp-contract=off -I$SRC "$@" -o $OUT/$n bench_main.cpp vm_exp.cpp $SRC/asm.cpp $SRC/ikili.cpp; }
b v_base; b v_hist -DPM_HIST; b v_nofuel -DPM_NOFUEL; b v_nozero -DPM_NOZERO; b v_nochk -DPM_NOCHK
b v_cgoto -DPM_CGOTO; b v_cgoto_nofuel -DPM_CGOTO -DPM_NOFUEL
b v_all_off -DPM_CGOTO -DPM_NOFUEL -DPM_NOZERO -DPM_NOCHK -DPM_NOMEMCHK
g++ -std=c++17 -O2 -I$SRC -o $OUT/pm2cpp pm2cpp.cpp $SRC/asm.cpp $SRC/ikili.cpp
g++ -std=c++17 -O2 -o $OUT/native_O2 native.cpp; g++ -std=c++17 -O0 -o $OUT/native_O0 native.cpp
for w in $W; do
  $OUT/pm2cpp $w.pma none > $OUT/aot_$w.cpp;          g++ -std=c++17 -O2 -o $OUT/aot_$w  $OUT/aot_$w.cpp
  $OUT/pm2cpp $w.pma restrict > $OUT/aotr_$w.cpp;     g++ -std=c++17 -O2 -o $OUT/aotr_$w $OUT/aotr_$w.cpp
  $OUT/pm2cpp $w.pma restrict+fuel > $OUT/aotf_$w.cpp; g++ -std=c++17 -O2 -o $OUT/aotf_$w $OUT/aotf_$w.cpp
done
{
echo "### Mekanizma kapat/ac (en iyi 3 kosu, s; ns/komut)"
for v in v_base v_nofuel v_nozero v_nochk v_cgoto v_cgoto_nofuel v_all_off; do
  echo "== $v"; for w in $W; do printf "%-10s " $w; $OUT/$v $w.pma 3 | tail -1; done; done
echo; echo "### Dinamik komut histogrami (ilk 6)"
for w in $W; do echo "== $w"; HIST=1 $OUT/v_hist $w.pma 1 | grep -v '^adim' | grep -E '^  [a-z]' | sort -k2 -n -r | head -6; done
echo; echo "### Yerel C++"; echo "-O2"; $OUT/native_O2; echo "-O0"; $OUT/native_O0
echo; echo "### AOT (en iyi 5 kosu, duvar saati, s) -- cikti VM ile ayni mi?"
best() { local b=99; for i in 1 2 3 4 5; do local t0=$(date +%s.%N); "$@" > $OUT/o.tmp; local t1=$(date +%s.%N); b=$(echo "if ($t1 - $t0 < $b) $t1 - $t0 else $b" | bc -l); done; printf "%.4f" $b; }
printf "%-10s %9s %9s %9s %9s  %s\n" is VM AOT AOT+rstr AOT+fuel "cikti-ayni"
for w in $W; do
  vm=$($OUT/v_base $w.pma 3 | tail -1 | sed 's/.*en_iyi=\([0-9.]*\).*/\1/')
  ref=$($OUT/v_base $w.pma 1 | head -1); a=$($OUT/aot_$w); f=$($OUT/aotf_$w)
  same=hayir; [ "$ref" = "$a" ] && [ "$ref" = "$f" ] && same=evet
  printf "%-10s %9s %9s %9s %9s  %s\n" $w $vm $(best $OUT/aot_$w) $(best $OUT/aotr_$w) $(best $OUT/aotf_$w) $same
done
} | tee $OUT/sonuc.txt
