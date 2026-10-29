#!/bin/bash
# dogrula_valid.sh'in yakaladigini kanitlar: valid.pma.in'de (komut ozellikleri tablodan, SEM'den turetilmis dizilerle okunan) kosullari tek tek bozar, valid.pmb'yi yeniden uretir (VALID=), denetimin BASARISIZ olmasini bekler.
# Kullanim: sabotaj_valid.sh [pm]   (SADECE: etiket deseni; NL/NB: dogrula_valid.sh mutant sayilari)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); yakalandi=0; kacti=0
mut() {   # etiket ifade
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $R/oz/arac $R/oz/dogr $R/oz/mutasyon.cpp $T/w/oz/ 2>/dev/null
  (cd $T/w/oz/dogr && perl -0pi -e "$2" valid.pma.in && ! cmp -s valid.pma.in $D/valid.pma.in) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/nesne && TOOLS=valid GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: derlenemedi"; kacti=$((kacti+1)); return; }
  if VALID=$T/w/oz/dogr/valid.pmb $D/dogrula_valid.sh $PM ${NL:-25} ${NB:-20} > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut sonlandirici_atlama 's/eqi z, fw, 2\n    jnz z, lastok\n    call x, rej, 15/call x, rej, 15/'
mut tur_anlik 's/andi t, kf, 4\n    jnz t, vnext/andi t, kf, 8\n    jnz t, vnext/'
mut etiket_arali 's/(klab:\n    lt z, v, 0\n    jnz z, vbad\n    )ge z, v, ncode/$1gt z, v, ncode/'
mut host_arali 's/ge z, v, \@HOSTCOUNT\@/gt z, v, \@HOSTCOUNT\@/'
mut bolen_sifir 's/jz c3, dmbad/jmp dmbad2/; s/dmbad:\n    call x, rej, 17/dmbad:\n    call x, rej, 17\ndmbad2:/'
mut bolen_eksi1 's/eq z, c3, -1\n    jz z, vcall/eq z, c3, -2\n    jz z, vcall/'
mut cagri_pencere 's/wc: add t, fb, need\n    gt z, t, nr/wc: add t, fb, need\n    gt z, t, 99999/'
mut okuma_bayrak 's/andi t, kf, 1\n    jnz t, srd/andi t, kf, 2\n    jnz t, srd/'
mut yazma_bayrak 's/andi t, kf, 2\n    jz t, s2n/andi t, kf, 1\n    jz t, s2n/'
mut kosullu_atlama 's/eqi z, fw, 3\n    jz z, s7/eqi z, fw, 2\n    jz z, s7/'
mut temizle 's/eqi z, cc, 1\n    jz z, s4/eqi z, cc, 2\n    jz z, s4/'
mut hedef_yuvasi 's/add a, op, OPTGT\n    ld tgs, a, 0/mov tgs, 0/'
mut arite_host 's/shq:\n    add a, fa, ARITY/shq:\n    add a, fa, FNP/'
echo "sabotaj(dogrulayici): $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
