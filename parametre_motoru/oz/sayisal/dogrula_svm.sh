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
