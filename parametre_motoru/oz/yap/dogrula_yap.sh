#!/bin/bash
# Parametre tabanli PMY1 -> .pma ozellestiricisi (oz/yap/yap.pmb) = C++ `pm yapisal`: ayni metin (bayt-bayt), ayni kabul/ret, ayni PMB1 zinciri.
# Kullanim: dogrula_yap.sh [pm] [rastgele_model] [bayt_mutanti]    (tum calistirmalar zaman+bellek siniri altinda)
PM=${1:-../../pm}; NR=${2:-300}; NM=${3:-1500}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
ok=0; bad=0; acc=0; rej=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
YAP=${YAPPMB:-$D/yap.pmb}; ASMPMB=${ASMPMB:-$D/../asm/asm.pmb}
if [ -z "$YAPPMB" ]; then cp yap.pmb $T/old.pmb; ./uret.sh $PM > /dev/null 2>&1; cmp -s yap.pmb $T/old.pmb && pass || fail "yap.pmb guncel degil (uret.sh)"; fi
g++ -O2 -o $T/mut ../mutasyon.cpp || exit 1
runyap() { if [ -n "$YAPELF" ]; then (ulimit -v 900000; timeout 30 $YAPELF < $1 > $T/mine.pma 2> $T/mine.err); else (ulimit -v 900000; timeout 30 $PM run $YAP < $1 > $T/mine.pma 2> $T/mine.err); fi; }
runasm() { if [ -n "$ASMELF" ]; then (ulimit -v 900000; timeout 30 $ASMELF < $1 > $2 2>/dev/null); else (ulimit -v 900000; timeout 30 $PM run $ASMPMB < $1 > $2 2>/dev/null); fi; }
cmpone() {   # etiket dosya.pmy
  rm -f $T/ref.pma $T/ref.pmb
  $PM yapisal $2 --pma $T/ref.pma -o $T/ref.pmb --strip > /dev/null 2>&1; rr=$?
  runyap $2; rm_=$?
  if [ -s $T/ref.pma ]; then
    if [ $rm_ = 0 ] && cmp -s $T/ref.pma $T/mine.pma; then pass; else fail "$1: metin farkli (C++ uretti, parametre rc=$rm_ $(head -c 60 $T/mine.err))"; cp $2 /tmp/yap_fark_$((ok+bad)).pmy; return; fi
    # zincir: parametre .pma -> asm.pmb  ==  pm yapisal -o  (PMB1 baytlari)
    runasm $T/mine.pma $T/mine.pmb; ra=$?
    if [ $rr = 0 ]; then { [ $ra = 0 ] && cmp -s $T/ref.pmb $T/mine.pmb; } && { pass; acc=$((acc+1)); } || fail "$1: zincir PMB1 farkli (rr=$rr ra=$ra)"
    else [ $ra != 0 ] && { pass; rej=$((rej+1)); } || fail "$1: C++ zincir reddetti ama parametre zincir kabul etti"; fi
  else
    if [ $rm_ != 0 ]; then pass; rej=$((rej+1)); else fail "$1: C++ reddetti, parametre kabul etti"; cp $2 /tmp/yap_fark_$((ok+bad)).pmy; fi
  fi
}
# ---- 1) derlem
n=0; for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 $R/uygulamalar -name '*.pmy' 2>/dev/null | sort); do cmpone "derlem $f" $f; n=$((n+1)); done
for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 -name '*.sayi' 2>/dev/null | sort); do awk -f $R/ai_testi3/paketle.awk $f > $T/s.pmy 2>/dev/null && { cmpone "sayi $f" $T/s.pmy; n=$((n+1)); }; done
echo "derlem: $n tablo"
# ---- 2) rastgele gecerli modeller
for s in $(seq 1 $NR); do awk -v seed=$s -f rastgele_pmy.awk | awk -f $R/ai_testi3/paketle.awk > $T/r$s.pmy 2>/dev/null || { fail "rastgele $s paketlenemedi"; continue; }; cmpone "rastgele-$s" $T/r$s.pmy; done
# ---- 2b) sinir degerli modeller (sinir-1, sinir, sinir+1)
./sinir_modelleri.sh $T/sinir; for f in $T/sinir/*.pmy; do cmpone "sinir $(basename $f)" $f; done
echo "sinir modelleri tamam"
# ---- 3) bayt mutantlari (derlem + rastgele modeller)
srcs=($(find $R/ai_testi3/ref $R/ai_testi4/cozumler $R/oz -name '*.pmy' | sort)); for s in $(seq 1 12); do srcs+=($T/r$s.pmy); done
m=0; for p in "${srcs[@]}"; do mkdir -p $T/mu; rm -f $T/mu/*; $T/mut $p $T/mu $((NM / ${#srcs[@]} + 1)) $((m+5)); m=$((m+1)); for q in $T/mu/*.bin; do cmpone "mutant $(basename $p)/$(basename $q)" $q; done; done
echo "mutantlar tamam"
# ---- 4) ELF olarak (yap.elf, gen2 ile uretilmis) ve yalniz ELF'lerle zincir
if [ -z "$YAPELF" ]; then $PM liste $YAP > $T/y.l; (ulimit -v 900000; $PM run $D/../elf2/gen2.pmb < $T/y.l > $T/yap.elf) && chmod +x $T/yap.elf && pass || fail "yap.pmb ELF'e cevrilemedi"; YAPELF=$T/yap.elf; fi
for f in $R/ai_testi3/ref/*.pmy $R/ai_testi4/cozumler/*.pmy $T/r1.pmy $T/r2.pmy $T/r3.pmy; do $PM yapisal $f --pma $T/ref.pma > /dev/null 2>&1 || continue; (ulimit -v 900000; timeout 30 $YAPELF < $f > $T/e.pma 2>/dev/null) && cmp -s $T/ref.pma $T/e.pma && pass || fail "yap.elf farkli: $f"; done
rm -rf $T
echo "yapisal (parametre): gecen $ok, kalan $bad (zincir kabul $acc, ret $rej)"; [ $bad = 0 ]
