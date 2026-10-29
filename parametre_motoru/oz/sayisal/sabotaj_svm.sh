#!/bin/bash
# dogrula_svm.sh'nin yakaladigini kanitlar: svm.pma'da dagitim/operand eslemesi/bellek/tuzak/ondalik kollarini tek tek bozar, svm.pmb'yi yeniden uretir,
# (HIZLI) denetimin BASARISIZ olmasini bekler. NOT: svm tamsayi/ondalik aritmetigi pm'nin VM'inde calisir; burada sinanan dagitim, esleme, bellek, tuzak ve bilesimdir.
# Kullanim: sabotaj_svm.sh [pm]   (SADECE: etiket deseni)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
yakalandi=0; kacti=0
mut() {   # etiket ifade
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  cp $D/svm.pma $T/svm.pma
  (cd $T && perl -0pi -e "$2" svm.pma && ! cmp -s svm.pma $D/svm.pma) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  $PM asm $T/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "mutant $1: derlenemedi"; kacti=$((kacti+1)); return; }
  if LIMS=8 SVM=$T/svm.pmb HIZLI=1 $D/dogrula_svm.sh $PM 3 > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut add_sub 's/add v, x, y\n    jmp a_st\na6:/sub v, x, y\n    jmp a_st\na6:/'
mut cmp_lt 's/lt v, x, y\n    jmp a_st\nc17:/le v, x, y\n    jmp a_st\nc17:/'
mut cmp_gt_sira 's/(c18: eqi z, pid, 18\n    jz z, c19\n    )lt v, y, x/$1lt v, x, y/'
mut cmp_ge 's/c19: le v, y, x/c19: lt v, y, x/'
mut un_neg 's/neg v, x\n    jmp a_st\nu21:/not v, x\n    jmp a_st\nu21:/'
mut divs_eksi1 's/neg v, x\n    jmp a_st\ndv_m1:/mov v, x\n    jmp a_st\ndv_m1:/'
mut mods_eksi1 's/dv_m1: mov v, 0/dv_m1: mov v, 1/'
mut div_sifir 's/jnz y, dv_nz/jmp dv_nz/'
mut mod_op 's/dv_2: eqi z, pid, 23\n    jz z, dv_3\n    mod v, x, y/dv_2: eqi z, pid, 23\n    jz z, dv_3\n    div v, x, y/'
mut ld_sinir 's/ge z, ad, memw\n    jnz z, ld_oob/gt z, ad, memw\n    jnz z, ld_oob/'
mut ld_negatif 's/lti z, ad, 0\n    jnz z, ld_oob/lti z, ad, -5\n    jnz z, ld_oob/'
mut st_sinir 's/ge z, ad, memw\n    jnz z, st_oob/gt z, ad, memw\n    jnz z, st_oob/'
mut st_ofset 's/(d_st: eqi z, pid, 27\n    jz z, d_br\n    add a, f0, TS\n    ld xv, a, 0\n    add a, f1, TS\n    ld x, a, 0\n    add a, f2, )OP3/$1TS/'
mut alloc_sinir 's/gt z, x, room\n    jnz z, al_bad/ge z, x, room\n    jnz z, al_bad/'
mut alloc_negatif 's/lti z, x, 0\n    jnz z, al_bad/lti z, x, -1\n    jnz z, al_bad/'
mut alloc_adres 's/st heap, a, 0\n    add heap, heap, x/add t0, heap, x\n    st t0, a, 0\n    add heap, heap, x/'
mut im_deger 's/(d_im: eqi z, pid, 2\n    jz z, d_lk\n    add a, f1, )OP3/$1TS/'
mut lk_havuz 's/add a, o, CK\n    ld v, a, 0/add a, o, RS\n    ld v, a, 0/'
mut wr_yuva 's/(d_wr: add a, )f0(, OP3)/$1f1$2/'
mut fsub_op 's/fsub v, x, y/fadd v, x, y/'
mut fneg_op 's/fneg v, x\n    jmp a_st\nfp41:/fsqrt v, x\n    jmp a_st\nfp41:/'
mut flt_op 's/flt v, x, y/fle v, x, y/'
mut feq_op 's/feq v, x, y/fle v, x, y/'
mut f2i_op 's/fp46: f2i v, x/fp46: mov v, x/'
mut sel_sira 's/mov v, y\n    jz c, a_st\n    mov v, x/mov v, x\n    jz c, a_st\n    mov v, y/'
mut sel_kosul 's/jz c, a_st\n    mov v, x/jnz c, a_st\n    mov v, x/'
echo "sabotaj(svm): $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
