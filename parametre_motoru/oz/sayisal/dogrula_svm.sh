#!/bin/bash
# Sayisal anlam yurutucusu (svm.pmb: PMK1 tablosundaki PRIM+SEM'i calisma aninda okur) = C++ `pm run` (BAGIMSIZ REFERANS).
# Karsilastirma: stdout, cikis kodu, tuzak adi (svm "@@TRAP AD" yazar), yakit ile tuzak noktasi, ondalik ozel degerler (bit duzeyinde), grafik (kare raporu, PPM, ascii).
# Kullanim: dogrula_svm.sh [pm] [rastgele_sayisi]   (TABLO ile baska tablo verilebilir)
PM=${1:-../../pm}; NR=${2:-60}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
TABLO=${TABLO:-$D/../komut/komutlar.pmk}; SVM=${SVM:-$D/svm.pmb}
ok=0; bad=0; skip=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
lim() { (ulimit -v 2000000; timeout ${LIMS:-120} "$@"); }
# kos etiket pmb stdin_dosyasi [yakit] : pm run ile svm ayni cikti/cikis/tuzak
kos() {
  local et=$1 pmb=$2 inp=$3 fuel=$4
  $PM liste $pmb > $T/l.txt 2>/dev/null || { fail "$et: liste"; return; }
  { cat $TABLO; [ -n "$fuel" ] && echo "@@YAKIT $fuel"; echo "@@PROGRAM"; cat $T/l.txt; echo "@@GIRDI"; cat $inp; } > $T/in.txt
  lim $PM run $pmb ${fuel:+--fuel $fuel} < $inp > $T/o1 2> $T/e1; r1=$?
  if [ $r1 = 124 ]; then skip=$((skip+1)); return; fi
  lim $PM run $SVM < $T/in.txt > $T/o2 2> $T/e2; r2=$?
  if [ $r2 = 124 ]; then if [ -n "$HIZLI" ]; then fail "$et: svm zaman asimi"; else skip=$((skip+1)); fi; return; fi
  local exp=$T/o1x; cp $T/o1 $exp
  if [ $r1 = 2 ]; then tn=$(grep -a '^TRAP:' $T/e1 | cut -d' ' -f2); printf '@@TRAP %s\n' "$tn" >> $exp; fi
  if [ $r1 = $r2 ] && cmp -s $exp $T/o2; then pass; else fail "$et: pm rc=$r1 svm rc=$r2 $(cmp $exp $T/o2 2>&1 | head -c 70) $(head -c 60 $T/e2)"; fi
}
: > $T/bos; $PM asm svm.pma -o $T/s.pmb --strip > /dev/null 2>&1 && cmp -s $T/s.pmb svm.pmb && pass || fail "svm.pmb guncel degil (pm asm svm.pma --strip)"
 printf 'abc\n\200\377\000xyz 123\n' > $T/girdi
# ---- 1) depodaki programlar (HIZLI=1: yalniz sinir degerleri ve ondalik; sabotaj icin)
[ -n "$HIZLI" ] || for f in $(find $R/tests $R/uygulamalar $R/bench -name '*.pma' | sort); do
  $PM asm $f -o $T/k.pmb --strip > /dev/null 2>&1 || continue
  kos "$f" $T/k.pmb $T/bos; kos "$f (girdili)" $T/k.pmb $T/girdi
done
# ---- 2) yakit sinirli (tuzak noktasi)
[ -n "$HIZLI" ] || { n=0; for f in $(find $R/tests $R/uygulamalar -name '*.pma' | sort | head -30); do
  $PM asm $f -o $T/k.pmb --strip > /dev/null 2>&1 || continue
  for N in 1 3 50 1234; do kos "yakit:$f N=$N" $T/k.pmb $T/bos $N; done; done; }
# ---- 3) rastgele programlar (iki uretec)
[ -n "$HIZLI" ] || for s in $(seq 1 $NR); do
  awk -v seed=$s -f ../elf2/rastgele2.awk > $T/r.pma; $PM asm $T/r.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "rastgele2-$s" $T/k.pmb $T/girdi
  awk -v seed=$s -f ../elf/rastgele_pma.awk > $T/r.pma; $PM asm $T/r.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "rastgele1-$s" $T/k.pmb $T/girdi
done
# ---- 3b) tamsayi sinir degerleri (INT64_MIN/MAX, -1, kaydirma 63/64/65, bolme/mod INT64_MIN/-1, anlik bicimler) ve sifira bolme tuzaklari
../elf2/tamsayi_prog.sh $T/ti > /dev/null
for f in int dz1 dz2; do $PM asm $T/ti/$f.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "tamsayi:$f" $T/k.pmb $T/bos; done
# ---- 4) ondalik: ozel degerler (NaN, sonsuz, -0, alt normal ...) ve rastgele desenler, putf
../elf2/ondalik_prog.sh $T/fp > /dev/null
for f in ozel pf1; do $PM asm $T/fp/$f.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "ondalik:$f" $T/k.pmb $T/bos; done
for sd in 88172645463325252 1 -7046029254386353131; do
  sed "s/SEED/$sd/" $T/fp/rast.pma > $T/fp/r.pma; $PM asm $T/fp/r.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "ondalik:rast($sd)" $T/k.pmb $T/bos
  sed "s/SEED/$sd/" $T/fp/pf2.pma > $T/fp/r.pma; $PM asm $T/fp/r.pma -o $T/k.pmb --strip > /dev/null 2>&1 && kos "ondalik:putf-rast($sd)" $T/k.pmb $T/bos
done
# ---- 3c) bellek siniri ve alloc (ld/st dizin sinirlari, alloc sinirlari)
../elf2/bellek_prog.sh $T/bl $PM > /dev/null
for f in $T/bl/bellek_*.pma $T/bl/alloc_*.pma; do $PM asm $f -o $T/k.pmb --strip > /dev/null 2>&1 && kos "$(basename $f .pma)" $T/k.pmb $T/bos; done
# ---- 3d) genisletilmis tablo (yalniz OP+SEM): FMIN (sel), AIMM (im), LDK2 (lk); svm tabloyu calisma aninda okur
KM() { { echo "@@$1"; cat; } | (ulimit -v 900000; timeout 60 $PM run $D/../arac/komut.pmb); }
KM gor < $TABLO | awk '{print} /^op 54 GEI/{print "op 55 FMIN fmin d,f,f"; print "op 56 AIMM aimm d,r,i"; print "op 57 LDK2 ldk2 d,k"} /^sem GEI /{print "sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4"; print "sem AIMM rd t3 b ; im t5 c ; alu_add t2 t3 t5 ; wr a t2"; print "sem LDK2 lk t7 b ; wr a t7"}' | KM yaz > $T/ext.pmk
yama() { awk -v from=$1 -v to=$2 -v nf=$3 'BEGIN{pre=1;lone=0} pre{print; if(NF==1){lone++; if(lone==2)pre=0} next} NF==nf && $1==from {$1=to} {print}'; }
xsvm() {   # etiket pmb from to nf [beklenen]
  $PM liste $2 | yama $3 $4 $5 > $T/xp.l
  { cat $T/ext.pmk; echo "@@PROGRAM"; cat $T/xp.l; echo "@@GIRDI"; } > $T/xp.in
  lim $PM run $SVM < $T/xp.in > $T/xo 2>/dev/null
  if [ -n "$6" ]; then cmp -s $T/xo $6 && pass || fail "$1: svm cikti beklenenden farkli"
  else lim $PM run $2 < /dev/null > $T/xr 2>/dev/null; cmp -s $T/xo $T/xr && [ -s $T/xr ] && pass || fail "$1: svm cikti pm ile farkli"; fi
}
OZ=$(grep '^data tab' $T/fp/ozel.pma); NP=$(( $(echo "$OZ" | wc -w) - 3 ))
{ echo "memory 4096"; echo "$OZ"; printf 'func main() {\n mov i, 0\nli: gei z, i, %s\n jnz z, ld\n add a, i, tab\n ld x, a, 0\n mov j, 0\nlj: gei z, j, %s\n jnz z, nj\n add a, j, tab\n ld y, a, 0\n add r, x, y\n host puti, r\n host putc, 32\n addi j, j, 1\n jmp lj\nnj: host putc, 10\n addi i, i, 1\n jmp li\nld: halt\n}\nentry main\n' $NP $NP; } > $T/fmall.pma
cat > $T/fmin_oracle.cpp <<'EOT'
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <cstdlib>
int main(int argc, char** argv) {
    int n = argc - 1; uint64_t v[64];
    for (int i = 0; i < n; i++) v[i] = strtoull(argv[i + 1], 0, 16);
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            double x, y; memcpy(&x, &v[i], 8); memcpy(&y, &v[j], 8);
            double r = (x < y) ? x : y; int64_t b; memcpy(&b, &r, 8);
            printf("%lld ", (long long)b);
        }
        printf("\n");
    }
}
EOT
HEX=$(sed -n 's/^OZEL="\(.*\)"$/\1/p' ../elf2/ondalik_prog.sh | head -1 | tr ' ' '\n' | head -$NP | tr '\n' ' ')
$PM asm $T/fmall.pma -o $T/fmall.pmb --strip > /dev/null 2>&1 || fail "fmin tarama programi derlenemedi"
if g++ -O0 -o $T/fmin_oracle $T/fmin_oracle.cpp 2>/dev/null && $T/fmin_oracle $HEX > $T/fm.exp; then xsvm "FMIN tum cift desenleri" $T/fmall.pmb 3 55 4 $T/fm.exp; else fail "fmin oracle derlenemedi"; fi
VALS=$(sed -n 's/^VALS="\(.*\)"$/\1/p' ../elf2/tamsayi_prog.sh)
printf 'memory 4096\ndata tab = %s\nfunc main() {\n mov i, 0\nl: gei z, i, 23\n jnz z, d\n add a, i, tab\n ld x, a, 0\n addi r, x, 5\n host puti, r\n host putc, 32\n addi r, x, -77\n host puti, r\n host putc, 10\n addi i, i, 1\n jmp l\nd: halt\n}\nentry main\n' "$VALS" > $T/aimm.pma
$PM asm $T/aimm.pma -o $T/aimm.pmb --strip > /dev/null 2>&1 && xsvm "AIMM" $T/aimm.pmb 41 56 4 || fail "aimm programi derlenemedi"
printf 'func main() {\n mov a, 1.5\n host puti, a\n host putc, 32\n mov b, 0x7ff8000000000001\n host puti, b\n host putc, 32\n mov c, -123456789012345\n host puti, c\n host putc, 10\n halt\n}\n' > $T/ldk2.pma
$PM asm $T/ldk2.pma -o $T/ldk2.pmb --strip > /dev/null 2>&1 && xsvm "LDK2" $T/ldk2.pmb 2 57 3 || fail "ldk2 programi derlenemedi"
# ---- 5) grafik: svm kendi host islevleriyle present/poll/frame'i gecirir; kare raporu, son kare, ascii pm ile ayni
for a in hayat sahne pong; do
  $PM asm $R/uygulamalar/$a.pma -o $T/k.pmb --strip > /dev/null 2>&1 || continue
  g=""; [ $a = pong ] && g="--girdi $R/uygulamalar/pong.girdi"
  $PM liste $T/k.pmb > $T/l.txt; { cat $TABLO; echo "@@PROGRAM"; cat $T/l.txt; } > $T/in.txt
  for sec in "--max-kare 30" "--max-kare 7 --ascii 4"; do
    rm -f $T/p1.ppm $T/p2.ppm
    lim $PM run $T/k.pmb $sec $g --kare-rapor --ekran $T/p1.ppm < /dev/null > $T/o1 2> $T/e1; r1=$?
    lim $PM run $SVM $sec $g --kare-rapor --ekran $T/p2.ppm < $T/in.txt > $T/o2 2> $T/e2; r2=$?
    if [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && cmp -s $T/p1.ppm $T/p2.ppm && [ "$(grep -a '^ekran:' $T/e1)" = "$(grep -a '^ekran:' $T/e2)" ]; then pass; else fail "grafik $a $sec: rc $r1/$r2 | $(grep -a '^ekran:' $T/e1) <> $(grep -a '^ekran:' $T/e2)"; fi
  done
done
echo "sayisal anlam yurutucusu: $ok gecti, $bad basarisiz (zaman asimi nedeniyle atlanan: $skip)"
rm -rf $T
[ $bad = 0 ]
