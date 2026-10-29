#!/bin/bash
# dogrula_kural.sh'nin yakaladigini kanitlar: komut tablosundaki yeniden yazma kurali SAYILARINI tek tek bozar, asm'yi tablodan yeniden uretir, denetimin BASARISIZ olmasini bekler.
# Kullanim: sabotaj_kural.sh [pm]   (SADECE: etiket deseni)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
yakalandi=0; kacti=0
mod() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $R/oz/arac/komut.pmb); }
mut() {   # etiket sed_ifadesi (okunabilir gorunumdeki 'section 7' satiri uzerinde)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $R/oz/arac $R/oz/asm $T/w/oz/; rm -f $T/w/oz/asm/asm.pmb
  mod gor < $R/oz/komut/komutlar.pmk > $T/g.txt; sed -E "$2" $T/g.txt > $T/g2.txt
  cmp -s $T/g.txt $T/g2.txt && { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  mod yaz < $T/g2.txt > $T/w/oz/komut/komutlar.pmk
  [ "$(mod dogrula < $T/w/oz/komut/komutlar.pmk)" = "1" ] || { echo "mutant $1: tablo gecersiz (dogrulama yakaladi)"; yakalandi=$((yakalandi+1)); return; }
  (cd $T/w/oz/asm && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: asm uretilemedi"; kacti=$((kacti+1)); return; }
  if ASM=$T/w/oz/asm/asm.pmb $D/dogrula_kural.sh $PM > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
M=-2147483648; X=2147483647
mut sub_aralik "/^section 7/ s/ 1 -2147483647 2147483647 2 3 0 1 41 / 1 -2147483648 2147483647 2 3 0 1 41 /"
mut div_sifir "/^section 7/ s/ 1 1 2147483647 1 1 47 / 1 0 2147483647 1 1 47 /"
mut div_eksi1 "/^section 7/ s/ 1 -2147483648 -2 1 1 47 / 1 -2147483648 -1 1 1 47 /"
mut mod_sifir "/^section 7/ s/ 1 1 2147483647 1 1 48 / 1 0 2147483647 1 1 48 /"
mut mod_eksi1 "/^section 7/ s/ 1 -2147483648 -2 1 1 48 / 1 -2147483648 -1 1 1 48 /"
mut lt_takas "/^section 7/ s/ 2 -2147483648 2147483647 2 2 0 1 53 / 2 -2147483648 2147483647 2 3 0 1 53 /"
mut lt_ayna "/^section 7/ s/ 2 -2147483648 2147483647 2 2 0 1 53 / 2 -2147483648 2147483647 2 2 0 1 51 /"
mut le_anlik "/^section 7/ s/ 1 -2147483648 2147483647 1 1 52 / 1 -2147483648 2147483647 1 1 51 /"
mut gt_son "/^section 7/ s/ 3 0 0 2 2 0 1 16 / 3 0 0 2 2 0 1 17 /"
mut ge_anlik "/^section 7/ s/ 1 -2147483648 2147483647 1 1 54 / 1 -2147483648 2147483647 1 1 53 /"
mut ge_ayna "/^section 7/ s/ 2 -2147483648 2147483647 2 2 0 1 52 / 2 -2147483648 2147483647 2 2 0 1 54 /"
mut fgt_hedef "/^section 7/ s/ 3 0 0 2 2 0 1 27 / 3 0 0 2 2 0 1 28 /"
mut fge_hedef "/^section 7/ s/ 3 0 0 2 2 0 1 28 / 3 0 0 2 2 0 1 27 /"
mut shl_hedef "/^section 7/ s/ 1 -2147483648 2147483647 1 1 44 / 1 -2147483648 2147483647 1 1 45 /"
mut sar_hedef "/^section 7/ s/ 1 -2147483648 2147483647 1 1 46 / 1 -2147483648 2147483647 1 1 45 /"
mut add_ikinci "/^section 7/ s/ 2 -2147483648 2147483647 2 2 0 1 41 / 2 -2147483648 2147483647 1 1 41 /"
mut mul_takas "/^section 7/ s/ 2 -2147483648 2147483647 2 2 0 1 42 / 2 -2147483648 2147483647 2 3 0 1 42 /"
mut eq_hedef "/^section 7/ s/ 1 -2147483648 2147483647 1 1 49 / 1 -2147483648 2147483647 1 1 50 /"
mut and_aralik "/^section 7/ s/ 1 -2147483648 2147483647 1 1 43 / 1 0 2147483647 1 1 43 /"
echo "sabotaj(yeniden yazma kurallari): $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
