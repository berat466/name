#!/bin/bash
# dogrula_grafik.sh'in gercekten yakaladigini kanitlar: kaynaklari (rt.pma, sablon.txt, gen2.pma.in) tek tek bozar (mutant), gen2.pmb'yi yeniden uretir, grafik denetiminin BASARISIZ olmasini bekler.
# Kullanim: sabotaj_grafik.sh [pm]   (her mutant: dosya | perl degistirme ifadesi (ilk eslesme))
# Not: iki mutant bilerek yok (gozlenebilir davranis degistirmez): `gt z, ptr, n` -> `ge` (ptr==n zaten W*H>kalan denetimiyle reddedilir) ve cikis tamponu esigi 32768 -> 32769 (tasan bayt yalniz uygulamanin 0. sozcugune yazilir, cikti ayni) ve piksel maskesi 24 -> 25 bit (hash, PPM ve ASCII tuketicileri zaten bayt maskeler; 25. bit gozlenemez).
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
yakalandi=0; kacti=0
mutview() {   # tabloyu (okunabilir gorunum uzerinden) kopya agacta degistirir: x86 sablonlari tabloda
  { echo "@@gor"; cat $T/w/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb > $T/g.txt && perl -0pe "$1" $T/g.txt > $T/g2.txt && ! cmp -s $T/g.txt $T/g2.txt && { echo "@@yaz"; cat $T/g2.txt; } | $PM run $R/oz/arac/komut.pmb > $T/w/oz/komut/komutlar.pmk
}
mut() {   # etiket dosya ifade   (SADECE: yalniz bu desenle eslesen etiketler)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  rm -rf $T/w; mkdir -p $T/w/src $T/w/oz; cp -r $R/oz/komut $T/w/oz/; cp -r $R/oz/arac $R/oz/elf2 $T/w/oz/
  { if [ "$2" = sablon.txt ]; then mutview "$3"; else (cd $T/w/oz/elf2 && perl -0pi -e "$3" $2 && ! cmp -s $2 $D/$2); fi; } || { echo "SABOTAJ HATASI: $1 uygulanamadi"; kacti=$((kacti+1)); return; }
  (cd $T/w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: uretici derlenemedi (yakalandi)"; yakalandi=$((yakalandi+1)); return; }
  if GEN2=$T/w/oz/elf2/gen2.pmb $D/dogrula_grafik.sh $PM > $T/log 2>&1; then echo "KACTI: $1"; kacti=$((kacti+1)); else echo "yakalandi: $1 ($(grep -c HATA $T/log) hata)"; yakalandi=$((yakalandi+1)); fi
}
mut ppm_esik rt.pma 's/gti z, w, 4096/gti z, w, 4097/'
mut alan_sinir rt.pma 's/gt z, wh, rem/ge z, wh, rem/'
mut hash_kanal rt.pma 's/shri t, px, 16/shri t, px, 15/'
mut azami_kare rt.pma 's/ge z, fr, mk/gt z, fr, mk/'
mut olay_kare rt.pma 's/gt z, kare, fr/ge z, kare, fr/'
mut ascii_esik rt.pma 's/lti z, m, 128/lti z, m, 127/'
mut zincir rt.pma 's/xor ch, ch, hh/add ch, ch, hh/'
mut rapor_x rt.pma 's/call x, wb, 120/call x, wb, 121/'
mut ppm_basl rt.pma 's/80, 54, 10, 0/80, 53, 10, 0/'
mut hex_harf rt.pma 's/addi c, nb, 87/addi c, nb, 55/'
mut olay_kodu rt.pma 's/ld code, a, 65/ld code, a, 64/'
mut tarama_frame gen2.pma.in 's/lei z, h, 6/lei z, h, 5/'
mut tarama_poll gen2.pma.in 's/lti z, h, 4\n    jnz z, sr1\n/lti z, h, 6\n    jnz z, sr1\n/'
mut bayrak_rapor gen2.pma.in 's/jz fg, nfg\n    mov u, 1/jz fg, nfg\n    mov u, 0/'
mut ldp_sinir sablon.txt 's/(ldp : 48 8B 83 R_B 48 05 I_C 48 05 RTW 48 3D )LIMRT/$1RTW/'
mut exitc_kod sablon.txt 's/exitc : 45 85 C9 74 12 44 89 CE 44 89 D2 B8 01 00 00 00 BF 02 00 00 00 0F 05 44 89 FF/exitc : 45 85 C9 74 12 44 89 CE 44 89 D2 B8 01 00 00 00 BF 02 00 00 00 0F 05 BF 00 00 00 00/'
mut exitc_rt sablon.txt 's/exitc_rt : 48 C7 C3 STK4 45 31 ED E8 REL_RTEND/exitc_rt : 48 C7 C3 STK4 45 31 ED/'
mut sysw_fd sablon.txt 's/host_sysw : 48 8B BB R_B 48 8B B3 R_B8/host_sysw : 48 8B BB R_B8 48 8B B3 R_B8/'
mut event_sira gen2.pma.in 's/st k, a, 0\n    st cd, a, 1/st cd, a, 0\n    st k, a, 1/'
mut hash_ofset gen2.pma.in 's/mov v, -3750763034362895579\n    call x, eb64, v\n    call x, eb64, z0/mov v, -3750763034362895578\n    call x, eb64, v\n    call x, eb64, z0/'
echo "sabotaj: $yakalandi yakalandi, $kacti kacti"
rm -rf $T
[ $kacti = 0 ]
