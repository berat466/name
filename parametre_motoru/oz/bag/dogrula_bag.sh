#!/bin/bash
# pm'siz arac zinciri (kur.sh ile uretilen *.elf) denetimi. Kullanim: dogrula_bag.sh [pm] [bayt_mutanti_program_basina]
# zincir.sh yalniz {timeout, chmod, rm} bulunan bir PATH ile (pm ve g++ erisilemez) calistirilir; pm yalnizca KARSILASTIRMA referansi olarak bu betikte kullanilir.
PM=${1:-../../pm}; MUT=${2:-40}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
ok=0; bad=0; acc=0; rej=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
./kur.sh $PM > /dev/null || { echo "kur.sh basarisiz"; exit 1; }
mkdir -p $T/bin; for c in timeout chmod rm; do ln -s $(command -v $c) $T/bin/$c; done
Z() { env -i PATH=$T/bin ./zincir.sh "$@"; }
g++ -O2 -o $T/mut ../mutasyon.cpp || exit 1
corp=$T/corp; mkdir -p $corp; n=0
for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' 2>/dev/null | sort); do for s in "" "--strip"; do $PM asm $f -o $corp/k$n.pmb $s > /dev/null 2>&1 && n=$((n+1)); done; done
for f in $(find $R/uygulamalar $R/tests -name '*.pmt' 2>/dev/null | sort); do (cd $(dirname $f) && $PM sablon $f -o $corp/k$n.pmb > /dev/null 2>&1) && n=$((n+1)); done
for f in $(find $R/oz $R/ai_testi3 $R/ai_testi4 -name '*.pmy' 2>/dev/null | sort); do $PM yapisal $f -o $corp/k$n.pmb > /dev/null 2>&1 && n=$((n+1)); done
# referans karar: pm yukleyicisi (decode_program) kabul eder VE uretici destekler mi (pm tabanli gen2 ile)?
ref() { # pmb -> $T/ref.elf uretir; donus 0: kabul+uretildi, 1: ret
  $PM dump $1 > /dev/null 2>&1 || return 1
  $PM liste $1 > $T/rl.txt 2>/dev/null || return 1
  (ulimit -v 700000; timeout 20 $PM run $R/oz/elf2/gen2.pmb < $T/rl.txt > $T/ref.elf 2>/dev/null) || return 1
  return 0; }
check() { # etiket pmb
  rm -f $T/z.elf; Z $2 $T/z.elf > /dev/null 2>&1; rz=$?
  ref $2; rr=$?
  if [ $rr = 0 ]; then
    if [ $rz = 0 ] && cmp -s $T/ref.elf $T/z.elf; then pass; acc=$((acc+1)); else fail "$1: referans uretti, zincir rc=$rz ya da ELF farkli"; return; fi
    # davranis: uretilen ELF = pm run
    (ulimit -v 700000; timeout 10 $PM run $2 < /dev/null > $T/o1 2> $T/e1); r1=$?
    if grep -q "TRAP: FUEL" $T/e1 || [ $r1 = 124 ]; then return; fi
    (ulimit -v 700000; timeout 10 $T/z.elf < /dev/null > $T/o2 2> $T/e2); r2=$?
    [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && pass || fail "$1: davranis farkli pm rc=$r1 elf rc=$r2"
  else
    if [ $rz != 0 ] && [ ! -e $T/z.elf ]; then pass; rej=$((rej+1)); else fail "$1: referans reddetti ama zincir rc=$rz"; fi
  fi
}
for p in $corp/*.pmb; do check "derlem $(basename $p)" $p; done
echo "derlem: $n PMB1 dosyasi"
m=0; for p in $(ls $corp/*.pmb | awk 'NR % 3 == 1'); do mkdir -p $T/mu; rm -f $T/mu/*; $T/mut $p $T/mu $MUT $((m+11)); m=$((m+1))
  for q in $T/mu/*.bin; do check "mutant $(basename $p)/$(basename $q)" $q; done; done
# C++ siz kanit: zincirde pm cagrisi yok
grep -v '^#' zincir.sh | grep -qE '(^|[^a-z_.])pm( |$)|g\+\+' && fail "zincir.sh pm/g++ iceriyor" || pass
rm -rf $T
echo "pm'siz zincir: gecen $ok, kalan $bad (uretilen $acc, reddedilen $rej)"; [ $bad = 0 ]
