#!/bin/bash
# dogrula_i128.sh'in yakaladigini kanitlar: 128 bit komutlarinin SEM kayitlarini (tablo), svm'nin mulhu/rdw/wrw uygulamasini ve gen2'nin OPW simgesini tek tek bozar (mutant);
# kendi kopya agacinda yeniden uretip dogrula_i128.sh'in BASARISIZ olmasini bekler. Kullanim: sabotaj_i128.sh [pm] [paralel]   (SADECE: etiket deseni)
PM=${1:-../../pm}; JOBS=${2:-4}; D=$(cd "$(dirname "$0")" && pwd); PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); . $D/../slot.sh; [ -n "$HIZLI_SLOT" ] \&\& JOBS=1000
taban() { rm -rf $1; mkdir -p $1; cp -r $R/oz $1/oz; }
KM() { { echo "@@$1"; cat; } | $PM run $R/oz/arac/komut.pmb; }
sonuc() { if $D/../ilkhata.sh $T/log_$2 bash -c "cd $1/oz/tur128 && LIMS=200 timeout 700 ./dogrula_i128.sh $PM 8 3" && $D/../ilkhata.sh $T/log_$2 bash -c "cd $1/oz/tur128 && LIMS=200 timeout 700 ./dogrula_i128.sh $PM 26 4"; then echo "KACTI: $2"; else echo "yakalandi: $2 ($(grep -c HATA $T/log_$2) hata)"; fi; }
mut_sem() {   # etiket KOMUT eski yeni
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
    (cd $w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; }
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
mut_dosya() {   # etiket dosya eski yeni (dosya agacta; degisimden sonra ilgili arac yeniden uretilir)
  [ -z "$SADECE" ] || [[ "$1" =~ $SADECE ]] || return
  ( w=$T/w_$1; taban $w
    python3 - "$w/$2" "$3" "$4" <<'PY' || { echo "SABOTAJ HATASI: $1 uygulanamadi"; exit 3; }
import sys
p, old, new = sys.argv[1], sys.argv[2], sys.argv[3]
s = open(p).read()
if old not in s: sys.exit(5)
open(p, 'w').write(s.replace(old, new, 1))
PY
    case "$2" in
      */svm.pma) ;;
      */gen2.pma.in) (cd $w/oz/nesne && TOOLS=gen2 GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: gen2 uretilemedi"; exit 4; } ;;
      */valid.pma.in) (cd $w/oz/nesne && TOOLS=valid GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: valid uretilemedi"; exit 4; } ;;
      */asm.pma.in) (cd $w/oz/nesne && TOOLS=asm GORUNUM=0 ./metinden.sh $PM > /dev/null 2>&1) || { echo "mutant $1: asm uretilemedi"; exit 4; } ;;
    esac
    sonuc $w $1; rm -rf $w ) > $T/sonuc_$1 2>&1; cat $T/sonuc_$1
}
bg() { while [ "$(jobs -rp | wc -l)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done; ( slot_al; "$@"; slot_birak ) & }
bg mut_sem add_tasima ADD_I128 'cmp_ltu t6 t5 t1' 'cmp_leu t6 t5 t1'
bg mut_sem add_yuksek ADD_U128 'alu_add t7 t7 t6' 'alu_add t7 t7 t7'
bg mut_sem sub_odunc SUB_I128 'cmp_ltu t6 t1 t3' 'cmp_ltu t6 t3 t1'
bg mut_sem sub_yuksek SUB_U128 'alu_sub t7 t7 t6' 'alu_add t7 t7 t6'
bg mut_sem mul_ust MUL_I128 'mulhu t6 t1 t3' 'alu_mul t6 t1 t3'
bg mut_sem mul_capraz1 MUL_U128 'alu_mul t7 t1 t4' 'alu_mul t7 t1 t3'
bg mut_sem mul_capraz2 MUL_I128 'alu_mul t7 t2 t3' 'alu_mul t7 t2 t4'
bg mut_sem neg_borc NEG_I128 'cmp_ne t4 t1 t3' 'cmp_eq t4 t1 t3'
bg mut_sem not_yuksek NOT_U128 'un_not t2 t2' 'un_neg t2 t2'
bg mut_sem and_yuksek AND_I128 'alu_and t2 t2 t4' 'alu_or t2 t2 t4'
bg mut_sem xor_dusuk XOR_U128 'alu_xor t1 t1 t3' 'alu_or t1 t1 t3'
bg mut_sem shl_buyuk SHL_I128 'sel t8 t4 t6 t7' 'sel t8 t4 t7 t6'
bg mut_sem shl_sayac SHL_U128 'lit t5 127' 'lit t5 63'
bg mut_sem shl_carpraz SHL_I128 'alu_shr t8 t1 t5 ; lit t5 63' 'alu_shr t8 t2 t5 ; lit t5 63'
bg mut_sem shr_buyuk SHR_U128 'sel t6 t4 t7 t6' 'sel t6 t4 t6 t7'
bg mut_sem shr_hi SHR_U128 'sel t7 t4 t5 t7' 'sel t7 t4 t7 t5'
bg mut_sem sar_dolgu SAR_I128 'alu_sar t8 t2 t5' 'alu_shr t8 t2 t5'
bg mut_sem sar_kucuk SAR_I128 'alu_sar t7 t2 t3' 'alu_shr t7 t2 t3'
bg mut_sem eq_yuksek EQ_128 'alu_xor t6 t2 t4' 'alu_xor t6 t2 t2'
bg mut_sem ne_sonuc NE_128 'cmp_ne t5 t5 t6' 'cmp_eq t5 t5 t6'
bg mut_sem lt_isaretli LT_I128 'cmp_lt t5 t2 t4' 'cmp_ltu t5 t2 t4'
bg mut_sem lt_isaretsiz LT_U128 'cmp_ltu t5 t2 t4' 'cmp_lt t5 t2 t4'
bg mut_sem lt_dusuk LT_I128 'cmp_ltu t7 t1 t3' 'cmp_lt t7 t1 t3'
bg mut_sem le_ters LE_U128 'cmp_eq t5 t5 t6' 'cmp_ne t5 t5 t6'
bg mut_sem sext_hi SEXT64_I128 'alu_sar t2 t1 t2' 'alu_shr t2 t1 t2'
bg mut_sem zext_hi ZEXT64_U128 'lit t2 0' 'lit t2 1'
bg mut_sem tobool_yuksek TOBOOL_I128 'alu_or t1 t1 t2' 'alu_or t1 t1 t1'
bg mut_sem trunc8 TRUNC_I128_I8 'sx8 t2 t1' 'sx16 t2 t1'
bg mut_sem trunc_u32 TRUNC_U128_U32 'zx32 t2 t1' 'zx16 t2 t1'
bg mut_sem hi64_sozcuk HI64_I128 'rdw t1 b 1' 'rdw t1 b 0'
bg mut_sem pack_yuksek PACK_U128 'wrw a 1 t2' 'wrw a 1 t1'
bg mut_sem ldi_yuksek LDI_I128 'alu_sar t2 t1 t2' 'alu_shr t2 t1 t2'
bg mut_dosya svm_mulhu oz/sayisal/svm.pma 'add t, t, p3
    ret t' 'ret t'
bg mut_dosya svm_rdw oz/sayisal/svm.pma 'add o, o, f2
    add a, rb, o' 'add a, rb, o'
bg mut_dosya svm_wrw oz/sayisal/svm.pma 'add o, o, f1
    add a, f2, TS' 'add a, f2, TS'
bg mut_dosya gen2_opw oz/elf2/gen2.pma.in '    add o, o, nn
    mul r, o, 8' '    mul r, o, 8'
bg mut_dosya valid_genislik oz/dogr/valid.pma.in '    shri w1, kf, 8
    addi w1, w1, 1
    add w1, w1, v' '    shri w1, kf, 8
    mov w1, 1
    add w1, w1, v'
bg mut_dosya valid_okuma oz/dogr/valid.pma.in 'srd:
    shri wd, kf, 8' 'srd:
    mov kf, 0
    shri wd, kf, 8'
bg mut_dosya asm_hedef_ayirma oz/asm/asm.pma.in 'stw: ge z, wi, wdt' 'stw: ge z, wi, wi'
wait
echo "---"
for f in $T/sonuc_*; do cat $f; done | awk '/^yakalandi/{y++} /^KACTI/{k++; print} /HATASI|uretilemedi/{h++; print} END{printf "sabotaj(i128): %d yakalandi, %d kacti, %d uygulanamadi/uretilemedi\n", y, k, h}'
rm -rf $T
