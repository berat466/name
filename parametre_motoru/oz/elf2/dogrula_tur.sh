#!/bin/bash
# PLAN BOLUM 01 -- TAMSAYI TURLERI: sext/zext donusumleri, int8/16/32 ve uint8/16/32 ailelerinin islemleri (add sub mul div mod neg shl not, sarma kurali), uint64 islemleri
# (divu modu ltu leu gtu geu), uint64 <-> double (u2f f2u). Komutlar komut tablosunda YALNIZCA OP+SEM kaydidir (x86 sablonu yok): ELF mikro-islem sablonlarindan uretilir.
# Bagimsiz referans: yerel C++ turleriyle yazilmis ORACLE (tur_oracle.cpp: int8_t .. uint64_t). Karsilastirma: ELF (gen2), svm (tablo calisma aninda okunur), C++ `pm run` (yorumlayici; src/anlam.h'de elle yazilmis anlam) ve oracle ayni cikti.
# Kullanim: dogrula_tur.sh [pm] [elf_deger_sayisi] [svm_deger_sayisi]
PM=${1:-../../pm}; NE=${2:-0}; NS=${3:-14}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout ${LIMS:-120} "$@"); }
TAB=$R/oz/komut/komutlar.pmk; ASM=$R/oz/asm/asm.pmb; GEN=$R/oz/elf2/gen2.pmb
g++ -O1 -o $T/oracle tur_oracle.cpp || { echo "oracle derlenemedi"; exit 1; }
$PM asm $R/oz/sayisal/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "svm derlenemedi"; exit 1; }
# program -> liste
derle() { lim $PM run $ASM < $1 > $2 2> $T/asm.err && $PM liste $2 > $3 2>/dev/null; }
# bir liste icin ELF ve svm ciktilari: elfcikti L CIKTI ; svmcikti L CIKTI
elfcikti() { lim $PM run $GEN < $1 > $T/x.elf 2>/dev/null || return 1; chmod +x $T/x.elf; lim $T/x.elf > $2 2>/dev/null; }
svmcikti() { { cat $TAB; echo "@@PROGRAM"; cat $1; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $2 2>/dev/null; }
karsilastir() {   # etiket pma beklenen_dosya [svm_icin_pma svm_beklenen]
  local et=$1 pma=$2 exp=$3
  [ -s $exp ] || { fail "$et: oracle ciktisi bos"; return; }
  if ! derle $pma $T/p.pmb $T/p.l; then fail "$et: derlenemedi ($(head -c 120 $T/asm.err))"; return; fi
  lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s $T/pm.out $exp && pass || fail "$et: C++ pm (yorumlayici, ikinci bagimsiz uygulama) cikti oracle'dan farkli ($(cmp $T/pm.out $exp 2>&1 | head -c 90))"
  if elfcikti $T/p.l $T/elf.out; then cmp -s $T/elf.out $exp && pass || fail "$et: ELF cikti oracle'dan farkli ($(cmp $T/elf.out $exp 2>&1 | head -c 90))"; else fail "$et: ELF uretilemedi/calismadi"; fi
  if [ -n "$4" ]; then
    derle $4 $T/ps.pmb $T/ps.l || { fail "$et (svm): derlenemedi"; return; }
    svmcikti $T/ps.l $T/svm.out; cmp -s $T/svm.out $5 && pass || fail "$et: svm cikti oracle'dan farkli ($(cmp $T/svm.out $5 2>&1 | head -c 90))"
  fi
}
# ---- kucuk turler: program uretimi
ailepma() {   # W sg N cikti
  local W=$1 sg=$2 N=$3 out=$4 k; local s=i; [ $sg = 1 ] || s=u; local ext=sext$W; [ $sg = 1 ] || ext=zext$W
  { echo "memory 4096"; echo "data tab = $($T/oracle veri $N)"; cat <<EOT
func main() {
    mov i, 0
li: gei z, i, $(( $($T/oracle veri $N | tr ',' '\n' | wc -l) ))
    jnz z, d1
    add a, i, tab
    ld x0, a, 0
    sext$W r1, x0
    host puti, r1
    host putc, 32
    zext$W r2, x0
    host puti, r2
    host putc, 10
    addi i, i, 1
    jmp li
d1: mov i, 0
lp: gei z, i, $(( $($T/oracle veri $N | tr ',' '\n' | wc -l) ))
    jnz z, done
    add a, i, tab
    ld x0, a, 0
    $ext x, x0
    mov j, 0
lj: gei z, j, $(( $($T/oracle veri $N | tr ',' '\n' | wc -l) ))
    jnz z, nj
    add a, j, tab
    ld y0, a, 0
    $ext y, y0
    add_$s$W r, x, y
    host puti, r
    host putc, 32
    sub_$s$W r, x, y
    host puti, r
    host putc, 32
    mul_$s$W r, x, y
    host puti, r
    host putc, 32
EOT
    if [ $sg = 1 ]; then cat <<EOT
    jz y, nodiv
    div_i$W r, x, y
    host puti, r
    host putc, 32
    mod_i$W r, x, y
    host puti, r
    host putc, 32
    jmp divd
nodiv: host putc, 45
    host putc, 32
    host putc, 45
    host putc, 32
divd:
EOT
    fi
    cat <<EOT
    neg_$s$W r, x
    host puti, r
    host putc, 32
    andi c, y0, 63
    shl_$s$W r, x, c
    host puti, r
EOT
    if [ $sg = 0 ]; then cat <<EOT
    host putc, 32
    not_u$W r, x
    host puti, r
EOT
    fi
    cat <<EOT
    host putc, 10
    addi j, j, 1
    jmp lj
nj: addi i, i, 1
    jmp lp
done: halt
}
entry main
EOT
  } > $out
}
for W in 8 16 32; do for sg in 1 0; do
  s=signed; [ $sg = 1 ] || s=unsigned
  ailepma $W $sg $NE $T/f.pma; $T/oracle dm $W $sg $NE > $T/f.exp
  ailepma $W $sg $NS $T/fs.pma; $T/oracle dm $W $sg $NS > $T/fs.exp
  karsilastir "$s $W bit" $T/f.pma $T/f.exp $T/fs.pma $T/fs.exp
done; done
# ---- uint64: bolme/kalan/karsilastirma (gtu geu takma komut: LTU/LEU'ya b,c takasi ile yeniden yazilir)
u64pma() {   # N cikti
  local n=$(( $($T/oracle veri $1 | tr ',' '\n' | wc -l) ))
  { echo "memory 4096"; echo "data tab = $($T/oracle veri $1)"; cat <<EOT
func main() {
    mov i, 0
lp: gei z, i, $n
    jnz z, done
    add a, i, tab
    ld x, a, 0
    mov j, 0
lj: gei z, j, $n
    jnz z, nj
    add a, j, tab
    ld y, a, 0
    jz y, nodiv
    divu r, x, y
    host puti, r
    host putc, 32
    modu r, x, y
    host puti, r
    host putc, 32
    jmp divd
nodiv: host putc, 45
    host putc, 32
    host putc, 45
    host putc, 32
divd: ltu r, x, y
    host puti, r
    host putc, 32
    leu r, x, y
    host puti, r
    host putc, 32
    gtu r, x, y
    host puti, r
    host putc, 32
    geu r, x, y
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
  } > $2
}
u64pma $NE $T/u.pma; $T/oracle u64 $NE > $T/u.exp; u64pma $NS $T/us.pma; $T/oracle u64 $NS > $T/us.exp
karsilastir "uint64 divu modu ltu leu gtu geu" $T/u.pma $T/u.exp $T/us.pma $T/us.exp
# ---- uint64 -> double (u2f) ve double -> uint64 (f2u, doyurmali: NaN ve negatif -> 0, >= 2^64 -> UINT64_MAX)
cvtpma() {   # islem veri_satiri cikti
  local n=$(echo "$2" | tr ',' '\n' | wc -l)
  { echo "memory 4096"; echo "data tab = $2"; cat <<EOT
func main() {
    mov i, 0
lp: gei z, i, $n
    jnz z, done
    add a, i, tab
    ld x, a, 0
    $1 r, x
    host puti, r
    host putc, 10
    addi i, i, 1
    jmp lp
done: halt
}
entry main
EOT
  } > $3
}
cvtpma u2f "$($T/oracle veri $NE)" $T/c.pma; $T/oracle u2f $NE > $T/c.exp; cvtpma u2f "$($T/oracle veri $NS)" $T/cs.pma; $T/oracle u2f $NS > $T/cs.exp
karsilastir "uint64 -> double (u2f)" $T/c.pma $T/c.exp $T/cs.pma $T/cs.exp
cvtpma f2u "$($T/oracle fveri)" $T/f.pma; $T/oracle f2u > $T/f.exp
karsilastir "double -> uint64 (f2u)" $T/f.pma $T/f.exp $T/f.pma $T/f.exp
# ---- tablo: yeni komutlar yalniz OP+SEM (x86 sablonu yok); takma komutlar gtu/geu
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $R/oz/arac/komut.pmb); }
KM gor < $TAB > $T/tab.txt
for op in sext8 sext16 sext32 zext8 zext16 zext32 add_i8 div_i32 neg_u16 not_u32 shl_u8 divu modu ltu leu u2f f2u; do grep -q "^op [0-9]* $(echo $op | tr a-z A-Z) $op " $T/tab.txt && pass || fail "tabloda $op yok"; done
for op in sext8 add_i8 divu ltu u2f; do grep -q "^tpl op $(echo $op | tr a-z A-Z) " $T/tab.txt && fail "$op icin elle yazilmis x86 sablonu var (yalniz SEM olmali)" || pass; done
KM tab < $TAB | grep '^PSNAMES' | grep -q '103 116 117 0 103 101 117 0' && pass || fail "gtu/geu takma komutlari tabloda yok"
echo "tamsayi turleri: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
