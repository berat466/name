#!/bin/bash
# asm.pmb'nin ondalik sabit ayristirmasi (fbits) = C++ strtod: PMB1 bayt-bayt ayni. Kullanim: dogrula_ondalik.sh [pm] [tohum_sayisi]   (ASMPMB ile asm.pmb secilebilir)
PM=${1:-../../pm}; NF=${2:-60}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); T=$(mktemp -d); AS=${ASMPMB:-$D/asm.pmb}
ok=0; bad=0; rej=0
for sd in $(seq 1 $NF); do
  awk -v seed=$sd -v n=60 -f ondalik_gen.awk > $T/f.pma
  $PM asm $T/f.pma -o $T/r.pmb --strip > /dev/null 2>&1; rr=$?
  (ulimit -v 900000; timeout 120 $PM run $AS < $T/f.pma > $T/m.pmb 2> /dev/null); rm_=$?
  if [ $rr = 0 ] && [ $rm_ = 0 ] && cmp -s $T/r.pmb $T/m.pmb; then ok=$((ok+1)); elif [ $rr != 0 ] && [ $rm_ != 0 ]; then ok=$((ok+1)); rej=$((rej+1)); else bad=$((bad+1)); echo "HATA: ondalik-$sd (ref rc=$rr, parametre rc=$rm_)"; fi
done
[ $rej -lt $NF ] || { bad=$((bad+1)); echo "HATA: hicbir program kabul edilmedi (iki taraf da hep reddediyor: bos gecis)"; }
echo "ondalik sabit: $ok gecti, $bad basarisiz (ikisi de reddeden: $rej)"
rm -rf $T; [ $bad = 0 ]
