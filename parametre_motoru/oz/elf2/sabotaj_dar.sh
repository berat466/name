#!/bin/bash
# dogrula_dar.sh'in yakaladigini kanitlar: dar kayan nokta komutlarinin SEM kayitlarini (komut tablosu) tek tek bozar (mutant), kendi kopya agacinda gen2'yi yeniden uretir
# ve dogrula_dar.sh'in BASARISIZ olmasini bekler.  Kullanim: sabotaj_dar.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-3}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); . $D/../slot.sh; [ -n "$HIZLI_SLOT" ] \&\& JOBS=1000
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
KM() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
mut_sem() {   # etiket KOMUT eski yeni   (SEM satirinda ilk eski metni yeniyle degistirir)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    KM gor < $R/oz/komut/komutlar.pmk | python3 -c "
import sys
op, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
done = False
for l in sys.stdin.read().split('\n'):
    if l.startswith('sem ' + op + ' ') and not done and old in l: l = l.replace(old, new, 1); done = True
    print(l)
if not done: sys.exit(5)
" "$2" "$3" "$4" | KM yaz > $w/oz/komut/komutlar.pmk || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
    cmp -s $w/oz/komut/komutlar.pmk $R/oz/komut/komutlar.pmk && { echo "SABOTAJ HATASI: $1 degisiklik yok"; exit 3; }
    (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    if $D/../ilkhata.sh $T/log_$1 bash -c "cd $w/oz/elf2 && LIMS=200 timeout 900 ./dogrula_dar.sh $PM 0 12"; then echo "KACTI: $1"; else echo "yakalandi: $1 ($(grep -c HATA $T/log_$1) hata)"; fi
    rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
# DENK (davranisi degistirmeyen) mutantlar bilincle YOK: cmp_lt->cmp_le (ef == EMIN esitliginde ayni secim), abs>=T -> abs>T (r>=T ayni durumu kapsar),
# C = 1.25*2^k (x >= 0 icin 1.5 sart degil), esik 53 -> 52 (2^52..2^53 araliginda yapiskanli yol da dogru sonuc verir).
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; ( slot_al; "$@"; slot_birak ) & }
bg mut_sem rnd_kaydir RND_F32 'lit t6 29' 'lit t6 28'
bg mut_sem rnd_kaydir16 RND_F16 'lit t6 42' 'lit t6 41'
bg mut_sem rnd_sessiz RND_F32 'lit t2 51 ; alu_shl t6 t6 t2 ; alu_or t7 t7 t6' 'lit t2 50 ; alu_shl t6 t6 t2 ; alu_or t7 t7 t6'
bg mut_sem rnd_nan_kosul RND_F32 'cmp_gt t2 t5 t1' 'cmp_ge t2 t5 t1'
bg mut_sem rnd_altnormal RND_F32 'lit t8 897' 'lit t8 896'
bg mut_sem rnd_altnormal16 RND_F16 'lit t8 1009' 'lit t8 1008'
bg mut_sem rnd_tasma RND_F32 'lit t8 1151' 'lit t8 1152'
bg mut_sem rnd_tasma16 RND_F16 'lit t8 1039' 'lit t8 1040'
bg mut_sem rnd_tasma_yuvarlanmis RND_F32 'cmp_ge t5 t3 t8' 'cmp_gt t5 t3 t8'
bg mut_sem rnd_isaret RND_F32 'alu_or t3 t3 t4' 'alu_or t3 t3 t3'
bg mut_sem rnd_sihirli RND_F32 'fsub t3 t3 t6' 'fadd t3 t3 t6'
bg mut_sem rnd_bf16 RND_BF16 'lit t8 897' 'lit t8 898'
bg mut_sem rnd_bf16_mant RND_BF16 'lit t6 45' 'lit t6 44'
bg mut_sem add_islem ADD_F32 'fadd t1 t1 t2' 'fsub t1 t1 t2'
bg mut_sem sub_islem SUB_F16 'fsub t1 t1 t2' 'fadd t1 t1 t2'
bg mut_sem mul_islem MUL_BF16 'fmul t1 t1 t2' 'fadd t1 t1 t2'
bg mut_sem div_islem DIV_F32 'fdiv t1 t1 t2' 'fmul t1 t1 t2'
bg mut_sem sqrt_islem SQRT_F16 'fsqrt t1 t1' 'fneg t1 t1'
bg mut_sem i2f_yapiskan I2F_F32 'alu_or t5 t5 t6' 'alu_xor t5 t5 t6'
bg mut_sem i2f_kaydir I2F_F32 'lit t2 11 ; alu_shr t5 t4 t2' 'lit t2 10 ; alu_shr t5 t4 t2'
bg mut_sem i2f_olcek I2F_F32 'lit t6 2048' 'lit t6 1024'
bg mut_sem i2f_esik I2F_F32 'lit t2 53 ; alu_shl t7 t7 t2' 'lit t2 30 ; alu_shl t7 t7 t2'
bg mut_sem i2f_isaretsiz_karsilastirma I2F_F32 'cmp_ltu t7 t4 t7' 'cmp_lt t7 t4 t7'
bg mut_sem i2f_mutlak I2F_F32 'alu_sub t4 t4 t3' 'alu_add t4 t4 t3'
bg mut_sem i2f_isaret I2F_F16 'sel t1 t7 t5 t6' 'sel t1 t7 t6 t6'
bg mut_sem u2f_esik U2F_F32 'lit t2 53 ; alu_shl t7 t7 t2' 'lit t2 54 ; alu_shl t7 t7 t2'
bg mut_sem u2f_yapiskan U2F_BF16 'cmp_ne t6 t6 t7' 'cmp_eq t6 t6 t7'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(dar): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
