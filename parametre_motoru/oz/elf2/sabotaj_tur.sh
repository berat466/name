#!/bin/bash
# dogrula_tur.sh'in yakaladigini kanitlar: tamsayi turu ailelerinin SEM kayitlarini (komut tablosu) ve svm'nin uint64 bolme yardimcisini tek tek bozar (mutant),
# kendi kopya agacinda gen2'yi yeniden uretir ve dogrula_tur.sh'in BASARISIZ olmasini bekler. Kullanim: sabotaj_tur.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-3}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
sonuc() { if (cd $1/oz/elf2 && LIMS=200 timeout 600 ./dogrula_tur.sh $PM ${NE:-0} ${NS:-14} > $T/log_$2 2>&1); then echo "KACTI: $2"; else echo "yakalandi: $2 ($(grep -c HATA $T/log_$2) hata)"; fi; }
mut_tablo() {   # etiket sed_ifadesi (komut tablosu okunabilir gorunumu)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    { echo "@@gor"; cat $R/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb | sed "$2" | { echo "@@yaz"; cat; } | $PM run $R/oz/arac/komut.pmb > $w/oz/komut/komutlar.pmk
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
mut_svm() {   # etiket sed_ifadesi (oz/sayisal/svm.pma)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    (cd $w/oz/sayisal && sed -i "$2" svm.pma && ! cmp -s svm.pma $R/oz/sayisal/svm.pma) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; "$@" & }
bg mut_tablo sext8_yanlis_genislik 's/^sem SEXT8 rd t1 b ; sx8 t2 t1/sem SEXT8 rd t1 b ; sx16 t2 t1/'
bg mut_tablo zext8_isaretli 's/^\(sem ZEXT8 rd t1 b ; \)zx8 t2 t1/\1sx8 t2 t1/'
bg mut_tablo add_i8_sarma_yok 's/^\(sem ADD_I8 .*alu_add t3 t1 t2\) ; sx8 t4 t3 ; wr a t4/\1 ; wr a t3/'
bg mut_tablo mul_i16_genislik 's/^\(sem MUL_I16 .*\)sx16 t4 t3/\1sx32 t4 t3/'
bg mut_tablo div_i32_isaretsiz 's/^\(sem DIV_I32 rd t1 b ; rd t2 c ; \)divs t3/\1divu t3/'
bg mut_tablo shl_u8_genislik 's/^\(sem SHL_U8 .*\)zx8 t4 t3/\1zx16 t4 t3/'
bg mut_tablo neg_u16_isaretli 's/^\(sem NEG_U16 .*\)zx16 t3 t2/\1sx16 t3 t2/'
bg mut_tablo not_u32_genislik 's/^\(sem NOT_U32 .*\)zx32 t3 t2/\1zx16 t3 t2/'
bg mut_tablo modu_bolme 's/^\(sem MODU rd t1 b ; rd t2 c ; \)modu t3/\1divu t3/'
bg mut_tablo ltu_leu 's/^\(sem LTU rd t1 b ; rd t2 c ; \)cmp_ltu t3/\1cmp_leu t3/'
bg mut_tablo u2f_yuvarlama 's/^\(sem U2F .*alu_and t4 t1 t2 ; \)alu_or t5 t3 t4/\1alu_or t5 t3 t3/'
bg mut_tablo f2u_esik 's/^\(sem F2U rd t1 b ; \)lit t2 1086/\1lit t2 1087/'
bg mut_svm svm_udiv_kaydir 's/shri h, x, 1$/shri h, x, 2/'
bg mut_svm svm_udiv_duzeltme 's/^\(func udiv(x, y) {\)$/\1\n    lti z, y, 0\n    jz z, uq\n    ret 7\nuq:/'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(tur): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
