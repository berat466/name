#!/bin/bash
# dogrula_ondalik.sh'in yakaladigini kanitlar: asm.pma.in'deki fbits/fb_* kodunu tek tek bozar (mutant), asm.pmb'yi yeniden uretir, denetimin BASARISIZ olmasini bekler.
# Not: `drop_sinir` (drop >= 60 kisayolu) bilerek yok: drop 60..63 icin q >> drop = 0 ve rem < half oldugundan sonuc zaten 0 (esdeger mutant).
# Not: eski `tasima_m` (yalniz `shri m, m, 1` silme) esdegerdi (sonraki `and m, m, 2^52-1` ayni biti atar); simdi `addi eb` silinir.
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); yakalandi=0; kacti=0
mut() {   # etiket ifade
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/src $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $T/w/oz/; cp -r $R/oz/arac $R/oz/asm $T/w/oz/
  (cd $T/w/oz/asm && perl -0pi -e "$2" asm.pma.in && ! cmp -s asm.pma.in $D/asm.pma.in) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/nesne && TOOLS=asm GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: derlenemedi"; kacti=$((kacti+1)); return; }
  if ASMPMB=$T/w/oz/asm/asm.pmb $D/dogrula_ondalik.sh $PM ${NF:-40} > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut tie_cift       's/andi t, m, 1\n    jz t, nrd/andi t, m, 1\n    jz zr, nrd/'
mut tie_yapisik    's/    jnz sb, up\n/    jmp nrd\n/'
mut alt_normal     's/mov t, -1022\n    sub t, t, eb/mov t, -1021\n    sub t, t, eb/'
mut tasma_esik     's/gti z, mag, 309/gti z, mag, 308/'
mut sifir_esik     's/lti z, mag, -323/lti z, mag, -322/'
mut yari_buyuk     's/gt z, rem, half\n    jnz z, up/ge z, rem, half\n    jnz z, up/'
mut yapisik_basamak 's/jz sticky, h1\n    add a, nsig, FDG/jz sticky, h1\n    jmp h1\n    add a, nsig, FDG/'
mut sifir_kirp     's/mov nsig, t\n    addi e10, e10, 1/mov nsig, t\n    addi e10, e10, 0/'
mut us_isaret      's/g3: jz es, g4\n    neg ev, ev/g3: jz es, g4\n    mov ev, ev/'
mut tasima_m       's/shri m, m, 1\n    addi eb, eb, 1/shri m, m, 1\n    addi eb, eb, 0/'
mut odunc          's/mov br, 1\n    add x, x, big\ns4: st x, p, 1\n    addi i, i, 1\n    jmp s1\ns5: jz la, s7\n    addi t, la, -1\n    add p, a, t\n    add p, p, FBN/mov br, 0\n    add x, x, big\ns4: st x, p, 1\n    addi i, i, 1\n    jmp s1\ns5: jz la, s7\n    addi t, la, -1\n    add p, a, t\n    add p, p, FBN/'
mut kesir_us       's/jz infrac, f8\n    addi e10, e10, -1\n    jmp f8\nf5:/jz infrac, f8\n    jmp f8\nf5:/'
mut bas_sifir      's/jz infrac, f8\n    addi e10, e10, -1\n    jmp f8\nf4:/jz infrac, f8\n    jmp f8\nf4:/'
mut tam_basamak    's/f6: jnz infrac, f8\n    addi e10, e10, 1/f6: jnz infrac, f8\n    addi e10, e10, 0/'
mut isaret_sifir   's/jnz nsig, h0\n    st sgn, G, 13/jnz nsig, h0\n    st zr, G, 13/'
mut shr1_tasima    's/shli w, w, 31/shli w, w, 30/'
mut bolme_bit      's/mov bit, 57\ni3:/mov bit, 56\ni3:/'
echo "sabotaj(asm ondalik): $yakalandi yakalandi, $kacti kacti"
rm -rf $T; [ $kacti = 0 ]
