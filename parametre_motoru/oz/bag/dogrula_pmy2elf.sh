#!/bin/bash
# UCTAN UCA BAGIMSIZ URETIM: yeni bir PMY1 tanimindan, YALNIZ ELF araclarla (yap.elf -> asm.elf -> coz.elf -> valid.elf -> gen2.elf; pm/g++/awk/sed yok)
# bagimsiz ELF uretilir; ELF'in davranisi `pm run tanim.pmy` (C++ yorumlayici, YALNIZ referans) ile ayni olmali.
# Kullanim: dogrula_pmy2elf.sh [pm] [rastgele_model_sayisi]   (BAG: arac ELF dizini; yoksa kur.sh ile gecici kurulur)
PM=${1:-../../pm}; NR=${2:-150}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0; uret=0; ret=0; skip=0; gk=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
if [ -z "$BAG" ]; then ./kur.sh $PM $T/bag > /dev/null || { echo "kur.sh basarisiz"; exit 1; }; BAG=$T/bag; fi; export BAG
mkdir -p $T/bin; for c in bash cat chmod cmp cp dirname ls mkdir mktemp realpath rm stat timeout head; do p=$(command -v $c) && ln -sf $p $T/bin/$c; done
PF() { env -i PATH=$T/bin BAG="$BAG" bash "$@"; }       # pm/g++/awk/sed erisilemez ortam
gr() {   # grafik: ayni model, ELF secenekleri (olay, azami kare, ascii, kare raporu, son kare) = pm run secenekleri
  printf '0 1\n2 5\n4 7\n' > $T/ev.txt
  ONEK="-2 3 0 1 2 5 4 7 -3 5 -4 4 -5 3" PF ./zincir.sh "$2" $T/g.elf > /dev/null 2>&1 || { fail "$1: grafik ELF uretilemedi"; return; }
  rm -f $T/pm.ppm $T/g.ppm
  (ulimit -v 900000; timeout 10 $PM run "$2" --max-kare 5 --ascii 4 --girdi $T/ev.txt --kare-rapor --ekran $T/pm.ppm < /dev/null > $T/go1 2> $T/ge1); g1=$?
  (ulimit -v 900000; timeout 10 $T/g.elf < /dev/null > $T/go2 2> $T/ge2 3> $T/g.ppm); g2=$?
  [ -s $T/g.ppm ] || rm -f $T/g.ppm
  if [ $g1 = 124 ] || [ $g1 = 134 ] || grep -q "TRAP: FUEL" $T/ge1; then return; fi
  gk=$((gk+1)); grep -a '^ekran:' $T/ge1 > $T/gr1; grep -a '^ekran:' $T/ge2 > $T/gr2
  [ $g1 = $g2 ] && cmp -s $T/go1 $T/go2 && cmp -s $T/gr1 $T/gr2 && [ "$(grep -a '^TRAP:' $T/ge1 | cut -d" " -f1-2)" = "$(grep -a '^TRAP:' $T/ge2 | cut -d" " -f1-2)" ] && { [ ! -e $T/pm.ppm ] && [ ! -e $T/g.ppm ] || cmp -s $T/pm.ppm $T/g.ppm; } && pass || fail "$1: grafik davranis farkli (pm rc=$g1, elf rc=$g2; $(head -c 80 $T/gr1) <> $(head -c 80 $T/gr2))"
}
one() {   # etiket tanim.pmy
  IFS= read -r -n 4 H0 < "$2"; [ "$H0" = "PMY1" ] || return
  rm -f $T/u.elf; PF ./zincir.sh "$2" $T/u.elf > $T/z.out 2>&1; rz=$?
  $PM yapisal "$2" -o $T/ref.pmb --strip > /dev/null 2>&1; rr=$?
  if [ $rr != 0 ]; then [ $rz != 0 ] && [ ! -e $T/u.elf ] && { pass; ret=$((ret+1)); } || fail "$1: C++ reddetti ama zincir ELF uretti"; return; fi
  if [ $rz != 0 ]; then
    # yalniz desteklenmeyen ana makine/kayan nokta yuzunden reddedilmeli: pm'in kendi uretici-destek denetimiyle ayni karar (gen2 pm ile)
    $PM liste $T/ref.pmb > $T/ref.l 2>/dev/null; (ulimit -v 900000; timeout 30 $PM run $R/oz/elf2/gen2.pmb < $T/ref.l > $T/ref.elf 2>/dev/null) && fail "$1: zincir reddetti (rc=$rz: $(head -c 80 $T/z.out)) ama gen2 (pm ile) uretebiliyor" || { pass; ret=$((ret+1)); }
    return; fi
  uret=$((uret+1))
  (ulimit -v 900000; timeout 10 $PM run "$2" < /dev/null > $T/o1 2> $T/e1); r1=$?
  if [ $r1 = 124 ] || [ $r1 = 134 ] || grep -q "TRAP: FUEL" $T/e1; then skip=$((skip+1)); return; fi
  (ulimit -v 900000; timeout 10 $T/u.elf < /dev/null > $T/o2 2> $T/e2); r2=$?
  [ $r1 = $r2 ] && cmp -s $T/o1 $T/o2 && pass || fail "$1: davranis farkli (pm rc=$r1, elf rc=$r2)"
  gr "$1" "$2"
  # uretilen ELF'in PMB1'i C++ zincirinin PMB1'iyle ayni mi? (yap.elf|asm.elf == pm yapisal -o)
  (ulimit -v 900000; $BAG/yap.elf < "$2" 2>/dev/null | $BAG/asm.elf > $T/mine.pmb 2>/dev/null); cmp -s $T/ref.pmb $T/mine.pmb && pass || fail "$1: PMB1 farkli (yap.elf|asm.elf != pm yapisal)"
}
# ---- 1) depodaki PMY1 tablolar ve sayi listeleri
n=0; for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 -name '*.pmy' | sort); do one "pmy $f" $f; n=$((n+1)); done
for f in $(find $R/oz $R/ai_testi2 $R/ai_testi3 $R/ai_testi4 -name '*.sayi' | sort); do awk -f $R/ai_testi3/paketle.awk $f > $T/s.pmy 2>/dev/null && { one "sayi $f" $T/s.pmy; n=$((n+1)); }; done
# ---- 2) elle yazilmis metin uygulamalari (sayi listesi): beklenen cikti da elle verilir
app() { echo "$3" | awk -f $R/ai_testi3/paketle.awk > $T/app.pmy; one "uygulama:$1" $T/app.pmy
  rm -f $T/a.elf; PF ./zincir.sh $T/app.pmy $T/a.elf > /dev/null 2>&1 && [ "$($T/a.elf 2>&1 | tr "\n" " ")" = "$2" ] && pass || fail "uygulama:$1: beklenen cikti '$2' degil"; }
app toplam_kareler "51" "80 77 89 49 0 0 0 1 1 z0 0 0 0 1 10 2 2 z1 z1 3 3 z0 z1 0 0 0 0 1 0 1 0 15 3 2 0 2 1 1 z1 0 0 1 8 3 0"
app kare_say "0 1 4 9 16 " "80 77 89 49 0 0 0 1 0 0 0 0 1 5 1 2 z0 z1 1 1 2 0 15 3 2 0 2 0 0 0 0 0 0 0 1 12 5 11 0 0 1 z0 11 0 0 1 z1 11 0 0 1 z2 11 0 0 1 z3 11 0 0 1 z4"
echo "uygulama sayisi: $n"
# ---- 3) rastgele PMY1 modelleri (metin cikisli olanlar ELF olur; ekran/girdi/kayan nokta olanlar reddedilir)
for s in $(seq 1 $NR); do awk -v seed=$s -f $D/../yap/rastgele_pmy.awk | awk -f $R/ai_testi3/paketle.awk > $T/r.pmy 2>/dev/null || continue; one "rastgele-$s" $T/r.pmy; done
rm -rf $T
echo "uctan uca (PMY1 -> ELF, yalniz ELF araclar): gecen $ok, kalan $bad (ELF uretilen $uret, reddedilen $ret, zaman asimi nedeniyle atlanan $skip; grafik secenekli karsilastirma $gk)"; [ $bad = 0 ]
