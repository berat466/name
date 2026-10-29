#!/bin/bash
# dogrula_float.sh'in yakaladigini kanitlar: sablon.txt'te ondalik sablonlarini tek tek bozar, gen2.pmb'yi yeniden uretir, denetimin BASARISIZ olmasini bekler.
# Kullanim: sabotaj_float.sh [pm]   (SADECE: etiket deseni)
# Not: `putf_sifir_kirp` (k=16 baslangici: okunan 16. basamak sozcugu hep 0) ve `putf_tahmin` (10 tabanli us tahmini 1233 -> 1200: ayarlama dongusu duzeltir) bilerek yok: gozlenebilir davranis ayni (esdeger mutantlar).
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
yakalandi=0; kacti=0
mutview() {   # tabloyu (okunabilir gorunum uzerinden) kopya agacta degistirir: x86 sablonlari tabloda
  { echo "@@gor"; cat $T/w/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb > $T/g.txt && perl -0pe "$1" $T/g.txt > $T/g2.txt && ! cmp -s $T/g.txt $T/g2.txt && { echo "@@yaz"; cat $T/g2.txt; } | $PM run $R/oz/arac/komut.pmb > $T/w/oz/komut/komutlar.pmk
}
mut() {   # etiket dosya ifade (dosya: sablon.txt | rt.pma | gen2.pma.in)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/src $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $T/w/oz/; cp -r $R/oz/arac $R/oz/elf2 $T/w/oz/
  { if [ "$2" = sablon.txt ]; then mutview "$3"; else (cd $T/w/oz/elf2 && perl -0pi -e "$3" $2 && ! cmp -s $2 $D/$2); fi; } || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  { case "$2" in gen2.pma.in) (cd $T/w/oz/nesne && TOOLS=gen2 GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1);; rt.pma) (cd $T/w/oz/elf2 && $PM asm rt.pma -o rt.pmb --strip > /dev/null 2>&1 && ./uret.sh $PM > /dev/null 2>&1);; *) (cd $T/w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1);; esac; } || { echo "mutant $1: uretici derlenemedi"; kacti=$((kacti+1)); return; }
  if GEN2=$T/w/oz/elf2/gen2.pmb $D/dogrula_float.sh $PM > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut fsub_op sablon.txt 's/tpl op FSUB : F2 0F 10 83 R_B F2 0F 5C/tpl op FSUB : F2 0F 10 83 R_B F2 0F 58/'
mut fdiv_op sablon.txt 's/tpl op FDIV : F2 0F 10 83 R_B F2 0F 5E/tpl op FDIV : F2 0F 10 83 R_B F2 0F 59/'
mut feq_nan sablon.txt 's/0F 9B C0 0F 94 C1 20 C8 0F B6 C0/0F 94 C0 90 90 90 90 0F B6 C0/'
mut flt_cc sablon.txt 's/(tpl op FLT : [^\n]*) 0F 97 C0/$1 0F 93 C0/'
mut fle_cc sablon.txt 's/(tpl op FLE : [^\n]*) 0F 93 C0/$1 0F 97 C0/'
mut fle_sira sablon.txt 's/tpl op FLE : F2 0F 10 83 R_C 66 0F 2E 83 R_B/tpl op FLE : F2 0F 10 83 R_B 66 0F 2E 83 R_C/'
mut f2i_nan sablon.txt 's/EB 02 31 C0 48 89 83 R_A/EB 02 90 90 48 89 83 R_A/'
mut f2i_isaret sablon.txt 's/78 0A 48 89 C8/79 0A 48 89 C8/'
mut f2i_tasma sablon.txt 's/48 89 C8 48 FF C8 EB 02/48 89 C8 90 90 90 EB 02/'
mut i2f_32 sablon.txt 's/tpl op I2F : F2 48 0F 2A/tpl op I2F : F2 90 0F 2A/'
mut fneg_bit sablon.txt 's/48 0F BA F8 3F/48 0F BA F8 3E/'
mut fsqrt_op sablon.txt 's/F2 0F 51 83 R_B/F2 0F 5E 83 R_B/'
mut putf_tie rt.pma 's/ld t, rb, 55\n    andi t, t, 1\n    jz t, nord/ld t, rb, 55\n    andi t, t, 1\n    jz zr, nord/'
mut putf_yuvarla rt.pma 's/lti z, c, 0\n    jnz z, nord\n    jnz c, up/lti z, c, 0\n    jnz z, nord\n    jmp nord/'
mut putf_basamak rt.pma 's/gei z, i, 15\n    jnz z, g4/gei z, i, 16\n    jnz z, g4/'
mut putf_ust_esik rt.pma 's/gei z, x0, 15\n    jnz z, expf/gei z, x0, 16\n    jnz z, expf/'
mut putf_alt_esik rt.pma 's/lti z, x0, -4\n    jnz z, expf/lti z, x0, -5\n    jnz z, expf/'
mut putf_us_isaret rt.pma 's/e2p: call x, wb, 43/e2p: call x, wb, 32/'
mut putf_us_iki_hane rt.pma 's/e3: lti z, ax, 10/e3: lti z, ax, 1/'
mut putf_nan_isaret rt.pma 's/jz sg, pos\n    call x, wb, 45/jz sg, pos\n    jz sg, pos/'
mut putf_sifir_pad rt.pma 's/f1w: addi c, dg, 48/f1w: addi c, dg, 49/'
mut putf_alt_normal rt.pma 's/addi e2, ex, -1075/addi e2, ex, -1074/'
mut putf_borc rt.pma 's/mov br, 1\n    add x, x, big/mov br, 0\n    add x, x, big/'
mut putf_kare gen2.pma.in 's/eqi z, h, \@H_PUTF\@\n    jnz z, srh/eqi z, h, 99\n    jnz z, srh/'
echo "sabotaj(ondalik): $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
