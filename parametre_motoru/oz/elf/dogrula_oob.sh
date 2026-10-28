#!/bin/bash
# ELF ureticisi icin bellek siniri (OOB) denetimi: ld/st, taban yazmaci x sabit ofset x bellek boyutu.
# pm run ile ELF ayni cikti, cikis kodu ve tuzak adini vermeli. Sinir: bellek[taban+ofset], 64 bit sarmali, isaretsiz karsilastirma.
# Zaman ve bellek siniri altinda calisir (ELF'te yakit siniri yok).
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); T=$(mktemp -d)
ok=0; bad=0; n=0; trapoob=0; inb=0; ret=0
fail() { bad=$((bad+1)); echo "HATA: $1"; }
kos() {   # etiket : $T/o.pma
  $PM asm $T/o.pma -o $T/o.pmb --strip > /dev/null 2>$T/ae || { fail "$1 derlenmedi: $(head -c 80 $T/ae)"; return; }
  $PM liste $T/o.pmb > $T/l.txt 2>/dev/null
  if ! $PM run $D/elf.pmy < $T/l.txt > $T/p.elf 2>/dev/null; then ret=$((ret+1)); return; fi
  chmod +x $T/p.elf; n=$((n+1))
  (ulimit -v 400000; timeout 10 $PM run $T/o.pmb < /dev/null > $T/o1 2> $T/e1); r1=$?
  (ulimit -v 400000; timeout 10 $T/p.elf < /dev/null > $T/o2 2> $T/e2); r2=$?
  if [ $r1 = 2 ]; then trapoob=$((trapoob+1)); else inb=$((inb+1)); fi
  if [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2; then ok=$((ok+1)); else fail "$1: pm rc=$r1 elf rc=$r2 ($(head -c 30 $T/e1 | tr '\n' ' ') | $(head -c 30 $T/e2 | tr '\n' ' '))"; fi
  if [ $r1 = 2 ]; then [ "$(cut -d' ' -f1-2 < $T/e1 | head -1)" = "$(cut -d' ' -f1-2 < $T/e2 | head -1)" ] && ok=$((ok+1)) || fail "$1: tuzak adi farkli"; fi
}
for N in 1 2 7 64 4096; do
  for B in 0 1 -1 $((N-1)) $N $((N+1)) 2147483647 2147483648 4294967296 9223372036854775807 -9223372036854775808; do
    for O in 0 1 -1 $((N-1)) $N $((-N)) 2147483647 -2147483648; do
      for OPK in ld st; do
        { echo "memory $N"; echo "func main() {"; echo "  mov b, $B"; echo "  mov v, 77"
          if [ $OPK = ld ]; then echo "  ld x, b, $O"; echo "  host puti, x"; else echo "  st v, b, $O"; echo "  ld x, b, $O"; echo "  host puti, x"; fi
          echo "  host putc, 10"; echo "}"; } > $T/o.pma
        kos "$OPK N=$N taban=$B ofset=$O"
      done
    done
  done
done
# hesaplanmis indeks: dongude artan adres sinira kadar dolduruluyor, sonra bir adim fazlasi (sinirda gecen son erisim ve ilk ihlal)
for N in 1 3 16 100; do
  for OPK in ld st; do
    { echo "memory $N"; echo "func main() {"; echo "  mov i, 0"; echo "L: st i, i, 0"; echo "  ld x, i, 0"; echo "  host puti, x"; echo "  host putc, 10"
      echo "  add i, i, 1"; echo "  jmp L"; echo "}"; } > $T/o.pma
    [ $OPK = ld ] && sed -i 's/^L: st i, i, 0/L: ld y, i, 0/' $T/o.pma
    kos "dongu $OPK N=$N"
  done
done
# kosullu atlama sonrasi ihlal ve yazmac uzerinden negatif indeks
for N in 4 50; do
  { echo "memory $N"; echo "func main() {"; echo "  mov i, 3"; echo "  mov s, 0"; echo "L: ld x, i, 0"; echo "  add s, s, x"; echo "  sub i, i, 1"; echo "  jnz i, L"; echo "  sub i, i, 1"; echo "  host puti, s"; echo "  host putc, 10"; echo "  st s, i, 0"; echo "  host puti, 5"; echo "}"; } > $T/o.pma
  kos "geri-say negatif N=$N"
done
rm -rf $T
echo "OOB denetimi: gecen $ok, kalan $bad (calistirilan program $n: tuzakli $trapoob, tuzaksiz $inb; uretici reddetti $ret)"; [ $bad = 0 ]
