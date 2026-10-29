#!/bin/bash
# 6. ASAMA + 7: motor kendi yeni surumunu uretir (pm, g++, awk, sed, od, tr, grep OLMADAN) ve eski surumu geri uretir.
#   1) kur.sh (pm, BIR KEZ): mevcut kaynaklardan E2 arac takimi (yakitli uretici)
#   2) yenile.sh (yalniz bash+cat/cmp/cp/mkdir/rm/chmod/stat/timeout... PATH'inde pm/g++/awk/sed yok): E2, v1 kaynaklarindan (nesil_v1: yakitsiz uretici) E1'i uretir
#   3) E1, mevcut kaynaklardan E2b'yi uretir: E2b == E2 (10 ELF bayt-bayt)  [A -> B -> A turu]
#   4) E1.gen2.elf == pm ile v1 kaynaklarindan kurulan gen2.elf   5) yeni takim test paketlerinden gecer (TAM=1)
# Kullanim: dogrula_yenile.sh [pm]
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
mkdir -p $T/bin; for c in bash cat chmod cmp cp dirname ls mkdir mktemp realpath rm stat timeout; do p=$(command -v $c) && ln -sf $p $T/bin/$c; done
[ -e $T/bin/pm ] || [ -e $T/bin/awk ] || [ -e $T/bin/sed ] || pass
PF() { env -i PATH=$T/bin BAG="$BAG" bash "$@"; }       # pm/g++/awk/sed erisilemez ortam
./kur.sh $PM $T/E2 > /dev/null || { echo "kur.sh basarisiz"; exit 1; }
# v1 kaynak agaci: mevcut agac, yalniz uretici kaynaklari v1
R1=$T/R1; mkdir -p $R1/oz; cp -r $R/oz/komut $R1/oz/; cp -r $R/oz/arac $R/oz/asm $R/oz/elf2 $R/oz/dogr $R/oz/yap $R/oz/elf $R1/oz/; cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $R1/oz/
# v1 sablonlari: mevcut tablonun "tokens" bolumunden onceki kismi + v1 sablon bolumleri (nesil_v1/sablonlar.txt: okunabilir gorunum)
{ echo "@@gor"; cat $R/oz/komut/komutlar.pmk; } | $PM run $R/oz/arac/komut.pmb | awk '/^tokens /{exit} {print}' > $T/v1.txt; cat nesil_v1/sablonlar.txt >> $T/v1.txt
{ echo "@@yaz"; cat $T/v1.txt; } | $PM run $R/oz/arac/komut.pmb > $R1/oz/komut/komutlar.pmk || { echo "v1 tablosu uretilemedi"; exit 1; }
cp nesil_v1/gen2.pma.in $R1/oz/elf2/gen2.pma.in
PF ./yenile.sh $T/E2 $T/E1 $R1 > $T/y1.out 2>&1 && pass || { fail "yenile (E2 -> E1) basarisiz: $(tail -2 $T/y1.out)"; }
PF ./yenile.sh $T/E1 $T/E2b $R > $T/y2.out 2>&1 && pass || { fail "yenile (E1 -> E2b) basarisiz: $(tail -2 $T/y2.out)"; }
for b in gen2 asm valid yap doldur opstab fmtc komut paketle coz yaz; do cmp -s $T/E2/$b.elf $T/E2b/$b.elf && pass || fail "tur: $b.elf E2 ile E2b farkli"; done
# E1.gen2.elf, v1 kaynaklarindan pm ile kurulanla ayni mi?
(cd $R1/oz/elf2 && PM=$PM ./uret.sh $PM > /dev/null 2>&1) && $PM liste $R1/oz/elf2/gen2.pmb > $T/g1.l && $PM run $R1/oz/elf2/gen2.pmb < $T/g1.l > $T/g1.elf && cmp -s $T/g1.elf $T/E1/gen2.elf && pass || fail "E1.gen2.elf v1 kaynaklarindan pm ile kurulanla ayni degil"
cmp -s $T/E1/gen2.elf $T/E2/gen2.elf && fail "E1 ve E2 uretici ayni (kaynak degisikligi etkisiz)" || pass
# yeni takim: yakit onekli uretim calisir
$PM asm $R/tests/collatz.pma -o $T/c.pmb --strip > /dev/null; $PM liste $T/c.pmb > $T/c.l
{ printf -- '-1 20\n'; cat $T/c.l; } | $T/E2b/gen2.elf > $T/cf.elf 2>/dev/null && chmod +x $T/cf.elf && $T/cf.elf 2>&1 | grep -q "TRAP: FUEL" && pass || fail "E2b yakit oneki calismadi"
# zincir (yalniz ELF'ler): PMB1 ve PMY1 girdileri
for f in tests/collatz.pma tests/gcd.pma tests/heap.pma bench/fib35.pma; do $PM asm $R/$f -o $T/z.pmb --strip > /dev/null; rm -f $T/z.elf; BAG=$T/E2b PF ./zincir.sh $T/z.pmb $T/z.elf > /dev/null 2>&1 && $T/z.elf > $T/o2 2>&1; $PM run $T/z.pmb > $T/o1 2>&1; cmp -s $T/o1 $T/o2 && pass || fail "yeni takim zinciri (pmb): $f"; done
# assembler ELF'i (yeni takim) pm asm ile ayni
for f in $R/tests/*.pma $R/uygulamalar/*.pma; do $PM asm $f -o $T/r.pmb --strip > /dev/null 2>&1; rr=$?; (ulimit -v 900000; timeout 30 $T/E2b/asm.elf < $f > $T/m.pmb 2>/dev/null); rm_=$?
  if [ $rr = 0 ]; then { [ $rm_ = 0 ] && cmp -s $T/r.pmb $T/m.pmb; } && pass || { grep -q "HATA [0-9]*: " $T/m.pmb && pass || fail "asm.elf (E2b) farkli: $f"; }; else [ $rm_ != 0 ] && pass || fail "asm.elf (E2b) reddetmedi: $f"; fi; done
echo "--- yenile (E2 -> E1):"; cat $T/y1.out; echo "--- yenile (E1 -> E2b):"; cat $T/y2.out
if [ -n "$TAM" ]; then
  (cd $R/oz/elf2 && GENELF=$T/E2b/gen2.elf ./dogrula_elf2.sh $PM 100 100 2>&1 | tail -3)
  (cd $R/oz/asm && ASMELF=$T/E2b/asm.elf ASMPMB=$R/oz/asm/asm.pmb ./dogrula_asm.sh $PM 50 400 2>&1 | tail -2)
  (cd $R/oz/yap && YAPELF=$T/E2b/yap.elf ASMELF=$T/E2b/asm.elf ./dogrula_yap.sh $PM 100 400 2>&1 | tail -2)
fi
rm -rf $T
echo "motor yenileme (pm'siz): gecen $ok, kalan $bad"; [ $bad = 0 ]
