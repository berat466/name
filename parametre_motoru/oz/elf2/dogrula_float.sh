#!/bin/bash
# Ondalik (SSE2) ELF denetimi: bagimsiz ELF = `pm run` (C++ referans). Sonuclar BIT DUZEYINDE karsilastirilir (yazmac degeri tamsayi olarak yazdirilir; putf'e gerek yok).
# 1) ozel degerler (±0, ±1, ±sonsuz, NaN'lar, alt normal, 2^63 siniri, ...) x tum ikili islemler (fadd fsub fmul fdiv feq flt fle)
# 2) tekli islemler (fneg fsqrt f2i i2f) ozel degerlerde
# 3) rastgele 64 bit desenleri ve "makul" ondalikler (3 ayri tohum) -> ozet degerleri
# Kullanim: dogrula_float.sh [pm]   (GEN2/GENELF ile uretici secilebilir)
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); T=$(mktemp -d)
G2=${GEN2:-$D/gen2.pmb}; GEN="${GENELF:-$PM run $G2}"
ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout 120 "$@"); }
# ozel degerler (onaltilik bit desenleri)
OZEL="0000000000000000 8000000000000000 3FF0000000000000 BFF0000000000000 3FE0000000000000 4000000000000000 4008000000000000 4024000000000000 3FB999999999999A 3FD5555555555555 7FEFFFFFFFFFFFFF FFEFFFFFFFFFFFFF 0010000000000000 000FFFFFFFFFFFFF 0000000000000001 8000000000000001 7FF0000000000000 FFF0000000000000 7FF8000000000000 FFF8000000000000 7FF4000000000000 7FF8000000000001 43E0000000000000 C3E0000000000000 43DFFFFFFFFFFFFF C3DFFFFFFFFFFFFF 4340000000000000 4340000000000001 C340000000000000 41DFFFFFFFC00000 BFE0000000000000 3FF8000000000000 4059000000000000 7E37E43C8800759C 01A56E1FC2F8F359 3CA0000000000000 3FEFFFFFFFFFFFFF 3FF0000000000001 400921FB54442D18 7FF0000000000001 FFF0000000000001"
{
  echo "memory 4096"; printf 'data tab ='
  for h in $OZEL; do printf ' %s' $((16#$h)); done; echo
  n=$(echo $OZEL | wc -w)
  cat <<EOF
func pr(v) {
    host puti, v
    host putc, 32
    ret 0
}
func main() {
    mov i, 0
li: gei z, i, $n
    jnz z, ld
    add a, i, tab
    ld x, a, 0
    mov j, 0
lj: gei z, j, $n
    jnz z, nj
    add a, j, tab
    ld y, a, 0
    fadd r, x, y
    call q, pr, r
    fsub r, x, y
    call q, pr, r
    fmul r, x, y
    call q, pr, r
    fdiv r, x, y
    call q, pr, r
    feq r, x, y
    call q, pr, r
    flt r, x, y
    call q, pr, r
    fle r, x, y
    call q, pr, r
    addi j, j, 1
    jmp lj
nj: fneg r, x
    call q, pr, r
    fsqrt r, x
    call q, pr, r
    f2i r, x
    call q, pr, r
    i2f r, x
    call q, pr, r
    host putc, 10
    addi i, i, 1
    jmp li
ld: ret 0
}
entry main
EOF
} > $T/ozel.pma
# rastgele: xorshift64* ile desen uret; ozel tur 0: ham 64 bit; 1: i2f(rastgele >> k) / i2f(rastgele2 >> k2) (makul buyukluk); sonuclari 64 bit ozet (h = h*1099511628211 ^ r)
cat > $T/rast.pma <<'EOF'
memory 1024
func main() {
    mov seed, SEED
    mov pr, 1099511628211
    mov i, 0
    mov h1, 0
    mov h2, 0
    mov h3, 0
    mov h4, 0
    mov h5, 0
    mov h6, 0
    mov h7, 0
    mov h8, 0
    mov h9, 0
    mov x, seed
lp: gei z, i, 30000
    jnz z, dn
    shli t, x, 13
    xor x, x, t
    shri t, x, 7
    xor x, x, t
    shli t, x, 17
    xor x, x, t
    mov a, x
    shli t, x, 13
    xor x, x, t
    shri t, x, 7
    xor x, x, t
    shli t, x, 17
    xor x, x, t
    mov b, x
    andi m, i, 3
    eqi z, m, 0
    jnz z, raw
    ; makul: i2f(a >> 11) ve i2f(b >> 40), bolum/carpim ile cesitlendir
    sari t, a, 11
    i2f a, t
    sari t, b, 40
    i2f b, t
    eqi z, m, 2
    jz z, raw
    shri t, b, 52
    andi t, t, 63
    ; 2^-k ile olcekle
    mov sc, 1.0
    jz t, raw
sc1: fmul sc, sc, 0.5
    addi t, t, -1
    jnz t, sc1
    fmul a, a, sc
raw:
    fadd r, a, b
    mul h1, h1, pr
    xor h1, h1, r
    fsub r, a, b
    mul h2, h2, pr
    xor h2, h2, r
    fmul r, a, b
    mul h3, h3, pr
    xor h3, h3, r
    fdiv r, a, b
    mul h4, h4, pr
    xor h4, h4, r
    feq r, a, b
    flt q, a, b
    fle w, a, b
    shli q, q, 1
    shli w, w, 2
    or r, r, q
    or r, r, w
    mul h5, h5, pr
    xor h5, h5, r
    fneg r, a
    mul h6, h6, pr
    xor h6, h6, r
    fsqrt r, a
    mul h7, h7, pr
    xor h7, h7, r
    f2i r, a
    mul h8, h8, pr
    xor h8, h8, r
    i2f r, a
    mul h9, h9, pr
    xor h9, h9, r
    addi i, i, 1
    jmp lp
dn: host puti, h1
    host putc, 10
    host puti, h2
    host putc, 10
    host puti, h3
    host putc, 10
    host puti, h4
    host putc, 10
    host puti, h5
    host putc, 10
    host puti, h6
    host putc, 10
    host puti, h7
    host putc, 10
    host puti, h8
    host putc, 10
    host puti, h9
    host putc, 10
    ret 0
}
entry main
EOF
cmpone() {   # etiket pma [stdin]
  $PM asm $2 -o $T/k.pmb --strip > $T/asm.err 2>&1 || { fail "$1: derlenmedi: $(head -c 100 $T/asm.err)"; return; }
  $PM liste $T/k.pmb > $T/k.l
  lim $GEN < $T/k.l > $T/k.elf 2>/dev/null || { fail "$1: ELF uretilemedi"; return; }; chmod +x $T/k.elf
  lim $PM run $T/k.pmb < ${3:-/dev/null} > $T/o1 2> $T/e1; r1=$?
  lim $T/k.elf < ${3:-/dev/null} > $T/o2 2> $T/e2; r2=$?
  [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && [ -s $T/o1 ] && pass || fail "$1: pm rc=$r1 elf rc=$r2 $(cmp $T/o1 $T/o2 2>&1 | head -c 90)"
}
cmpone ozel $T/ozel.pma
# rastgele: bes tohum; i%4: ham 64 bit desen / makul (i2f) / olcekli
for sd in 88172645463325252 1 -7046029254386353131 1234567890123 987654321987654321; do sed "s/SEED/$sd/" $T/rast.pma > $T/rast_s.pma; cmpone "rastgele($sd)" $T/rast_s.pma; done
# ---- 4) putf ("%.15g"): ozel degerler + rastgele desenler + onluk yarim-esitlik (tam yuvarlama) + 10^k / 2^k kuvvetleri; ELF = pm (snprintf)
{
  echo "memory 4096"; printf 'data tab ='; for h in $OZEL 0000000000000064 4059000000000000; do printf ' %s' $((16#$h)); done; echo
  cat <<EOF2
func pf(v) {
    host putf, v
    host putc, 10
    ret 0
}
func main() {
    mov i, 0
li: gei z, i, $((n+2))
    jnz z, ld
    add a, i, tab
    ld x, a, 0
    call q, pf, x
    fneg y, x
    call q, pf, y
    addi i, i, 1
    jmp li
ld: mov v, 1.0
    mov k, 0
p1: gei z, k, 330
    jnz z, p2
    call q, pf, v
    fmul v, v, 10.0
    addi k, k, 1
    jmp p1
p2: mov v, 1.0
    mov k, 0
p3: gei z, k, 330
    jnz z, p4
    call q, pf, v
    fdiv v, v, 10.0
    addi k, k, 1
    jmp p3
p4: mov v, 1.0
    mov k, 0
p5: gei z, k, 1100
    jnz z, p6
    call q, pf, v
    fmul v, v, 2.0
    addi k, k, 1
    jmp p5
p6: mov v, 1.0
    mov k, 0
p7: gei z, k, 1100
    jnz z, p8
    call q, pf, v
    fmul v, v, 0.5
    addi k, k, 1
    jmp p7
p8: ret 0
}
entry main
EOF2
} > $T/pf1.pma
cmpone putf-ozel $T/pf1.pma
cat > $T/pf2.pma <<'EOF2'
memory 1024
func pf(v) {
    host putf, v
    host putc, 10
    ret 0
}
func main() {
    mov x, SEED
    mov i, 0
lp: gei z, i, 12000
    jnz z, dn
    shli t, x, 13
    xor x, x, t
    shri t, x, 7
    xor x, x, t
    shli t, x, 17
    xor x, x, t
    andi m, i, 7
    ; 0,1: ham 64 bit desen
    lti z, m, 2
    jz z, m2
    call q, pf, x
    jmp nx
m2: lti z, m, 4
    jz z, m4
    ; 2,3: makul: tamsayi / 10^k
    sari t, x, 11
    i2f a, t
    shri k, x, 59
    andi k, k, 15
    mov d, 1.0
dl: jz k, dd
    fmul d, d, 10.0
    addi k, k, -1
    jmp dl
dd: fdiv a, a, d
    call q, pf, a
    jmp nx
m4: lti z, m, 6
    jz z, m6
    ; 4,5: n + 0.5, n 15-17 haneli (tam yarim-esitlik)
    shri t, x, 12
    mov base, 100000000000000
    mov md, 3000000000000000
    mod t, t, md
    add t, t, base
    i2f a, t
    fadd a, a, 0.5
    call q, pf, a
    jmp nx
m6: ; 6,7: n * 2^-j (alt basamaklarda cok sayida basamak)
    shri t, x, 11
    i2f a, t
    shri k, x, 56
    andi k, k, 63
    mov d, 1.0
dl2: jz k, dd2
    fmul d, d, 0.5
    addi k, k, -1
    jmp dl2
dd2: fmul a, a, d
    call q, pf, a
nx: addi i, i, 1
    jmp lp
dn: ret 0
}
entry main
EOF2
for sd in 88172645463325252 1 -7046029254386353131 4242424242; do sed "s/SEED/$sd/" $T/pf2.pma > $T/pf2s.pma; cmpone "putf-rastgele($sd)" $T/pf2s.pma; done
echo "ondalik: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
