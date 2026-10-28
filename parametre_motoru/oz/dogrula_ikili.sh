#!/bin/bash
# PMB1 kodlayici (oz/yaz.pmy) ve cozucu (oz/coz.pmy) = C++ referansi (pm liste / pm yazliste): bayt-bayt, sinir ve bozuk girdi denetimi.
# Kullanim: dogrula_ikili.sh [pm] [mutant_sayisi_program_basina]
PM=${1:-../pm}; MUT=${2:-300}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/..; T=$(mktemp -d); ok=0; bad=0
g++ -O2 -o $T/mut mutasyon.cpp || exit 1
Y="$PM run $D/yaz.pmy"; C="$PM run $D/coz.pmy"
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
agree() { if { [ "$1" != 0 ] && [ "$2" != 0 ]; } || { [ "$1" = 0 ] && [ "$2" = 0 ]; }; then return 0; fi; return 1; }
# ---- 1) derlem: tum .pma / .pmt / .pmy kaynaklarindan uretilen PMB1 (adli ve adsiz)
corp=$T/corp; mkdir -p $corp; n=0
for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' 2>/dev/null | sort); do for s in "" "--strip"; do $PM asm $f -o $corp/k$n.pmb $s > /dev/null 2>&1 && n=$((n+1)); done; done
for f in $(find $R/uygulamalar $R/tests -name '*.pmt' 2>/dev/null | sort); do (cd $(dirname $f) && $PM sablon $f -o $corp/k$n.pmb > /dev/null 2>&1) && n=$((n+1)); (cd $(dirname $f) && $PM sablon $f -o $corp/k$n.pmb --strip > /dev/null 2>&1) && n=$((n+1)); done
for f in $(find $R/oz $R/ai_testi3 $R/ai_testi4 -name '*.pmy' 2>/dev/null | sort); do $PM yapisal $f -o $corp/k$n.pmb > /dev/null 2>&1 && n=$((n+1)); done
for p in $corp/*.pmb; do
  $PM liste $p > $T/ref.txt
  $C < $p > $T/dec.txt 2>/dev/null && cmp -s $T/ref.txt $T/dec.txt && pass || fail "cozucu farkli: $p"
  $Y < $T/ref.txt > $T/enc.bin 2>/dev/null && cmp -s $p $T/enc.bin && pass || fail "kodlayici farkli: $p"
  $PM yazliste $T/ref.txt -o $T/cref.bin && cmp -s $T/enc.bin $T/cref.bin && pass || fail "kodlayici != pm yazliste: $p"
  $C < $T/enc.bin 2>/dev/null | cmp -s - $T/ref.txt && pass || fail "gidis-donus bilgi kaybi: $p"
done
echo "derlem: $n PMB1 dosyasi"
# ---- 2) 64 bit ve alan sinirlari (elle yazilmis listeler): kodlayici = C++ ve gidis-donus
lst() { printf '%s\n' "$@" > $T/s.txt; $Y < $T/s.txt > $T/e1.bin 2>/dev/null; $PM yazliste $T/s.txt -o $T/e2.bin 2>/dev/null; cmp -s $T/e1.bin $T/e2.bin && pass || fail "kodlayici ($1)"; $C < $T/e1.bin 2>/dev/null | cmp -s - $T/s.txt && pass || fail "gidis-donus ($1)"; }
lst "0 1 0" "3 0 1 18446744073709551615" "4 -1 0 9223372036854775807 -9223372036854775808" "1" "1" "2 2 3" "1 0 -2147483648" "1 1 2147483647" "36"
lst "1 134217728 65534" "0" "0" "0" "1" "0 1 1" "36" "4 109 97 105 110"
lst "0 134217728 65534" "1 18446744073709551615" "1 -9223372036854775808" "134217728" "0"
# ---- 3) elle yazilmis bozuk girdiler: cozucu C++ ile ayni karari vermeli
rej() { printf "$2" > $T/b.bin; $PM liste $T/b.bin > /dev/null 2>&1; ra=$?; $C < $T/b.bin > /dev/null 2>&1; rb=$?; agree $ra $rb && pass || fail "bozuk '$1' (C++ rc=$ra, pmy rc=$rb)"; }
rej bos ''; rej kisa 'PMB'; rej baslik_yanlis 'PMB2\x00\x10\x00\x00\x00\x00\x01'
rej bayrak2 'PMB1\x02\x10\x00\x00\x00\x00\x01'; rej mem0 'PMB1\x00\x00\x00\x00\x00\x00\x01'
rej mem_buyuk 'PMB1\x00\x81\x80\x80\x40\x00\x00\x00\x00\x00\x01'; rej giris_buyuk 'PMB1\x00\x10\xff\xff\x03\x00\x00\x00\x01'
rej nsabit_buyuk 'PMB1\x00\x10\x00\x7f'; rej leb_cok_uzun 'PMB1\x00\x80\x80\x80\x80\x80\x80\x80\x80\x80\x80\x01\x00'
rej leb_tasma 'PMB1\x00\xff\xff\xff\xff\xff\xff\xff\xff\xff\x03\x00'; rej leb_sonu_yok 'PMB1\x00\x80'
rej fazla_bayt 'PMB1\x00\x10\x00\x00\x00\x00\x00\xaa'; rej bss_buyuk 'PMB1\x00\x10\x00\x00\x00\x40\x00'
rej nfonk_buyuk 'PMB1\x00\x10\x00\x00\x00\x00\x81\x80\x04'; rej komut_kodu_bilinmeyen 'PMB1\x00\x10\x00\x00\x00\x00\x01\x00\x01\x01\xff'
rej np_buyuk_nr 'PMB1\x00\x10\x00\x00\x00\x00\x01\x05\x02\x01\x24'
# ---- 4) kesilmis onekler: gecerli dosyalarin her oneki
for p in $(ls $corp/*.pmb | head -3); do sz=$(stat -c %s $p); for i in $(seq 0 $((sz-1))); do head -c $i $p > $T/pre.bin; $PM liste $T/pre.bin > /dev/null 2>&1; ra=$?; $C < $T/pre.bin > /dev/null 2>&1; rb=$?; agree $ra $rb && pass || fail "onek $p/$i (C++ rc=$ra, pmy rc=$rb)"; done; done
# ---- 5) bozulmus kopyalar (fuzz): kabul/ret karari ve kabul edilince cikti ayni
m=0; for p in $(ls $corp/*.pmb | awk 'NR % 7 == 1' | head -8); do mkdir -p $T/mu; rm -f $T/mu/*; $T/mut $p $T/mu $MUT $((m+1)); m=$((m+1))
  for q in $T/mu/*.bin; do $PM liste $q > $T/r.txt 2>/dev/null; ra=$?; $C < $q > $T/c.txt 2>/dev/null; rb=$?
    if [ $ra = 0 ] && [ $rb = 0 ]; then cmp -s $T/r.txt $T/c.txt && pass || { fail "mutant cikti farkli $(basename $q)"; cp $q /tmp/mutant_fark_$m.bin; }; else agree $ra $rb && pass || { fail "mutant karar farkli (C++ rc=$ra, pmy rc=$rb)"; cp $q /tmp/mutant_fark_$m.bin; }; fi; done; done
rm -rf $T; echo "PMB1 kodlayici+cozucu (parametre): gecen $ok, kalan $bad"; [ $bad = 0 ]
