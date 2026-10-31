#!/bin/bash
# dogrula_donusum.sh'in yakaladigini kanitlar: TYPES bolumundeki donusum kayitlarini (komut kimligi) ve donusum komutlarinin anlamini tek tek bozar (mutant),
# kendi kopya agacinda dogrula_donusum.sh'i calistirir ve BASARISIZ olmasini bekler. Kullanim: sabotaj_donusum.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-3}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
KM() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
sonuc() { if (cd $1/oz/elf2 && LIMS=200 timeout 600 ./dogrula_donusum.sh $PM 12 > $T/log_$2 2>&1); then echo "KACTI: $2"; else echo "yakalandi: $2 ($(grep -c HATA $T/log_$2) hata)"; fi; }
mut_conv() {   # etiket kaynak_tur_adi hedef_tur_adi yeni_komut_kimligi_ya_da_ad  (donusum kaydinin komut sozcugunu degistirir)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    KM tur < $R/oz/komut/komutlar.pmk > $T/tur_$1.txt
    fid=$(awk -v a=$2 '$1=="tur" && $3==a {print $2}' $T/tur_$1.txt); tid=$(awk -v a=$3 '$1=="tur" && $3==a {print $2}' $T/tur_$1.txt)
    nop=$4; case "$4" in -1|[0-9]*) ;; *) nop=$(KM gor < $R/oz/komut/komutlar.pmk | awk -v n=$(echo $4 | tr a-z A-Z) '$1=="op" && $3==n {print $2}');; esac
    KM kanon < $R/oz/komut/komutlar.pmk | awk -v f=$fid -v t=$tid -v nop=$nop 'BEGIN{h=0} NF==4 && $1==10 && $2==0 && $3==1 {h=NR} h && NR==h+1 { p=2; while (p<=NF) { rl=$p; if ($(p+1)==2 && $(p+2)==f && $(p+3)==t) { $(p+4)=nop } p+=rl+1 } } {print}' > $w/oz/komut/komutlar.pmk
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
mut_tablo() {   # etiket sed_ifadesi (okunabilir gorunum; gen2 yeniden uretilir)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    KM gor < $R/oz/komut/komutlar.pmk | sed "$2" | KM yaz > $w/oz/komut/komutlar.pmk
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; "$@" & }
bg mut_conv i32_i8_ozdes i32 i8 -1
bg mut_conv i8_u16_ozdes i8 u16 -1
bg mut_conv i64_bool_ozdes i64 bool -1
bg mut_conv u32_i32_ozdes u32 i32 -1
bg mut_conv u64_u8_sext i64 u8 sext8
bg mut_conv i16_i32_zext i16 i32 zext32
bg mut_conv u16_i8_zext u16 i8 zext8
bg mut_conv bool_u8_tobool bool u8 sext8
bg mut_conv i32_f32_cift i32 f32 i2f
bg mut_conv u64_f64_isaretli u64 f64 i2f
bg mut_conv i64_f16_ozdes i64 f16 -1
bg mut_conv f64_f32_ozdes f64 f32 -1
bg mut_conv f32_f16_ozdes f32 f16 -1
bg mut_conv f16_bf16_ozdes f16 bf16 -1
bg mut_conv f64_i8_dar f64 i8 f2i_i16
bg mut_conv f64_u32_isaretli f64 u32 f2i_i32
bg mut_conv f32_u64_isaretli f32 u64 f2i
bg mut_conv f64_bool_tamsayi f64 bool f2i
bg mut_conv i8_f16_u2f i8 f16 u2f_f16
bg mut_tablo tobool_eq 's/^\(sem TOBOOL rd t1 b ; lit t2 0 ; \)cmp_ne t3/\1cmp_eq t3/'
bg mut_tablo zext16_genislik 's/^\(sem ZEXT16 rd t1 b ; \)zx16 t2 t1/\1zx8 t2 t1/'
bg mut_tablo sext32_genislik 's/^\(sem SEXT32 rd t1 b ; \)sx32 t2 t1/\1sx16 t2 t1/'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(donusum): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
