#!/bin/bash
# PLAN BOLUM 01 -- GENIS KAYAN NOKTA (f80 = x87 genisletilmis long double, f128 = IEEE binary128): add sub mul div sqrt neg abs eq ne lt le tobool, tamsayi/dar kayan/ikili64 <-> genis donusumleri.
# Komutlar tabloda YALNIZCA OP+SEM'dir (yazilim kayan nokta: rdw/wrw/mulhu/alu/cmp/sel + lpb/lpe dongu + spld/spst kazima alani). Karsilastirma: ELF (gen2) = svm (tablo calisma aninda) = C++ pm (long double / __float128) = BAGIMSIZ C++ oracle (kayan_oracle.cpp).
# Kullanim: dogrula_kayan.sh [pm] [elf_deger_sayisi] [svm_deger_sayisi]
PM=${1:-../../pm}; NE=${2:-22}; NS=${3:-5}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 4000000; timeout ${LIMS:-300} "$@"); }
TAB=$R/oz/komut/komutlar.pmk; ASM=$R/oz/asm/asm.pmb; GEN=$R/oz/elf2/gen2.pmb
g++ -std=gnu++20 -O1 -fcx-limited-range -ffp-contract=off -o $T/oracle kayan_oracle.cpp -lquadmath || { echo "oracle derlenemedi"; exit 1; }
$PM asm $R/oz/sayisal/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "svm derlenemedi"; exit 1; }
derle() { lim $PM run $ASM < $1 > $2 2> $T/asm.err && $PM liste $2 > $3 2>/dev/null; }
elfcikti() { lim $PM run $GEN < $1 > $T/x.elf 2>/dev/null || return 1; chmod +x $T/x.elf; lim $T/x.elf > $2 2>/dev/null; }
svmcikti() { { cat $TAB; echo "@@PROGRAM"; cat $1; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $2 2>/dev/null; }
norm() { awk '{$1=$1; print}' "$1"; }
karsilastir() {   # etiket pma beklenen svm_pma svm_beklenen
  local et=$1 pma=$2 exp=$3
  [ -s $exp ] || { fail "$et: oracle ciktisi bos"; return; }
  if ! derle $pma $T/p.pmb $T/p.l; then fail "$et: derlenemedi ($(head -c 200 $T/asm.err))"; return; fi
  # PMB1 kacis kodlamasi (komut kodu >= 255): sayisal assembler (asm.pmb) ve C++ assembler ayni ikiliyi uretir; liste/yeniden kodlama gidis-donus
  $PM asm $pma -o $T/pc.pmb --strip > /dev/null 2>&1; cmp -s $T/pc.pmb $T/p.pmb && pass || fail "$et: sayisal assembler ile C++ assembler ayni PMB1'i uretmedi"
  $PM liste $T/pc.pmb > $T/pc.l 2>/dev/null; cmp -s $T/pc.l $T/p.l && pass || fail "$et: C++ liste farkli"
  lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s <(norm $T/pm.out) <(norm $exp) && pass || fail "$et: C++ pm cikti oracle'dan farkli ($(diff <(norm $T/pm.out) <(norm $exp) | head -4 | cut -c1-200 | tr '\n' '|'))"
  if elfcikti $T/p.l $T/elf.out; then cmp -s <(norm $T/elf.out) <(norm $exp) && pass || fail "$et: ELF cikti oracle'dan farkli ($(diff <(norm $T/elf.out) <(norm $exp) | head -4 | cut -c1-200 | tr '\n' '|'))"; else fail "$et: ELF uretilemedi/calismadi"; fi
  derle $4 $T/ps.pmb $T/ps.l || { fail "$et (svm): derlenemedi"; return; }
  svmcikti $T/ps.l $T/svm.out; cmp -s <(norm $T/svm.out) <(norm $5) && pass || fail "$et: svm cikti oracle'dan farkli ($(diff <(norm $T/svm.out) <(norm $5) | head -4 | cut -c1-200 | tr '\n' '|'))"
}
test1() {   # tur F orakel_ad
  local kind=$1 F=$2 sfx=80; [ $F = f128 ] && sfx=128
  local on=$kind$sfx; [ $kind = cd -o $kind = ci ] && on=$kind$sfx
  python3 kayan_prog.py $kind $F $NE $T/oracle > $T/k.pma && $T/oracle $on $NE > $T/k.exp || { fail "$kind $F: uretim"; return; }
  python3 kayan_prog.py $kind $F $NS $T/oracle > $T/ks.pma && $T/oracle $on $NS > $T/ks.exp || { fail "$kind $F svm: uretim"; return; }
  karsilastir "$F $kind" $T/k.pma $T/k.exp $T/ks.pma $T/ks.exp
}
for F in f80 f128; do for k in bin un cd ci; do test1 $k $F; done; done
# 128 bit tamsayi <-> dar kayan nokta (f64 f32 f16 bf16)
python3 kayan_prog.py ni f64 $NE $T/oracle > $T/k.pma && $T/oracle ni $NE > $T/k.exp && python3 kayan_prog.py ni f64 $NS $T/oracle > $T/ks.pma && $T/oracle ni $NS > $T/ks.exp && karsilastir "i128/u128 <-> dar kayan" $T/k.pma $T/k.exp $T/ks.pma $T/ks.exp || fail "ni uretim"
# karmasik sayilar (c64 c128 c160): oracle derleyicinin _Complex turleri (-fcx-limited-range = motorun tanimi: duz formuller)
testc() {   # tur C orakel
  local kind=$1 C=$2; local sfx=${2#c}; local on=$kind$sfx
  python3 kayan_prog.py $kind $C $NE $T/oracle > $T/k.pma && $T/oracle $on $NE > $T/k.exp || { fail "$kind $C: uretim"; return; }
  python3 kayan_prog.py $kind $C $NS $T/oracle > $T/ks.pma && $T/oracle $on $NS > $T/ks.exp || { fail "$kind $C svm: uretim"; return; }
  karsilastir "$C $kind" $T/k.pma $T/k.exp $T/ks.pma $T/ks.exp
}
for C in c64 c128 c160; do testc cbin $C; testc cun $C; done
python3 kayan_prog.py ccv c128 $NE $T/oracle > $T/k.pma && $T/oracle ccv $NE > $T/k.exp && python3 kayan_prog.py ccv c128 $NS $T/oracle > $T/ks.pma && $T/oracle ccv $NS > $T/ks.exp && karsilastir "karmasik donusumler" $T/k.pma $T/k.exp $T/ks.pma $T/ks.exp || fail "ccv uretim"
echo "genis kayan nokta: $ok gecti, $bad basarisiz"
[ -n "$KEEP" ] && echo "T=$T" || rm -rf "$T"
[ $bad = 0 ]
