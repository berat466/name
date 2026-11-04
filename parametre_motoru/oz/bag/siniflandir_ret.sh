#!/bin/bash
# zincir.sh'in reddettigi PMY1 girdilerini siniflandirir: (A) C++ `pm yapisal` de gecersiz sayiyor, (B) gecerli ama ELF uretici desteklemiyor (neden: present/poll/frame, putf, kayan nokta), (C) diger.
# Kullanim: siniflandir_ret.sh [pm] [rastgele_sayisi]   (BAG: arac ELF dizini gerekir)
PM=${1:-../../pm}; NR=${2:-300}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
: ${BAG:?BAG (arac ELF dizini) gerekli}; export BAG
declare -A C; kayit() { C[$1]=$(( ${C[$1]:-0} + 1 )); }
sinif() {   # etiket dosya.pmy
  IFS= read -r -n 4 H0 < "$2"; [ "$H0" = "PMY1" ] || return
  rm -f $T/u.elf; ./zincir.sh "$2" $T/u.elf > /dev/null 2>&1 && { kayit "ELF uretildi"; return; }
  if ! $PM yapisal "$2" -o $T/r.pmb --strip > $T/y.out 2>&1; then
    # C++ neden reddetti? yapisal (decode/generate) mi assembler mi
    if $PM yapisal "$2" --pma $T/r.pma > /dev/null 2>&1 || [ -s $T/r.pma ]; then kayit "A2. C++ de gecersiz: uretilen .pma assembler'da reddedildi"; else kayit "A1. C++ de gecersiz: PMY1 yapisi/baglami reddedildi"; fi
    rm -f $T/r.pma; return; fi
  $PM dump $T/r.pmb > $T/d.txt 2>/dev/null
  n=""; for k in present poll frame putf; do grep -qw "$k" $T/d.txt && n="$n$k "; done
  grep -qE "^ *[0-9]+ +(fadd|fsub|fmul|fdiv|fneg|fsqrt|feq|flt|fle|i2f|f2i) " $T/d.txt && n="${n}kayan-nokta "
  [ -n "$n" ] && kayit "B. ELF uretici eksigi (${n% }): C++ gecerli" || kayit "C. diger (C++ gecerli, neden belirsiz)"
}
for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 -name '*.pmy' | sort); do sinif "pmy" $f; done
for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 -name '*.sayi' | sort); do awk -f $R/ai_testi3/paketle.awk $f > $T/s.pmy 2>/dev/null && sinif "sayi" $T/s.pmy; done
for s in $(seq 1 $NR); do awk -v seed=$s -f $D/../yap/rastgele_pmy.awk | awk -f $R/ai_testi3/paketle.awk > $T/r.pmy 2>/dev/null && sinif "rastgele" $T/r.pmy; done
for k in "${!C[@]}"; do echo "${C[$k]}	$k"; done | sort -rn
rm -rf $T
