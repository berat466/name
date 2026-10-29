#!/bin/bash
# dogrula_mikro.sh'in yakaladigini kanitlar: sablon.txt'te MIKRO-ISLEM sablonlarini (p_<islem>) tek tek bozar, gen2.pmb'yi yeniden uretir, denetimin BASARISIZ olmasini bekler.
# Kullanim: sabotaj_mikro.sh [pm]   (SADECE: etiket deseni)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
yakalandi=0; kacti=0
mutview() {   # tabloyu (okunabilir gorunum uzerinden) kopya agacta degistirir: x86 sablonlari tabloda
  { echo "@@gor"; cat $T/w/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb > $T/g.txt && perl -0pe "$1" $T/g.txt > $T/g2.txt && ! cmp -s $T/g.txt $T/g2.txt && { echo "@@yaz"; cat $T/g2.txt; } | $PM run $R/oz/arac/komut.pmb > $T/w/oz/komut/komutlar.pmk
}
mut() {   # etiket ifade (tablonun okunabilir gorunumu uzerinde)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/src $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $T/w/oz/; cp -r $R/oz/arac $R/oz/elf2 $T/w/oz/
  mutview "$2" || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: uretici derlenemedi"; kacti=$((kacti+1)); return; }
  if ELF2DIR=$T/w/oz/elf2 GEN2=$T/w/oz/elf2/gen2.pmb $D/dogrula_mikro.sh $PM 40 > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut sub_op 's/(tpl prim alu_sub :.*?) 48 29 C8/$1 48 01 C8/'
mut mul_op 's/(tpl prim alu_mul :.*?) 48 0F AF C1/$1 48 21 C8 90/'
mut xor_op 's/(tpl prim alu_xor :.*?) 48 31 C8/$1 48 09 C8/'
mut shl_op 's/(tpl prim alu_shl :.*?) 48 D3 E0/$1 48 D3 E8/'
mut sar_op 's/(tpl prim alu_sar :.*?) 48 D3 F8/$1 48 D3 E8/'
mut cmp_lt_cc 's/(tpl prim cmp_lt :.*?) 0F 9C C0/$1 0F 9E C0/'
mut cmp_le_cc 's/(tpl prim cmp_le :.*?) 0F 9E C0/$1 0F 9C C0/'
mut cmp_gt_cc 's/(tpl prim cmp_gt :.*?) 0F 9F C0/$1 0F 9D C0/'
mut cmp_ge_cc 's/(tpl prim cmp_ge :.*?) 0F 9D C0/$1 0F 9F C0/'
mut cmp_eq_cc 's/(tpl prim cmp_eq :.*?) 0F 94 C0/$1 0F 95 C0/'
mut un_not 's/(tpl prim un_not :.*?) 48 F7 D0/$1 48 F7 D8/'
mut divs_sifir 's/(tpl prim divs :.*?) 0F 84 REL_DIV0/$1 90 90 90 90 90 90/'
mut divs_eksi1 's/(tpl prim divs :.*?) 48 83 F9 FF 75 05/$1 48 83 F9 FE 75 05/'
mut mods_sifir 's/(tpl prim mods :.*?) 0F 84 REL_DIV0/$1 90 90 90 90 90 90/'
mut mods_eksi1 's/(tpl prim mods :.*?) 75 04 31 C0/$1 75 04 90 90/'
mut divn_op 's/(tpl prim divn :.*?) 48 99 48 F7 F9 48 89 44/$1 48 99 48 F7 F9 48 89 54/'
mut modn_op 's/(tpl prim modn :.*?) 48 89 D0 48 89 44/$1 48 89 C8 48 89 44/'
mut ld_sinir 's/(tpl prim ld :.*?) 0F 83 REL_OOB/$1 90 90 90 90 90 90/'
mut ld_ofset 's/(tpl prim ld :.*?) 48 05 OPV_F2/$1 48 05 OPV_F1/'
mut st_sinir 's/(tpl prim st :.*?) 0F 83 REL_OOB/$1 90 90 90 90 90 90/'
mut st_deger 's/(tpl prim st :.*?) 48 8B 4C 24 TMP_F0/$1 48 8B 4C 24 TMP_F1/'
mut alloc_sinir 's/(tpl prim alloc :.*?) 0F 87 REL_ALLOC/$1 90 90 90 90 90 90/'
mut alloc_art 's/(tpl prim alloc :.*?) 49 01 C7/$1 90 90 90/'
mut rd_slot 's/tpl prim rd : 48 8B 83 OPR_F1/tpl prim rd : 48 8B 83 OPR_F0/'
mut wr_slot 's/(tpl prim wr :.*?) 48 89 83 OPR_F0/$1 48 89 83 OPR_F1/'
mut im_deger 's/tpl prim im : 48 C7 C0 OPV_F1/tpl prim im : 48 C7 C0 OPV_F0/'
mut lk_adres 's/tpl prim lk : 48 8B 04 25 KAD_F1/tpl prim lk : 48 8B 04 25 KAD_F0/'
mut add_gecici 's/tpl prim alu_add : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F2/tpl prim alu_add : 48 8B 44 24 TMP_F1 48 8B 4C 24 TMP_F1/'
mut sel_cc 's/(tpl prim sel :.*?) 48 0F 45 C1/$1 48 0F 44 C1/'
mut sel_sira 's/tpl prim sel : 48 8B 44 24 TMP_F3 48 8B 4C 24 TMP_F2/tpl prim sel : 48 8B 44 24 TMP_F2 48 8B 4C 24 TMP_F3/'
mut fsub_op 's/(tpl prim fsub :.*?) F2 0F 5C/$1 F2 0F 58/'
mut fdiv_op 's/(tpl prim fdiv :.*?) F2 0F 5E/$1 F2 0F 59/'
mut fsqrt_op 's/tpl prim fsqrt : F2 0F 51/tpl prim fsqrt : F2 0F 5E/'
mut fneg_bit 's/(tpl prim fneg :.*?) 48 0F BA F8 3F/$1 48 0F BA F8 3E/'
mut feq_nan 's/(tpl prim feq :.*?) 0F 9B C0 0F 94 C1 20 C8/$1 0F 94 C0 90 90 90 90/'
mut flt_cc 's/(tpl prim flt :.*?) 0F 97 C0/$1 0F 93 C0/'
mut flt_sira 's/tpl prim flt : F2 0F 10 44 24 TMP_F2 66 0F 2E 44 24 TMP_F1/tpl prim flt : F2 0F 10 44 24 TMP_F1 66 0F 2E 44 24 TMP_F2/'
mut fle_cc 's/(tpl prim fle :.*?) 0F 93 C0/$1 0F 97 C0/'
mut f2i_nan 's/(tpl prim f2i :.*?) EB 02 31 C0/$1 EB 02 90 90/'
mut f2i_isaret 's/(tpl prim f2i :.*?) 78 0A 48 89 C8/$1 79 0A 48 89 C8/'
mut i2f_op 's/tpl prim i2f : F2 48 0F 2A/tpl prim i2f : F2 90 0F 2A/'
# br_hedef (REL_TF0 -> REL_TF1) bilerek yok: 'br' mikro-isleminin 1. alani disindaki alanlar 0'dir, REL_TF1 ayni yuvaya (a) isaret eder = esdeger mutant
mut brz_kosul 's/(tpl prim brz : [^\n]*) 0F 84 REL_TF1/$1 0F 85 REL_TF1/'
mut brnz_kosul 's/(tpl prim brnz : [^\n]*) 0F 85 REL_TF1/$1 0F 84 REL_TF1/'
mut brz_deger 's/(tpl prim brz : 48 8B 44 24) TMP_F0/$1 TMP_F1/'
mut br_geri_yakit 's/tpl sp 26 br_back : 4D 85 F6 0F 84 REL_FUEL 49 FF CE E9 REL_TF0/tpl sp 26 br_back : E9 REL_TF0 90 90 90 90 90 90 90 90 90 90 90 90/'
mut brz_geri_atla 's/(tpl sp 27 brz_back : [^\n]*) 0F 85 11 00 00 00/$1 0F 84 11 00 00 00/'
mut brnz_geri_atla 's/(tpl sp 28 brnz_back : [^\n]*) 0F 84 11 00 00 00/$1 0F 85 11 00 00 00/'
mut brz_geri_hedef 's/(tpl sp 27 brz_back : [^\n]*) E9 REL_TF1/$1 E9 REL_TF0/'
echo "sabotaj(mikro-islem sablonlari): $yakalandi yakalandi, $kacti kacti"
[ -n "$KEEP" ] && echo "KEEP: $T" || rm -rf $T
[ $kacti = 0 ]
