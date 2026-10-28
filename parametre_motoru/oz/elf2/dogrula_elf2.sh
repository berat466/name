#!/bin/bash
# gen2 (oz/elf2/gen2.pmb: PMB1 tablosu olarak ELF ureticisi) denetimi. Kullanim: dogrula_elf2.sh [pm] [rastgele_sayisi] [valid_mutanti]
# 1) derlem + rastgele cok fonksiyonlu programlar + elle yazilmis tuzak/yigin/alloc/getc programlari: uretilen ELF = `pm run` (stdout, cikis kodu, tuzak adi)
# 2) bagimsizlik: tek PT_LOAD, INTERP/DYNAMIC yok, PMB1/libstdc/GLIBC izi yok, `env -i` ile calisir
# 3) valid.pmb (dogrulayici) ELF'e cevrildi: ayni girdilerde `pm run valid.pmb` ile ayni cikti
# 4) SABIT NOKTA: gen2.elf (pm'siz) kendi listesinden kendi ELF'ini bayt-bayt uretir (iki nesil)
# Tum calistirmalar zaman (timeout) ve bellek (ulimit -v) siniri altindadir; ELF'te yakit siniri yoktur.
PM=${1:-../../pm}; NR=${2:-150}; NV=${3:-200}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
ok=0; bad=0; n=0; unsup=0; skip=0; traps=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
[ -n "$GEN2" ] || { $PM asm gen2.pma -o $T/g.pmb --strip > /dev/null 2>&1 && cmp -s $T/g.pmb gen2.pmb; } && pass || fail "gen2.pmb guncel degil (uret.sh)"
G2=${GEN2:-$D/gen2.pmb}; GEN="$PM run $G2"
lim() { (ulimit -v 700000; timeout 20 "$@"); }
cmpone() {   # etiket pmb stdin
  $PM liste $2 > $T/l.txt 2>/dev/null || { fail "$1: pm liste"; return; }
  if ! lim $GEN < $T/l.txt > $T/p.elf 2>/dev/null; then unsup=$((unsup+1)); [ -s $T/p.elf ] && fail "$1: reddedilen girdide cikti var"; return; fi
  chmod +x $T/p.elf
  lim $PM run $2 < $3 > $T/o1 2> $T/e1; r1=$?
  if grep -q "TRAP: FUEL" $T/e1 || [ $r1 = 124 ]; then skip=$((skip+1)); return; fi
  lim $T/p.elf < $3 > $T/o2 2> $T/e2; r2=$?
  n=$((n+1)); [ $r1 = 2 ] && traps=$((traps+1))
  if [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2; then pass; else fail "$1: pm rc=$r1 elf rc=$r2 $(cmp $T/o1 $T/o2 2>&1 | head -c 60)"; return; fi
  if [ $r1 = 2 ]; then [ "$(head -c 11 $T/e1 | cut -d' ' -f1-2)" = "$(head -c 11 $T/e2 | cut -d' ' -f1-2)" ] && pass || fail "$1: tuzak adi farkli ($(head -c 20 $T/e1) | $(head -c 20 $T/e2))"; fi
}
: > $T/bos; printf 'abc\n\200\377\000xyz 123\n' > $T/girdi
# ---- 1a) derlem
for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' 2>/dev/null | sort); do $PM asm $f -o $T/k.pmb --strip > /dev/null 2>&1 || continue; cmpone "$f" $T/k.pmb $T/bos; cmpone "$f(stdin)" $T/k.pmb $T/girdi; done
# ---- 1b) rastgele cok fonksiyonlu programlar (stdin: kaynak dosyanin seed'e bagli uzunlukta oneki)
for s in $(seq 1 $NR); do awk -v seed=$s -f rastgele2.awk > $T/r.pma; $PM asm $T/r.pma -o $T/r.pmb --strip > /dev/null 2>&1 || { fail "rastgele $s derlenmedi"; continue; }
  head -c $((s % 37)) $T/r.pma > $T/in.txt; cmpone "rastgele2-$s" $T/r.pmb $T/in.txt; done
# ---- 1c) elle yazilmis uc durumlar
prog() { cat > $T/h.pma; $PM asm $T/h.pma -o $T/h.pmb --strip > /dev/null 2>$T/ae || { fail "$1 derlenmedi: $(head -c 100 $T/ae)"; return; }; cmpone "elle:$1" $T/h.pmb ${2:-$T/bos}; }
prog sonsuz_ozyineleme_DEPTH <<'EOF'
func f(n) {
    addi m, n, 1
    call r, f, m
    ret r
}
func main() {
    call r, f, 0
    host puti, r
}
EOF
for k in 5 12 20 40; do   # yigin: kare basina ilerleme ~k yazmac -> DEPTH ya da STACK (hangisi once)
  { echo "func f(n) {"; for i in $(seq 1 $k); do echo "  ldi q$i, $i"; done; echo "  addi m, n, 1"; echo "  call r, f, m"; echo "  ret r"; echo "}"; printf 'func main() {\n  call r, f, 0\n  host puti, r\n}\n'; } | prog "yigin_k$k"
done
prog ozyineleme_sinirda_ok <<'EOF'
func s(n) {
    jnz n, rec
    ret 0
rec:
    addi m, n, -1
    call r, s, m
    add r, r, n
    ret r
}
func main() {
    call r, s, 60000
    host puti, r
    host putc, 10
}
EOF
prog ozyineleme_derinlik_65535 <<'EOF'
func s(n) {
    jnz n, rec
    ret 0
rec:
    addi m, n, -1
    call r, s, m
    addi r, r, 1
    ret r
}
func main() {
    call r, s, 65535
    host puti, r
    host putc, 10
}
EOF
prog ozyineleme_derinlik_65536_DEPTH <<'EOF'
func s(n) {
    jnz n, rec
    ret 0
rec:
    addi m, n, -1
    call r, s, m
    addi r, r, 1
    ret r
}
func main() {
    call r, s, 65536
    host puti, r
    host putc, 10
}
EOF
prog karsilikli_ozyineleme <<'EOF'
func ev(n) {
    jnz n, a
    ret 1
a:  addi m, n, -1
    call r, od, m
    ret r
}
func od(n) {
    jnz n, a
    ret 0
a:  addi m, n, -1
    call r, ev, m
    ret r
}
func main() {
    call r, ev, 1001
    host puti, r
    host putc, 10
}
EOF
prog alloc_tam_dolu <<'EOF'
memory 20
func main() {
    ldi n, 19
    alloc p, n
    st n, p, 18
    ld x, p, 18
    host puti, x
    host putc, 10
    ldi z, 0
    alloc q, z
    host puti, q
    host putc, 10
}
EOF
prog alloc_bir_fazla_TRAP <<'EOF'
memory 20
func main() {
    ldi n, 20
    alloc p, n
    host puti, p
}
EOF
prog alloc_negatif_TRAP <<'EOF'
memory 20
func main() {
    ldi n, -1
    alloc p, n
    host puti, p
}
EOF
prog alloc_ikinci_TRAP <<'EOF'
memory 20
func main() {
    ldi n, 10
    alloc p, n
    alloc q, n
    host puti, q
    alloc r, n
    host puti, r
}
EOF
prog veri_ve_alloc <<'EOF'
memory 40
data tablo = 10, 20, 30, -5
space tampon 3
func main() {
    ld a, tablo, 0
    ld b, tablo, 3
    host puti, a
    host putc, 32
    host puti, b
    host putc, 32
    ldi n, 4
    alloc p, n
    host puti, p
    host putc, 10
}
EOF
prog getc_hepsi <<'EOF'
func main() {
loop:
    host c, getc
    lti e, c, 0
    jnz e, son
    host puti, c
    host putc, 32
    jmp loop
son:
    host puti, c
    host putc, 10
    host d, getc
    host puti, d
}
EOF
cat > /dev/null <<'EOF'
EOF
printf 'abc\n\200\377\000xyz 123\n' > $T/g1; cmpone "getc_dosyadan" $T/h.pmb $T/g1
printf '' > $T/g0; cmpone "getc_bos" $T/h.pmb $T/g0
prog ret_main_degerli <<'EOF'
func main() {
    ldi x, 7
    host puti, x
    ret x
}
EOF
# okunmadan once yazilmamis yazmac (PMB1 listesi elle): cagrilan fonksiyonun yerel yazmaclari sifir olmali
printf "0 16 0\n0\n0\n0\n2\n0 4 9\n1 0 100\n1 1 200\n1 2 55\n1 3 66\n34 1 2\n40 1 2\n40 1 3\n40 1 1\n36\n0 3 3\n3 2 0 1\n41 2 2 5\n35 2\n" > $T/zl.txt; $PM yazliste $T/zl.txt -o $T/zl.pmb && cmpone "liste:yerel_yazmaclar_sifir" $T/zl.pmb $T/bos; lim $PM run $T/zl.pmb | grep -q "^50200" && pass || fail "liste:sifir beklentisi (pm 50200 vermeli)"
prog cagri_pencere_ustuste <<'EOF'
func h(a, b) {
    sub c, a, b
    ldi junk, 12345
    ret c
}
func main() {
    ldi r0, 50
    ldi r1, 8
    ldi r2, 9
    ldi r3, 77
    call r2, h, r0, r1
    host puti, r0
    host putc, 32
    host puti, r1
    host putc, 32
    host puti, r2
    host putc, 32
    host puti, r3
    host putc, 10
}
EOF
# ---- 1e) yigin/derinlik siniri tam esikte: pm'in gecen en buyuk ozyineleme derinligi (ikili arama) ve +1; ELF ayni davranmali
for kk in 1:0 12:0 14:0 30:0 60:0 13:13; do k=${kk%%:*}; ml=${kk##*:}
  { echo "func f(n) {"; for i in $(seq 1 $k); do echo "  ldi q$i, $i"; done; echo "  jnz n, rec"; echo "  ret 0"; echo "rec:"; echo "  addi m, n, -1"; echo "  call r, f, m"; echo "  addi r, r, 1"; echo "  ret r"; echo "}"; echo "func main() {"; for j in $(seq 1 $ml); do echo "  ldi z$j, 0"; done; echo "  ldi d, DERINLIK"; echo "  call r, f, d"; echo "  host puti, r"; echo "}"; } > $T/st.tpl
  mk() { sed "s/DERINLIK/$1/" $T/st.tpl > $T/st.pma; $PM asm $T/st.pma -o $T/st.pmb --strip > /dev/null 2>&1; }
  lo=1; hi=70000
  while [ $((hi - lo)) -gt 1 ]; do mid=$(((lo + hi) / 2)); mk $mid; (ulimit -v 700000; timeout 20 $PM run $T/st.pmb > /dev/null 2>&1) && lo=$mid || hi=$mid; done
  for d in $((lo - 1)) $lo $hi $((hi + 1)); do mk $d; cmpone "esik:k=$k:derinlik=$d" $T/st.pmb $T/bos; done
done
# ---- 1d) desteklenmeyen girdiler: uretici reddeder ve stdout BOS kalir (kismi ELF yok)
for t in "fadd x, a, b" "host present, a, b, c" "host putf, a" "host x, poll" "host x, frame"; do
  case "$t" in host\ present*) body="ldi a, 1
  ldi b, 1
  ldi c, 1
  host r, present, a, b, c";; host\ putf*) body="ldi a, 1
  host putf, a";; host\ x*) body="$t";; *) body="ldi a, 1
  ldi b, 2
  $t";; esac
  printf 'memory 16\nfunc main() {\n  %s\n}\n' "$body" > $T/u.pma; $PM asm $T/u.pma -o $T/u.pmb --strip > /dev/null 2>&1 || { fail "desteklenmeyen ornek derlenmedi: $t"; continue; }
  $PM liste $T/u.pmb > $T/u.l; lim $GEN < $T/u.l > $T/u.elf 2>/dev/null; rc=$?; [ $rc != 0 ] && [ ! -s $T/u.elf ] && pass || fail "desteklenmeyen '$t': rc=$rc cikti=$(stat -c %s $T/u.elf)"
done
# ---- 2) bagimsizlik
$PM asm $R/tests/collatz.pma -o $T/c.pmb --strip > /dev/null; $PM liste $T/c.pmb > $T/c.l; lim $GEN < $T/c.l > $T/c.elf; chmod +x $T/c.elf
readelf -lW $T/c.elf 2>/dev/null | grep -qE "INTERP|DYNAMIC" && fail "ELF dinamik/yorumlayicili" || pass
[ "$(readelf -lW $T/c.elf 2>/dev/null | grep -c ' LOAD ')" = 1 ] && pass || fail "tek LOAD degil"
grep -qa -e "PMB1" -e "libstdc" -e "GLIBC" $T/c.elf && fail "ELF icinde motor/libc izi" || pass
[ "$(env -i $T/c.elf | head -1)" = 111 ] && pass || fail "env -i ile calismadi"
# ---- 3) dogrulayici ELF olarak: valid.pmb -> valid.elf, ayni girdilerde pm run ile ayni cikti
VP=$D/../dogr/valid.pmb; lim $GEN < <($PM liste $VP) > $T/valid.elf || fail "valid.pmb ELF'e cevrilemedi"; chmod +x $T/valid.elf
nvl=0; for p in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' | sort); do $PM asm $p -o $T/k.pmb --strip > /dev/null 2>&1 || continue; $PM liste $T/k.pmb > $T/k.l
  for v in 0 1; do if [ $v = 1 ]; then awk -v seed=$nvl -v k=2 -f ../dogr/mut.awk $T/k.l > $T/k2.l; mv $T/k2.l $T/k.l; fi
    (ulimit -v 700000; timeout 20 $PM run $VP < $T/k.l > $T/a1 2>/dev/null); ra=$?; lim $T/valid.elf < $T/k.l > $T/a2 2>/dev/null; rb=$?
    nvl=$((nvl+1)); [ $ra = $rb ] && cmp -s $T/a1 $T/a2 && pass || fail "valid.elf farkli ($p v=$v): pm rc=$ra elf rc=$rb"; done; done
for s in $(seq 1 $NV); do awk -v seed=$s -f rastgele2.awk > $T/r.pma; $PM asm $T/r.pma -o $T/r.pmb --strip > /dev/null 2>&1 || continue; $PM liste $T/r.pmb > $T/k.l
  awk -v seed=$s -v k=$((1 + s % 3)) -f ../dogr/mut.awk $T/k.l > $T/k2.l
  for f in $T/k.l $T/k2.l; do (ulimit -v 700000; timeout 20 $PM run $VP < $f > $T/a1 2>/dev/null); ra=$?; lim $T/valid.elf < $f > $T/a2 2>/dev/null; rb=$?
    nvl=$((nvl+1)); [ $ra = $rb ] && cmp -s $T/a1 $T/a2 && pass || fail "valid.elf farkli (rastgele $s): pm rc=$ra elf rc=$rb"; done; done
echo "valid.elf karsilastirmasi: $nvl girdi"
# ---- 4) sabit nokta: gen2.elf (pm'siz) kendi ELF'ini uretir
$PM liste $G2 > $T/g.l; lim $GEN < $T/g.l > $T/g1.elf; chmod +x $T/g1.elf
lim $T/g1.elf < $T/g.l > $T/g2.elf; chmod +x $T/g2.elf; lim $T/g2.elf < $T/g.l > $T/g3.elf
cmp -s $T/g1.elf $T/g2.elf && pass || fail "sabit nokta: 2. nesil farkli"
cmp -s $T/g2.elf $T/g3.elf && pass || fail "sabit nokta: 3. nesil farkli"
lim $T/g2.elf < $T/c.l | cmp -s - $T/c.elf && pass || fail "gen2.elf ile uretilen collatz ELF'i farkli"
echo "sabit nokta: gen2 ELF boyutu $(stat -c %s $T/g1.elf) bayt"
ELFGEN=$G2 ../elf/dogrula_oob.sh $PM > $T/oob.out 2>&1 && pass || { fail "gen2 OOB denetimi: $(tail -3 $T/oob.out | tr "\n" " ")"; }; ok=$((ok+$(grep -o "gecen [0-9]*" $T/oob.out | grep -o "[0-9]*"))); echo "gen2 OOB: $(tail -1 $T/oob.out)"
rm -rf $T
echo "ELF uretici gen2: gecen $ok, kalan $bad (karsilastirilan program: $n, tuzakli: $traps, uretici reddetti: $unsup, yakita bagli atlanan: $skip)"; [ $bad = 0 ]
