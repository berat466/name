#!/bin/bash
# PLAN BOLUM 01 -- sayisal turler arasi GENEL DONUSUM KURALLARI: TYPES bolumundeki donusum matrisi (komut @@tur 'donusum' satirlari) gercek yurutmeyle dogrulanir.
#  * matristeki her (kaynak,hedef) cifti icin komut kimligi (-1 = ozdes) TABLODAN okunur; ayni gosterimli turler (char/schar=i8, uchar/char8/byte=u8, wchar=i32, char16=u16, char32=u32) ayni komutu kullanmak zorundadir
#  * temsilci ciftler (i8..u64, bool: 9x9) icin program uretilir: ELF (gen2) = svm (tabloyu calisma aninda okur) = C++ pm = BAGIMSIZ C++ oracle (static_cast, donusum_oracle.cpp)
#  * tamsayi benzeri turler icin matris tam: her sirali cift icin kayit vardir
# Kullanim: dogrula_donusum.sh [pm] [deger_sayisi_svm]
PM=${1:-../../pm}; NS=${2:-24}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout ${LIMS:-120} "$@"); }
TAB=$R/oz/komut/komutlar.pmk; ASM=$R/oz/asm/asm.pmb; GEN=$R/oz/elf2/gen2.pmb; KOMUT=$R/oz/arac/komut.pmb
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $KOMUT); }
g++ -std=c++17 -O1 -o $T/oracle donusum_oracle.cpp || { echo "oracle derlenemedi"; exit 1; }
$PM asm $R/oz/sayisal/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "svm derlenemedi"; exit 1; }
KM tur < $TAB > $T/tur.txt; KM gor < $TAB > $T/gor.txt
declare -A ID NAME_OF CLS BITS SG OPMN
while read -r _ id ad cls bits sg rest; do ID[$ad]=$id; NAME_OF[$id]=$ad; CLS[$ad]=$cls; BITS[$ad]=$bits; SG[$ad]=$sg; done < <(grep '^tur ' $T/tur.txt)
while read -r _ id sym mn rest; do OPMN[$id]=$mn; done < <(grep '^op ' $T/gor.txt)
declare -A CV   # "from,to" -> op
while read -r _ f t op fl; do CV["$f,$t"]=$op; done < <(grep '^donusum ' $T/tur.txt)
# 1) tamlik: tamsayi benzeri (sinif 1..4, <=64 bit) her sirali cift icin kayit var
INTL=(); for ad in "${!ID[@]}"; do c=${CLS[$ad]}; [ "$c" -ge 1 ] && [ "$c" -le 4 ] && [ "${BITS[$ad]}" -le 64 ] && INTL+=($ad); done
miss=0; for a in "${INTL[@]}"; do for b in "${INTL[@]}"; do [ -n "${CV[${ID[$a]},${ID[$b]}]}" ] || { miss=$((miss+1)); echo "eksik: $a -> $b"; }; done; done
[ $miss = 0 ] && [ ${#INTL[@]} -ge 17 ] && pass || fail "donusum matrisi eksik ($miss eksik, ${#INTL[@]} tur)"
# 2) takma adlar temsilcileriyle ayni komutu kullanir
declare -A REP=([char]=i8 [schar]=i8 [uchar]=u8 [char8]=u8 [byte]=u8 [wchar]=i32 [char16]=u16 [char32]=u32)
for a in "${INTL[@]}"; do for b in "${INTL[@]}"; do ra=${REP[$a]:-$a}; rb=${REP[$b]:-$b}
  [ "${CV[${ID[$a]},${ID[$b]}]}" = "${CV[${ID[$ra]},${ID[$rb]}]}" ] && pass || fail "takma ad: $a->$b komutu ${CV[${ID[$a]},${ID[$b]}]} ama $ra->$rb ${CV[${ID[$ra]},${ID[$rb]}]}"; done; done
# 3) yurutme
derle() { lim $PM run $ASM < $1 > $2 2> $T/asm.err && $PM liste $2 > $3 2>/dev/null; }
elfcikti() { lim $PM run $GEN < $1 > $T/x.elf 2>/dev/null || return 1; chmod +x $T/x.elf; lim $T/x.elf > $2 2>/dev/null; }
svmcikti() { { cat $TAB; echo "@@PROGRAM"; cat $1; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $2 2>/dev/null; }
prog() {   # kaynak hedef op veri cikti
  local mn=mov; [ "$3" -ge 0 ] && mn=${OPMN[$3]}
  local n=$(echo "$4" | tr ',' '\n' | wc -l)
  { echo "memory 4096"; echo "data tab = $4"; cat <<EOT
func main() {
    mov i, 0
lp: gei z, i, $n
    jnz z, done
    add a, i, tab
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
  } > $5
}
REPS=(i8 i16 i32 i64 u8 u16 u32 u64 bool)
for f in "${REPS[@]}"; do veri=$($T/oracle veri $f); sveri=$(echo "$veri" | tr ',' '\n' | awk -v n=$NS 'BEGIN{ORS=""} {a[NR]=$1} END{for(i=0;i<n;i++){k=int(i*(NR-1)/(n-1))+1; printf "%s%s", i?",":"", a[k]}}')
  for t in "${REPS[@]}"; do op=${CV[${ID[$f]},${ID[$t]}]}
    $T/oracle don $f $t > $T/exp.txt
    prog $f $t $op "$veri" $T/p.pma
    derle $T/p.pma $T/p.pmb $T/p.l || { fail "$f->$t: derlenemedi ($(head -c 100 $T/asm.err))"; continue; }
    lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s $T/pm.out $T/exp.txt && pass || fail "$f->$t: C++ pm cikti oracle'dan farkli ($(cmp $T/pm.out $T/exp.txt 2>&1 | head -c 80))"
    if elfcikti $T/p.l $T/elf.out; then cmp -s $T/elf.out $T/exp.txt && pass || fail "$f->$t: ELF cikti oracle'dan farkli ($(cmp $T/elf.out $T/exp.txt 2>&1 | head -c 80))"; else fail "$f->$t: ELF uretilemedi/calismadi"; fi
    # svm: ayni program, tum degerler (svm yavas: ornek alt kume) -> beklenen satirlar ayni indislerle secilir
    prog $f $t $op "$sveri" $T/ps.pma; derle $T/ps.pma $T/ps.pmb $T/ps.l || { fail "$f->$t (svm): derlenemedi"; continue; }
    awk -v n=$NS -v tot=$(wc -l < $T/exp.txt) 'BEGIN{ORS=""} {a[NR]=$0} END{for(i=0;i<n;i++){k=int(i*(tot-1)/(n-1))+1; print a[k] "\n"}}' $T/exp.txt > $T/exps.txt
    svmcikti $T/ps.l $T/svm.out; cmp -s $T/svm.out $T/exps.txt && pass || fail "$f->$t: svm cikti oracle'dan farkli ($(cmp $T/svm.out $T/exps.txt 2>&1 | head -c 80))"
  done; done
# 4) kayan nokta donusumleri (tamsayi <-> f16/bf16/f32/f64, kayan <-> kayan): BAGIMSIZ tam-rasyonel referans (ref_donusum.py)
FLS=(f16 bf16 f32 f64)
kayan_cift() {   # kaynak hedef
  local f=$1 t=$2 op=${CV[${ID[$1]},${ID[$2]}]}
  [ -n "$op" ] || { fail "$f->$t: matriste kayit yok"; return; }
  python3 ref_donusum.py veri $f $t 0 | paste -sd, > $T/kv.txt; python3 ref_donusum.py bekle $f $t 0 > $T/exp.txt
  [ -s $T/exp.txt ] || { fail "$f->$t: referans bos"; return; }
  python3 ref_donusum.py veri $f $t $NS | paste -sd, > $T/ksv.txt; python3 ref_donusum.py bekle $f $t $NS > $T/exps.txt
  prog $f $t $op "$(cat $T/kv.txt)" $T/p.pma
  derle $T/p.pma $T/p.pmb $T/p.l || { fail "$f->$t: derlenemedi ($(head -c 100 $T/asm.err))"; return; }
  lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s $T/pm.out $T/exp.txt && pass || fail "$f->$t: C++ pm cikti referanstan farkli ($(cmp $T/pm.out $T/exp.txt 2>&1 | head -c 80))"
  if elfcikti $T/p.l $T/elf.out; then cmp -s $T/elf.out $T/exp.txt && pass || fail "$f->$t: ELF cikti referanstan farkli ($(cmp $T/elf.out $T/exp.txt 2>&1 | head -c 80))"; else fail "$f->$t: ELF uretilemedi/calismadi"; fi
  prog $f $t $op "$(cat $T/ksv.txt)" $T/ps.pma; derle $T/ps.pma $T/ps.pmb $T/ps.l || { fail "$f->$t (svm): derlenemedi"; return; }
  svmcikti $T/ps.l $T/svm.out; cmp -s $T/svm.out $T/exps.txt && pass || fail "$f->$t: svm cikti referanstan farkli ($(cmp $T/svm.out $T/exps.txt 2>&1 | head -c 80))"
}
for i in "${REPS[@]}"; do for fl in "${FLS[@]}"; do kayan_cift $i $fl; kayan_cift $fl $i; done; done
for a in "${FLS[@]}"; do for b in "${FLS[@]}"; do kayan_cift $a $b; done; done
echo "donusum: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
