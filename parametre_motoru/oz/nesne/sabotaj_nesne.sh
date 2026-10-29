#!/bin/bash
# dogrula_nesne.sh'in yakaladigini kanitlar: baglayici.pma'yi tek tek bozar (mutant), kendi kucuk agacinda yeniden derler, dogrula_nesne.sh'in BASARISIZ olmasini bekler;
# ayrica sayisal nesnelerin (.pmo) sayilarini bozar (bagil kayit katsayisi/konumu, anahtar turu) ve ayni denetimi bekler.
# Kullanim: sabotaj_nesne.sh [pm]   (SADECE: etiket deseni)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/..; T=$(mktemp -d); yakalandi=0; kacti=0
taban() { rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/nesne $R/komut $R/arac $R/asm $R/dogr $R/elf2 $T/w/oz/; }
sonuc() {   # etiket
  if (cd $T/w/oz/nesne && ./dogrula_nesne.sh $PM > $T/log 2>&1); then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut() {   # etiket ifade (baglayici.pma)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  taban
  (cd $T/w/oz/arac && perl -0pi -e "$2" baglayici.pma && ! cmp -s baglayici.pma $R/arac/baglayici.pma) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/arac && $PM asm baglayici.pma -o baglayici.pmb --strip > /dev/null 2>&1) || { echo "mutant $1: derlenemedi"; kacti=$((kacti+1)); return; }
  sonuc "$1"
}
mutnesne() {   # etiket nesne ifade (awk, nesne sayilari uzerinde)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  taban
  awk "$3" $R/nesne/$2.pmo > $T/w/oz/nesne/$2.pmo.yeni; cmp -s $T/w/oz/nesne/$2.pmo.yeni $R/nesne/$2.pmo && { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  mv $T/w/oz/nesne/$2.pmo.yeni $T/w/oz/nesne/$2.pmo
  sonuc "$1"
}
mut skaler_eksi 's/neg sv, sv\n    call x, spc/call x, spc/'
mut skaler_aralik 's/sub d1, v, s1\n    lti z, d1, 0\n    jnz z, t1\n    ge z, d1, nk/sub d1, v, s1\n    lti z, d1, 0\n    jnz z, t1\n    gt z, d1, nk/'
mut skaler_tur 's/ld kd, a, 0\n    jnz kd, t1\n    call x, scal, d1/ld kd, a, 0\n    call x, scal, d1/'
mut dizi_basi 's/andi lo, d2, 65535\n    jnz lo, t3/andi lo, d2, 65535\n    jnz zr, t3/'
mut dizi_say 's/a1: ge z, i, c\n    jnz z, a9/a1: gt z, i, c\n    jnz z, a9/'
mut bagil_katsayi 's/mul dl, dl, cf/add dl, dl, cf/'
mut bagil_taban 's/sub dl, act, bs/sub dl, bs, act/'
mut bagil_dizi 's/t5: add a, di, DC\n    ld act, a, 0/t5: add a, di, DL\n    ld act, a, 0/'
mut bagil_skaler 's/add a, o, DV\n    ld act, a, 0\n    jmp t6/mov act, 0\n    jmp t6/'
mut bagil_zincir 's/add a, r, RX\n    ld hh, a, 0\n    jmp t4/mov hh, 0\n    jmp t4/'
mut satir_sonu 's/host putc, 10\n    mov z0, 0\n    st z0, G, 3/mov z0, 0\n    st z0, G, 3/'
mut bosluk 's/s1: mov one, 1\n    st one, G, 3\n    ret 0/s1: ret 0/'
mut sihirli 's/eq z, mg, 1347243825\n    jnz z, o1/jmp o1\n    jnz z, o1/'
mut eksik_anahtar 's/lti z, fnd, 0\n    jz z, k7\n    call x, err, 11/jmp k7\n    call x, err, 11/'
mut skaler_cok_deger 's/eqi z, cc, 1\n    jnz z, k8\n    call x, err, 12/jmp k8\n    call x, err, 12/'
mut dokunulmamis 's/t8a: ge z, i, e2\n    jnz z, t9a/t8a: ge z, i, p\n    jnz z, t9a/'
mut sozluk_sayi 's/d2f: add a, nd, DC\n    st cnt, a, 0/d2f: add a, nd, DC\n    addi cnt, cnt, 1\n    st cnt, a, 0/'
mut konum_sayaci 's/t9: addi pos, pos, 1/t9: addi pos, pos, 2/'
mutnesne nesne_katsayi asm 'NR==2+22{print;next} /^[0-9]+ [0-9]+ 1$/ && !d {$3=2; d=1} {print}'
mutnesne nesne_anahtar gen2 'NR==2{$1=1-$1} {print}'
mutnesne nesne_konum valid 'NR==1{print;next} NF==3 && $3==1 && !d {$1=$1+1; d=1} {print}'
echo "sabotaj(nesne/baglayici): $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
