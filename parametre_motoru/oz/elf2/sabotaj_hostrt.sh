#!/bin/bash
# dogrula_hostrt.sh'in yakaladigini kanitlar: host_rt / host_rtr sablonlarini (RT cercevesi cagiran cerceve sonunda, arguman kopyasi, sonuc pencere[0], rbx geri yukleme) ve gen2 hostmap kodunu tek tek bozar; mutant kopya agacta gen2'yi yeniden uretir, dogrula_hostrt.sh'in BASARISIZ olmasini bekler.
# kendi kopya agacinda gen2'yi yeniden uretir ve dogrula_hostrt.sh'in BASARISIZ olmasini bekler. Kullanim: sabotaj_cagri.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-3}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); . $D/../slot.sh; [ -n "$HIZLI_SLOT" ] && JOBS=1000
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
mut_tablo() {   # etiket sed_ifadesi (komut tablosu okunabilir gorunumu)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    { echo "@@gor"; cat $R/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb | sed "$2" | { echo "@@yaz"; cat; } | $PM run $R/oz/arac/komut.pmb > $w/oz/komut/komutlar.pmk
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    [ "$(echo "@@dogrula"; cat $w/oz/komut/komutlar.pmk)" ] ; (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    if $D/../ilkhata.sh $T/log_$1 bash -c "cd $w/oz/elf2 && TAB=$w/oz/komut/komutlar.pmk timeout 900 bash -c './dogrula_hostrt.sh $PM; ./dogrula_grafik.sh $PM'"; then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
mut_gen2() {   # etiket perl_ifadesi (oz/elf2/gen2.pma.in)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    (cd $w/oz/elf2 && perl -0pi -e "$2" gen2.pma.in && ! cmp -s gen2.pma.in $R/oz/elf2/gen2.pma.in) || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    (cd $w/oz/nesne && TOOLS=gen2 GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    if $D/../ilkhata.sh $T/log_$1 bash -c "cd $w/oz/elf2 && timeout 900 bash -c './dogrula_hostrt.sh $PM; ./dogrula_grafik.sh $PM'"; then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; ( slot_al; "$@"; slot_birak ) & }
# ---- tablo: host_rt sablonu (tpl sp 30) ve host_rtr (tpl sp 31)
bg mut_tablo rt_pencere_tabani '/^tpl sp 30 host_rt/s/ 48 8D 83 CURNR8 / 48 8D 83 R_B /'
bg mut_tablo rt_pencere_tabani_r '/^tpl sp 31 host_rtr/s/ 48 8D 83 CURNR8 / 48 8D 83 R_B /'
bg mut_tablo rt_rbx_geri_yok '/^tpl sp 30 host_rt/s/ 48 81 EB CURNR8 / /'
bg mut_tablo rt_arg0_kopya_yok '/^tpl sp 30 host_rt/s/ 48 8B 8B R_B 48 89 08 / /'
bg mut_tablo rt_arg1_kopya_yok '/^tpl sp 31 host_rtr/s/ 48 8B 8B R_B8 48 89 48 08 / /'
bg mut_tablo rt_arg2_kopya_yok '/^tpl sp 31 host_rtr/s/ 48 8B 8B R_B16 48 89 48 10 / /'
bg mut_tablo rt_sonuc_sifir_yok '/^tpl sp 30 host_rt/s/ 31 C0 48 89 83 R_B$//'
bg mut_tablo rt_sonuc_yaz_yok '/^tpl sp 31 host_rtr/s/ 48 89 83 R_B$//'
bg mut_tablo rt_yakit_yok '/^tpl sp 30 host_rt/s/^\(tpl sp 30 host_rt : \)4D 85 F6 0F 84 REL_FUEL 49 FF CE /\1/'
bg mut_tablo rt_derinlik_kaydet_yok '/^tpl sp 30 host_rt/s/ 41 55 4D 31 ED / /'
bg mut_tablo rt_derinlik_sifirla_yok '/^tpl sp 30 host_rt/s/ 41 55 4D 31 ED / 41 55 /'
bg mut_tablo rt_derinlik_geri_yok '/^tpl sp 30 host_rt/s/ 41 5D 48 8B 03 / 48 8B 03 /'
bg mut_tablo rt_yigin_siniri_yok '/^tpl sp 31 host_rtr/s/ 48 C7 C1 STKEND 48 29 C1 48 81 F9 NREGS8 0F 82 REL_STACK / /'
bg mut_tablo rt_sifirlama_yok '/^tpl sp 30 host_rt/s/ 48 89 C3 31 C0 F3 48 AB / 48 89 C3 /'
# ---- gen2 kodu
bg mut_gen2 hostmap_putf_eski 's/mov r, \@TS_HOST_RT\@\n    ret r\nhq:/mov r, \@TS_HOST_SYSW\@\n    ret r\nhq:/'
bg mut_gen2 hostmap_rtr_yok 's/mov r, \@TS_HOST_RTR\@/mov r, \@TS_HOST_RT\@/'
bg mut_gen2 val_curnr 's/(vn1: eqi z, k, 37\n.*?)mul r, r, 8/$1mul r, r, 4/s'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(hostrt): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
