#!/bin/bash
# PLAN BOLUM 01 -- 128 BIT TAMSAYILAR (i128 / u128) ve GENIS OPERAND altyapisi: add sub mul neg not and or xor shl shr sar eq ne lt le (isaretli/isaretsiz), donusumler (sext64/zext64/trunc/tobool/hi64/pack).
# Komutlar komut tablosunda YALNIZCA OP+SEM'dir (rdw/wrw/mulhu mikro-islemleri). Karsilastirma: ELF (gen2) = svm (tablo calisma aninda) = C++ pm (unsigned __int128 ile) = BAGIMSIZ C++ oracle (i128_oracle.cpp).
# Kullanim: dogrula_i128.sh [pm] [elf_deger_sayisi] [svm_deger_sayisi]
PM=${1:-../../pm}; NE=${2:-26}; NS=${3:-6}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout ${LIMS:-180} "$@"); }
TAB=$R/oz/komut/komutlar.pmk; ASM=$R/oz/asm/asm.pmb; GEN=$R/oz/elf2/gen2.pmb
g++ -std=gnu++17 -O1 -o $T/oracle i128_oracle.cpp || { echo "oracle derlenemedi"; exit 1; }
$PM asm $R/oz/sayisal/svm.pma -o $T/svm.pmb --strip > /dev/null 2>&1 || { echo "svm derlenemedi"; exit 1; }
derle() { lim $PM run $ASM < $1 > $2 2> $T/asm.err && $PM liste $2 > $3 2>/dev/null; }
elfcikti() { lim $PM run $GEN < $1 > $T/x.elf 2>/dev/null || return 1; chmod +x $T/x.elf; lim $T/x.elf > $2 2>/dev/null; }
svmcikti() { { cat $TAB; echo "@@PROGRAM"; cat $1; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $2 2>/dev/null; }
# genis sonucu yazdir: sonuc yazmaci R (genis), gecici t: dusuk ve yuksek sozcuk
W() { echo "    trunc_i128_i64 t, $1"; echo "    host puti, t"; echo "    host putc, 32"; echo "    hi64_i128 t, $1"; echo "    host puti, t"; echo "    host putc, 32"; }
N() { echo "    host puti, $1"; echo "    host putc, 32"; }
progcift() {   # N cikti
  local veri=$($T/oracle veri $1); local n=$(echo "$veri" | tr ',' '\n' | wc -l); n=$((n/2))
  { echo "memory 8192"; echo "data tab = $veri"; cat <<EOT
func main() {
    mov i, 0
li: gei z, i, $n
    jnz z, done
    muli a, i, 2
    ld xl, a, tab
    ld xh, a, tab
    ld xh, a, 0
EOT
  } > /dev/null
  # PMA'da ld d, taban, anlik: taban yazmaci + sabit kayma; data adresi 'tab' bir anlik
  { echo "memory 8192"; echo "data tab = $veri"; echo "func main() {"; echo "    mov i, 0"; echo "li: gei z, i, $n"; echo "    jnz z, done"
    echo "    muli a, i, 2"; echo "    addi a, a, tab"; echo "    ld xl, a, 0"; echo "    ld xh, a, 1"; echo "    pack_i128 x, xl, xh"
    echo "    mov j, 0"; echo "lj: gei z, j, $n"; echo "    jnz z, nj"
    echo "    muli b, j, 2"; echo "    addi b, b, tab"; echo "    ld yl, b, 0"; echo "    ld yh, b, 1"; echo "    pack_i128 y, yl, yh"
    for op in add sub mul and or xor; do echo "    ${op}_i128 r, x, y"; W r; done
    for op in add sub mul and or xor; do echo "    ${op}_u128 r, x, y"; W r; done
    for op in eq_128 ne_128 lt_i128 le_i128 lt_u128 le_u128; do echo "    $op c, x, y"; N c; done
    echo "    tobool_i128 c, y"; echo "    jz c, nodiv"; echo "    host putc, 124"; echo "    host putc, 32"
    echo "    div_i128 r, x, y"; W r; echo "    mod_i128 r, x, y"; W r; echo "    div_u128 r, x, y"; W r; echo "    mod_u128 r, x, y"; W r
    echo "nodiv: host putc, 10"; echo "    addi j, j, 1"; echo "    jmp lj"; echo "nj: addi i, i, 1"; echo "    jmp li"; echo "done: halt"; echo "}"; echo "entry main"; } > $2
}
progtek() {   # N cikti
  local veri=$($T/oracle veri $1); local n=$(echo "$veri" | tr ',' '\n' | wc -l); n=$((n/2))
  { echo "memory 8192"; echo "data tab = $veri"; echo "func main() {"; echo "    mov i, 0"; echo "li: gei z, i, $n"; echo "    jnz z, done"
    echo "    muli a, i, 2"; echo "    addi a, a, tab"; echo "    ld xl, a, 0"; echo "    ld xh, a, 1"; echo "    pack_i128 x, xl, xh"; echo "    pack_u128 xu, xl, xh"
    for op in neg not; do echo "    ${op}_i128 r, x"; W r; done
    for op in neg not; do echo "    ${op}_u128 r, xu"; W r; done
    for c in 0 1 5 31 32 63 64 65 100 127 128 200 77; do
      echo "    host putc, 124"; echo "    host putc, 32"; echo "    ldi n, $c"
      echo "    shl_i128 r, x, n"; W r; echo "    shl_u128 r, xu, n"; W r; echo "    shr_u128 r, xu, n"; W r; echo "    sar_i128 r, x, n"; W r
    done
    echo "    host putc, 124"; echo "    host putc, 32"
    echo "    sext64_i128 r, xl"; W r; echo "    zext64_i128 r, xl"; W r; echo "    sext64_u128 r, xl"; W r; echo "    zext64_u128 r, xl"; W r
    echo "    host putc, 124"; echo "    host putc, 32"
    for ty in i128 u128; do v=x; [ $ty = u128 ] && v=xu
      for tn in i8 i16 i32 i64 u8 u16 u32 u64; do echo "    trunc_${ty}_${tn} c, $v"; N c; done
      echo "    tobool_$ty c, $v"; N c; echo "    hi64_$ty c, $v"; N c; done
    echo "    host putc, 124"; echo "    host putc, 32"
    for ty in i128 u128; do for im in 5 -5 2147483647 -2147483648; do echo "    ldi_$ty r, $im"; W r; done; done
    echo "    host putc, 124"; echo "    host putc, 32"; echo "    bswap128 r, x"; W r
    echo "    host putc, 10"; echo "    addi i, i, 1"; echo "    jmp li"; echo "done: halt"; echo "}"; echo "entry main"; } > $2
}
karsilastir() {   # etiket pma beklenen svm_pma svm_beklenen
  local et=$1 pma=$2 exp=$3
  [ -s $exp ] || { fail "$et: oracle ciktisi bos"; return; }
  if ! derle $pma $T/p.pmb $T/p.l; then fail "$et: derlenemedi ($(head -c 200 $T/asm.err))"; return; fi
  # beklenen satirlarda sozcuk bicimi: bosluklu sayilar; program ayni bicimde yazar ('|' ayiricilari tek tek)
  lim $PM run $T/p.pmb < /dev/null > $T/pm.out 2>/dev/null; cmp -s <(awk '{$1=$1; print}' $T/pm.out) <(awk '{$1=$1; print}' $exp) && pass || fail "$et: C++ pm cikti oracle'dan farkli ($(cmp <(awk '{$1=$1; print}' $T/pm.out) <(awk '{$1=$1; print}' $exp) 2>&1 | head -c 100))"
  if elfcikti $T/p.l $T/elf.out; then cmp -s <(awk '{$1=$1; print}' $T/elf.out) <(awk '{$1=$1; print}' $exp) && pass || fail "$et: ELF cikti oracle'dan farkli ($(cmp <(awk '{$1=$1; print}' $T/elf.out) <(awk '{$1=$1; print}' $exp) 2>&1 | head -c 100))"; else fail "$et: ELF uretilemedi/calismadi"; fi
  derle $4 $T/ps.pmb $T/ps.l || { fail "$et (svm): derlenemedi"; return; }
  svmcikti $T/ps.l $T/svm.out; cmp -s <(awk '{$1=$1; print}' $T/svm.out) <(awk '{$1=$1; print}' $5) && pass || fail "$et: svm cikti oracle'dan farkli ($(cmp <(awk '{$1=$1; print}' $T/svm.out) <(awk '{$1=$1; print}' $5) 2>&1 | head -c 100))"
}
progcift $NE $T/c.pma; $T/oracle cift $NE | sed 's/ $//' > $T/c.exp; progcift $NS $T/cs.pma; $T/oracle cift $NS | sed 's/ $//' > $T/cs.exp
karsilastir "i128/u128 ikili islemler ve karsilastirmalar" $T/c.pma $T/c.exp $T/cs.pma $T/cs.exp
progtek $NE $T/t.pma; $T/oracle tek $NE | sed 's/ $//' > $T/t.exp; progtek $NS $T/ts.pma; $T/oracle tek $NS | sed 's/ $//' > $T/ts.exp
karsilastir "i128/u128 tekli islemler, kaydirmalar, donusumler" $T/t.pma $T/t.exp $T/ts.pma $T/ts.exp
# ---- doğrulayıcı / assembler / C++ referans: genis operand siniri ve kesin atama (valid.pmb = C++ listedogrula = svm yolu)
VAL=$R/oz/dogr/valid.pmb
dg() {   # etiket beklenen(1/0) pma
  local et=$1 exp=$2; cat > $T/v.pma
  $PM asm $T/v.pma -o $T/v.pmb --strip > $T/asm.err 2>&1 || { fail "$et: derlenemedi ($(head -c 100 $T/asm.err))"; return; }
  $PM liste $T/v.pmb > $T/v.l
  a=$(lim $PM run $VAL < $T/v.l | head -1 | tr -d '\r'); b=$($PM listedogrula $T/v.l | head -1)
  [ "$a" = "$exp" ] && [ "$b" = "$exp" ] && pass || fail "$et: beklenen $exp; sayisal valid '$a', C++ listedogrula '$b'"
  # PMB1 asm: C++ ile sayisal assembler (asm.pmb) ayni ikiliyi uretir
  lim $PM run $ASM < $T/v.pma > $T/v2.pmb 2>/dev/null; cmp -s $T/v.pmb $T/v2.pmb && pass || fail "$et: sayisal assembler C++ assembler ile ayni PMB1 degil"
}
dg "genis kaynak: cift yazmac tamam" "1 1" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 ldi c, 3
 ldi d, 4
 add_i128 e, a, c
 halt
}
entry main
EOT
dg "genis kaynak: ust sozcuk atanmamis (kesin atama kanitlanamaz: zero_free=0)" "1 0" <<'EOT'
func main() {
 ldi q, 0
 ldi a, 1
 add_i128 e, a, a
 halt
}
entry main
EOT
dg "genis hedef sonrasi her iki sozcuk atanmis sayilir" "1 1" <<'EOT'
func main() {
 ldi a, 1
 ldi b, 2
 mov_i128 e, a
 trunc_i128_i64 x, e
 hi64_i128 y, e
 add x, x, y
 halt
}
entry main
EOT
# sinir: genis hedef son yazmaca verilirse (r+1 = nregs) iki dogrulayici da reddeder -- PMB1 listesi elle uretilir
opadd=$({ echo "@@gor"; cat $TAB; } | $PM run $R/oz/arac/komut.pmb | awk '$1=="op" && $3=="ADD_I128"{print $2}')
ophalt=$({ echo "@@gor"; cat $TAB; } | $PM run $R/oz/arac/komut.pmb | awk '$1=="op" && $3=="HALT"{print $2}')
opldi=$({ echo "@@gor"; cat $TAB; } | $PM run $R/oz/arac/komut.pmb | awk '$1=="op" && $3=="LDI"{print $2}')
# program: 4 yazmac; ldi r0..r3 ; add_i128 hedef=<h> kaynak r0, r2 ; halt   (liste: 0 mem entry | nconst 0 | ndata 0 | bss 0 | nfunc 1 | npar nreg ncode ...)
mk() { echo "0 65536 0 0 0 0 1 0 4 $((5+1))  $opldi 0 1  $opldi 1 1  $opldi 2 1  $opldi 3 1  $opadd $1 0 2  $ophalt"; }
mk 2 > $T/b_ok.l; mk 3 > $T/b_bad.l; mk 4 > $T/b_oob.l
for c in ok bad oob; do
  exp=0; [ $c = ok ] && exp='1 1'
  a=$(lim $PM run $VAL < $T/b_$c.l | head -1); b=$($PM listedogrula $T/b_$c.l | head -1)
  [ "${a%% *}" = "${exp%% *}" ] && [ "${b%% *}" = "${exp%% *}" ] && pass || fail "genis hedef siniri ($c): beklenen $exp; sayisal valid '$a', C++ listedogrula '$b'"
done
# sifira bolme: ucu de ayni tuzak (DIV0, cikis kodu 2); svm "@@TRAP DIV0" yazar
for op in div_i128 mod_i128 div_u128 mod_u128; do
  printf 'func main() {\n ldi a, 5\n ldi b, 0\n ldi c, 0\n ldi d, 0\n %s e, a, c\n halt\n}\nentry main\n' $op > $T/z.pma
  derle $T/z.pma $T/z.pmb $T/z.l || { fail "$op sifira bolme: derlenemedi"; continue; }
  lim $PM run $T/z.pmb < /dev/null > /dev/null 2> $T/z.err; rc1=$?
  lim $PM run $GEN < $T/z.l > $T/z.elf 2>/dev/null && chmod +x $T/z.elf && lim $T/z.elf > /dev/null 2> $T/z.err2; rc2=$?
  { cat $TAB; echo "@@PROGRAM"; cat $T/z.l; echo "@@GIRDI"; } > $T/sv.in; lim $PM run $T/svm.pmb < $T/sv.in > $T/z.svm 2>/dev/null
  [ $rc1 = 2 ] && [ $rc2 = 2 ] && grep -q 'DIV0' $T/z.svm && grep -q 'DIV0' $T/z.err && pass || fail "$op sifira bolme: pm rc=$rc1 ELF rc=$rc2 svm='$(head -c 40 $T/z.svm)'"
done
echo "128 bit tamsayi: $ok gecti, $bad basarisiz"
[ -n "$KEEP" ] && echo "T=$T" || rm -rf "$T"
[ $bad = 0 ]
