#!/bin/bash
# call / ret / halt / host MIKRO-ISLEM SABLONLARI (komut tablosunda `tpl prim ret|halt|call|host`) denetimi. Kullanim: dogrula_cagri.sh [pm]
# Yontem: tabloya YALNIZCA OP + SEM kaydiyla yeni komutlar eklenir (x86 sablonu YOK, C++ anlam kodu YOK):
#     STOP   (SEM: halt)                   -- HALT'un kopyasi
#     RETX s (SEM: rd t1 a ; ret t1)       -- RET'in kopyasi
#     CALLX F,b (SEM: call a b)            -- CALL'in kopyasi
#   (HOSTX sonraki adimda eklenir)
# Programlar standart komutlarla yazilir ve C++ `pm run` ile (BAGIMSIZ REFERANS) calistirilir; ayni PMB1 listesinde HALT/RET kodlari yeni komut kimlikleriyle degistirilir ("yama"),
# bu program (a) yeni tablodan uretilen gen2 ile ELF'e cevrilir (yeni komutlar yalniz mikro-islem sablonlarindan uretilir), (b) svm ile (tablo calisma aninda okunur) calistirilir.
# Karsilastirma: stdout, cikis kodu, tuzak adi, yakit (ELF'e "-1 N" oneki = pm --fuel N). Ayrica eski tablo yamali programi reddeder; zorlanmis (-6 1) mikro yolu sablon yolundan farkli bayt uretir.
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 900000; timeout ${LIMS:-20} "$@"); }
TAB=${TAB:-$R/oz/komut/komutlar.pmk}
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $R/oz/arac/komut.pmb); }
# ---- genisletilmis arac takimi (kopya agacta)
mkdir -p $T/w/oz; cp -r $R/oz/nesne $R/oz/komut $R/oz/arac $R/oz/elf2 $T/w/oz/; rm -f $T/w/oz/elf2/gen2.pmb
KM gor < $TAB | awk '{print} /^op 54 GEI/{print "op 55 STOP stop -"; print "op 56 RETX retx s"; print "op 57 CALLX callx F,b"} /^sem GEI /{print "sem STOP halt"; print "sem RETX rd t1 a ; ret t1"; print "sem CALLX call a b"}' | KM yaz > $T/w/oz/komut/komutlar.pmk
[ "$(KM dogrula < $T/w/oz/komut/komutlar.pmk)" = "1" ] && pass || fail "genisletilmis tablo gecersiz"
(cd $T/w/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) && pass || fail "genisletilmis tablodan gen2 uretilemedi"
XG=$T/w/oz/elf2/gen2.pmb; OG=$R/oz/elf2/gen2.pmb
yama() { awk -v from=$1 -v to=$2 -v nf=$3 'BEGIN{pre=1;lone=0} pre{print; if(NF==1){lone++; if(lone==2)pre=0} next} NF==nf && $1==from {$1=to} {print}'; }
# yeni komutlarla yamali liste: HALT(36)->STOP(55), RET(35)->RETX(56), CALL(34)->CALLX(57)
yamali() { yama 36 55 1 | yama 35 56 2 | yama 34 57 3; }
izle() {   # stdout, rc, tuzak adi
  lim "$@" > $T/o 2> $T/e; local r=$?; echo "rc=$r"; cat $T/o; [ $r = 2 ] && head -c 11 $T/e | cut -d' ' -f1-2
}
kos() {   # etiket pma [fuel...]
  local et=$1 pma=$2; shift 2
  $PM asm $pma -o $T/k.pmb --strip > /dev/null 2>&1 || { fail "$et: derlenemedi"; return; }
  $PM liste $T/k.pmb > $T/k.l; yamali < $T/k.l > $T/ky.l
  if grep -qE '^(55|56|57)( |$)' $T/ky.l && ! grep -qE '^(34|35|36)( |$)' $T/ky.l; then pass; else fail "$et: yama komut kimliklerini degistirmedi ya da eski komut kalmis"; fi
  lim $PM run $XG < $T/ky.l > $T/ky.elf 2>/dev/null; [ -s $T/ky.elf ] && chmod +x $T/ky.elf && pass || { fail "$et: yeni komutlu liste ELF'e cevrilemedi"; return; }
  lim $PM run $OG < $T/ky.l > $T/ko.elf 2>/dev/null; [ -s $T/ko.elf ] && fail "$et: eski gen2 bilinmeyen komut kimliklerini reddetmedi" || pass
  local fuels="${@:-0}" f
  for f in $fuels; do
    local fa="" fe=""; [ "$f" != 0 ] && { fa="--fuel $f"; }
    izle $PM run $T/k.pmb $fa < /dev/null > $T/r1
    if [ "$f" != 0 ]; then { echo "-1 $f"; cat $T/ky.l; } | lim $PM run $XG > $T/kf.elf 2>/dev/null; chmod +x $T/kf.elf; izle $T/kf.elf < /dev/null > $T/r2; else izle $T/ky.elf < /dev/null > $T/r2; fi
    cmp -s $T/r1 $T/r2 && pass || fail "$et (yakit=$f): pm [$(head -c 60 $T/r1 | tr '\n' ' ')] ELF(yeni komutlar) [$(head -c 60 $T/r2 | tr '\n' ' ')]"
  done
  { cat $T/w/oz/komut/komutlar.pmk; echo "@@PROGRAM"; cat $T/ky.l; echo "@@GIRDI"; } > $T/sv.in
  lim $PM run $R/oz/sayisal/svm.pmb < $T/sv.in > $T/sv.o 2>/dev/null; local sr=$?
  lim $PM run $T/k.pmb > $T/pm.o 2>/dev/null; local pr=$?
  if [ $pr = 0 ]; then [ $sr = 0 ] && cmp -s $T/sv.o $T/pm.o && pass || fail "$et: svm (yeni komutlar) cikti pm ile farkli"; else pass; fi
}
# ---- programlar
cat > $T/fib.pma <<'EOT'
func fib(n) {
    lti c, n, 2
    jz c, r
    ret n
r:  addi a, n, -1
    call a, fib, a
    addi b, n, -2
    call b, fib, b
    add a, a, b
    ret a
}
func main() {
    mov i, 0
l:  gei c, i, 14
    jnz c, e
    call v, fib, i
    host puti, v
    host putc, 32
    addi i, i, 1
    jmp l
e:  host putc, 10
    halt
}
entry main
EOT
cat > $T/erken.pma <<'EOT'
func f(n) {
    jnz n, nz
    host putc, 90
    host putc, 10
    halt
nz: host putc, 78
    ret n
}
func g(n) {
    call x, f, n
    host puti, x
    host putc, 10
    ret x
}
func main() {
    ldi a, 5
    call b, g, a
    ldi a, 0
    call b, g, a
    host putc, 88
    halt
}
entry main
EOT
cat > $T/derin.pma <<'EOT'
func r(n) {
    addi a, n, 1
    call b, r, a
    ret b
}
func main() {
    mov x, 0
    call y, r, x
    halt
}
entry main
EOT
{ echo "func r(n) {"; for i in $(seq 1 300); do echo "    mov v$i, $i"; done; echo "    addi a, n, 1"; echo "    call b, r, a"; echo "    ret b"; echo "}"; echo "func main() {"; echo "    mov x, 0"; echo "    call y, r, x"; echo "    halt"; echo "}"; echo "entry main"; } > $T/yigin.pma
cat > $T/dongu.pma <<'EOT'
func inc(n) {
    addi n, n, 1
    ret n
}
func main() {
    mov s, 0
    mov i, 0
l:  gei c, i, 12
    jnz c, e
    call s, inc, s
    addi i, i, 1
    jmp l
e:  host puti, s
    host putc, 10
    halt
}
entry main
EOT
cat > $T/bol.pma <<'EOT'
func d(n) {
    ldi z, 0
    div q, n, z
    ret q
}
func main() {
    ldi a, 7
    host puti, a
    call b, d, a
    host puti, b
    halt
}
entry main
EOT
cat > $T/sonsuz.pma <<'EOT'
func main() {
l:  jmp l
}
entry main
EOT
kos "fib (ozyineleme, ret)" $T/fib.pma 0 1 7 100 5000
kos "erken halt (cagrilan islevde, yigin acilmadan)" $T/erken.pma 0 3 9 25
kos "yigin tasmasi (ret/call)" $T/yigin.pma 0
kos "derinlik siniri" $T/derin.pma 0
kos "yakit sayaci dongusu" $T/dongu.pma 0 1 2 3 10 40 80 1000
kos "bolme sifir tuzagi cagrilan islevde" $T/bol.pma 0 1 2
# yigin sinirinin TAM esiginde (pm'in gectigi en buyuk ozyineleme derinligi ve +1): cagri sablonunun yerel yazmac sifirlamasi (rep stosq adedi) yigin sonunu asmamali
esikprog() {   # L
  { echo "data guard = 777, 888"; echo "func r(n) {"; for i in $(seq 1 300); do echo "    mov v$i, $i"; done; echo "    gei c, n, $1"; echo "    jnz c, e"; echo "    addi a, n, 1"; echo "    call b, r, a"; echo "e:  ret n"; echo "}"; echo "func main() {"; echo "    mov x, 0"; echo "    call y, r, x"; echo "    host puti, y"; echo "    host putc, 32"; echo "    ld g1, guard, 0"; echo "    host puti, g1"; echo "    host putc, 32"; echo "    ld g2, guard, 1"; echo "    host puti, g2"; echo "    host putc, 10"; echo "    halt"; echo "}"; echo "entry main"; } > $T/esik_$1.pma
}
esik_ok() { esikprog $1; $PM asm $T/esik_$1.pma -o $T/esik.pmb --strip > /dev/null 2>&1; lim $PM run $T/esik.pmb > /dev/null 2>&1; }
lo=1; hi=70000
while [ $((hi - lo)) -gt 1 ]; do mid=$(( (lo + hi) / 2 )); if esik_ok $mid; then lo=$mid; else hi=$mid; fi; done
[ $lo -gt 100 ] && [ $lo -lt 69000 ] && pass || fail "yigin esigi bulunamadi ($lo)"
esikprog $lo; kos "yigin esiginde (L=$lo, gecer)" $T/esik_$lo.pma 0
esikprog $((lo + 1)); kos "yigin esiginin bir fazlasi (L=$((lo+1)), STACK tuzagi)" $T/esik_$((lo + 1)).pma 0
# zorlanmis (-6 1) mikro yolu halt/ret icin de calisiyor ve sablon yolundan farkli bayt uretiyor
$PM asm $T/fib.pma -o $T/f.pmb --strip > /dev/null 2>&1; $PM liste $T/f.pmb > $T/f.l
lim $PM run $OG < $T/f.l > $T/f_a.elf 2>/dev/null; { echo "-6 1"; cat $T/f.l; } | lim $PM run $OG > $T/f_b.elf 2>/dev/null
cmp -s $T/f_a.elf $T/f_b.elf && fail "zorlanmis mikro yolu sablon yoluyla ayni bayt (halt/ret mikro sablonu kullanilmadi)" || pass
chmod +x $T/f_a.elf $T/f_b.elf; [ "$(lim $T/f_a.elf)" = "$(lim $T/f_b.elf)" ] && [ -n "$(lim $T/f_b.elf)" ] && pass || fail "zorlanmis ELF cikti farkli"
# mikro sablonlar tabloda
for pr in ret halt call; do KM gor < $TAB | grep -q "^tpl prim $pr : " && pass || fail "tablo $pr mikro-islem sablonunu icermiyor"; done
echo "call/ret/halt/host mikro-islem sablonlari: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
