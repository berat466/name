#!/bin/bash
# PLAN BOLUM 01 -- DAR KAYAN NOKTA TURLERI float32 / float16 / bfloat16: yuvarlama (rnd_*), + - * / sqrt (add_* .. sqrt_*), tamsayidan (i2f_*, u2f_*) komutlari; hepsi komut tablosunda YALNIZCA OP+SEM'dir (x86 sablonu yok).
# Beklenen degerler BAGIMSIZ REFERANSTAN gelir: ref_dar.py (tam rasyonel aritmetik; hicbir motor ya da derleyici parcasi yok). Karsilastirma: ELF (gen2) = svm (tablo calisma aninda) = C++ pm (yorumlayici; tamsayi algoritmasi) = referans.
# f32 icin ek olarak DONANIM ucuncu referans: C++ (float) donusumu/islemleri (donanim_dar.cpp). NaN yuku kisaltma + sessizlestirme kurali donanimla (f32) dogrulanir.
# Kullanim: dogrula_dar.sh [pm] [elf_deger_sayisi (0 = hepsi)] [svm_deger_sayisi]
PM=${1:-../../pm}; NE=${2:-0}; NS=${3:-12}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout ${LIMS:-180} "$@"); }
TAB=$R/oz/komut/komutlar.pmk; ASM=$R/oz/asm/asm.pmb; GEN=$R/oz/elf2/gen2.pmb
$PM asm $R/oz/sayisal/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "svm derlenemedi"; exit 1; }
derle() { lim $PM run $ASM < $1 > $2 2> $T/asm.err && $PM liste $2 > $3 2>/dev/null; }
elfcikti() { lim $PM run $GEN < $1 > $T/x.elf 2>/dev/null || return 1; chmod +x $T/x.elf; lim $T/x.elf > $2 2>/dev/null; }
svmcikti() { { cat $TAB; echo "@@PROGRAM"; cat $1; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $2 2>/dev/null; }
prog() {   # mnemonik arite veri_x [veri_y] cikti
  local mn=$1 ar=$2 vx=$3 vy=$4 out=$5; local nx=$(echo "$vx" | tr ',' '\n' | wc -l)
  if [ "$ar" = 1 ]; then { echo "memory 8192"; echo "data tx = $vx"; cat <<EOT
func main() {
    mov i, 0
lp: gei z, i, $nx
    jnz z, done
    add a, i, tx
    ld x, a, 0
    $mn r, x
    host puti, r
    host putc, 10
    addi i, i, 1
    jmp lp
done: halt
}
entry main
EOT
  } > $out; else { echo "memory 8192"; echo "data tx = $vx"; cat <<EOT
func main() {
    mov i, 0
lp: gei z, i, $nx
    jnz z, done
    add a, i, tx
    ld x, a, 0
    mov j, 0
lj: gei z, j, $nx
    jnz z, nj
    add a, j, tx
    ld y, a, 0
    $mn r, x, y
    host puti, r
    host putc, 10
    addi j, j, 1
    jmp lj
nj: addi i, i, 1
    jmp lp
done: halt
}
entry main
EOT
  } > $out; fi
}
run1() {   # etiket mnemonik arite veri_x beklenen_dosya svm_veri_x svm_beklenen
  local et=$1 mn=$2 ar=$3 vx=$4 exp=$5 svx=$6 sexp=$7
  [ -s $exp ] || { fail "$et: referans ciktisi bos"; return; }
  prog $mn $ar "$vx" "$vx" $T/p.pma
  derle $T/p.pma $T/p.pmb $T/p.l || { fail "$et: derlenemedi ($(head -c 100 $T/asm.err))"; return; }
  lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s $T/pm.out $exp && pass || fail "$et: C++ pm cikti referanstan farkli ($(cmp $T/pm.out $exp 2>&1 | head -c 90))"
  if elfcikti $T/p.l $T/elf.out; then cmp -s $T/elf.out $exp && pass || fail "$et: ELF cikti referanstan farkli ($(cmp $T/elf.out $exp 2>&1 | head -c 90))"; else fail "$et: ELF uretilemedi/calismadi"; fi
  prog $mn $ar "$svx" "$svx" $T/ps.pma; derle $T/ps.pma $T/ps.pmb $T/ps.l || { fail "$et (svm): derlenemedi"; return; }
  svmcikti $T/ps.l $T/svm.out; cmp -s $T/svm.out $sexp && pass || fail "$et: svm cikti referanstan farkli ($(cmp $T/svm.out $sexp 2>&1 | head -c 90))"
}
for F in f32 f16 bf16; do
  # rnd: double -> bicim
  python3 ref_dar.py veri rnd $NE > $T/v.txt; python3 ref_dar.py bekle $F rnd $NE > $T/e.txt; python3 ref_dar.py veri rnd $NS > $T/vs.txt; python3 ref_dar.py bekle $F rnd $NS > $T/es.txt
  run1 "rnd_$F" rnd_$F 1 "$(cat $T/v.txt)" $T/e.txt "$(cat $T/vs.txt)" $T/es.txt
  for K in i2f u2f; do
    python3 ref_dar.py veri int 0 > $T/v.txt; python3 ref_dar.py bekle $F $K 0 > $T/e.txt; python3 ref_dar.py veri int $NS > $T/vs.txt; python3 ref_dar.py bekle $F $K $NS > $T/es.txt
    run1 "${K}_$F" ${K}_$F 1 "$(cat $T/v.txt)" $T/e.txt "$(cat $T/vs.txt)" $T/es.txt; done
  # aritmetik: bicim degerleri uzerinde tum ciftler (NE=60 -> 3600 satir; svm: NS)
  NA=40; NSA=$(( NS > 8 ? 8 : NS ))
  for K in add sub mul div; do
    python3 ref_dar.py veri fmt$F $NA > $T/v.txt; python3 ref_dar.py bekle $F $K $NA > $T/e.txt; python3 ref_dar.py veri fmt$F $NSA > $T/vs.txt; python3 ref_dar.py bekle $F $K $NSA > $T/es.txt
    run1 "${K}_$F" ${K}_$F 2 "$(cat $T/v.txt)" $T/e.txt "$(cat $T/vs.txt)" $T/es.txt; done
  python3 ref_dar.py veri fmt$F $NA > $T/v.txt; python3 ref_dar.py bekle $F sqrt $NA > $T/e.txt; python3 ref_dar.py veri fmt$F $NSA > $T/vs.txt; python3 ref_dar.py bekle $F sqrt $NSA > $T/es.txt
  run1 "sqrt_$F" sqrt_$F 1 "$(cat $T/v.txt)" $T/e.txt "$(cat $T/vs.txt)" $T/es.txt
done
# donanim ucuncu referansi (f32): C++ (float) donusumleri ve islemleri ile ref_dar.py ayni mi
g++ -std=gnu++23 -O1 -ffp-contract=off -o $T/hw donanim_dar.cpp 2>$T/g.err || { fail "donanim_dar.cpp derlenemedi: $(head -c 200 $T/g.err)"; }
if [ -x $T/hw ]; then
  for K in rnd; do python3 ref_dar.py veri rnd 400 > $T/v.txt; python3 ref_dar.py bekle f32 rnd 400 > $T/e.txt; $T/hw rnd f32 < $T/v.txt > $T/h.txt; cmp -s $T/h.txt $T/e.txt && pass || fail "donanim f32 rnd referanstan farkli ($(cmp $T/h.txt $T/e.txt 2>&1 | head -c 90))"
    python3 ref_dar.py veri rnd 400 > $T/v.txt; python3 ref_dar.py bekle f16 rnd 400 > $T/e.txt; $T/hw rnd f16 < $T/v.txt > $T/h.txt; cmp -s $T/h.txt $T/e.txt && pass || fail "derleyici f16 rnd referanstan farkli ($(cmp $T/h.txt $T/e.txt 2>&1 | head -c 90))"
    python3 ref_dar.py bekle bf16 rnd 400 > $T/e.txt; $T/hw rnd bf16 < $T/v.txt > $T/h.txt; cmp -s $T/h.txt $T/e.txt && pass || fail "derleyici bf16 rnd referanstan farkli ($(cmp $T/h.txt $T/e.txt 2>&1 | head -c 90))"; done
  python3 ref_dar.py veri int 120 > $T/v.txt
  for K in i2f u2f; do python3 ref_dar.py bekle f32 $K 120 > $T/e.txt; $T/hw $K f32 < $T/v.txt > $T/h.txt; cmp -s $T/h.txt $T/e.txt && pass || fail "donanim f32 $K referanstan farkli ($(cmp $T/h.txt $T/e.txt 2>&1 | head -c 90))"; done
  python3 ref_dar.py veri fmtf32 40 > $T/v.txt
  for K in add sub mul div sqrt; do python3 ref_dar.py bekle f32 $K 40 > $T/e.txt; $T/hw $K f32 < $T/v.txt > $T/h.txt; cmp -s $T/h.txt $T/e.txt && pass || fail "donanim f32 $K referanstan farkli ($(cmp $T/h.txt $T/e.txt 2>&1 | head -c 90))"; done
fi
echo "dar kayan nokta: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
