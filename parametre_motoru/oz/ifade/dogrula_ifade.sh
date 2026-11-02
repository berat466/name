#!/bin/bash
# PLAN BOLUM 02 -- IFADELER, OPERATORLER, SABITLER: gercek C++ kaynak testleri (.cx)
#   Her olumlu test: g++ (BAGIMSIZ ORACLE) = pm ifade -> PMB1 -> {C++ pm yorumlayicisi, native ELF (gen2), sayisal svm}  (cikti birebir)
#   Beklenen cikti g++'den gelir; g++ ile farkli (belirsiz sira/uygulama tanimli) davranislar icin testin yanina .out dosyasi konur (g++ KULLANILMAZ).
#   Olumsuz testler (red/*.cx): ilk satir "// RED: KOD" -> pm ifade 1 ile cikar ve stderr o hata kodunu icerir (varsa "// GPP: red" satiri: g++ da reddeder).
# Kullanim: dogrula_ifade.sh [pm] [rastgele_sayisi] ; ORTAM: ONLY=desen  (yalniz eslesen testler), SVM=0 (svm atla)
PM=${1:-../../pm}; NR=${2:-40}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d)
export PM D R T
GEN=$R/oz/elf2/gen2.pmb; SVMB=$R/oz/sayisal/svm.pmb; TAB=$R/oz/komut/komutlar.pmk
export GEN SVMB TAB
lim() { (ulimit -v 3000000; timeout ${LIMS:-120} "$@"); }
export -f lim
one() {   # olumlu test
  f=$1; n=$(basename $f .cx); w=$T/$n; mkdir -p $w; fail() { echo "HATA: $n: $1" >> $T/hatalar; }; pass() { echo 1 >> $T/gecen; }
  if [ -f ${f%.cx}.out ]; then cp ${f%.cx}.out $w/exp; else
    g++ -std=c++20 -O0 -w -include $D/oracle.h -x c++ -o $w/bin $f 2> $w/gpp.err || { fail "g++ oracle derlenemedi ($(head -c 150 $w/gpp.err))"; return; }
    lim $w/bin > $w/exp 2>/dev/null || { fail "oracle cikis kodu != 0"; return; }
  fi
  $PM ifade $f --pmx1 $w/x.pmx --pma $w/p.pma -o $w/p.pmb --strip > /dev/null 2> $w/ifade.err || { fail "pm ifade basarisiz: $(head -c 200 $w/ifade.err)"; return; }
  # PMX1 sayi listesinden yeniden derleme: ayni PMA
  $PM ifade $f --pmx1-oku $w/x.pmx --pma $w/p2.pma > /dev/null 2>&1 && cmp -s $w/p.pma $w/p2.pma && pass || fail "PMX1 sayilarindan derleme farkli PMA uretti"
  lim $PM run $w/p.pmb > $w/pm.out 2> $w/pm.err; cmp -s $w/pm.out $w/exp && pass || fail "pm yorumlayici cikti oracle'dan farkli ($(diff $w/pm.out $w/exp 2>&1 | head -3 | tr '\n' '|' | head -c 150)) $(head -c 80 $w/pm.err)"
  $PM liste $w/p.pmb > $w/p.l 2>/dev/null
  if lim $PM run $GEN < $w/p.l > $w/p.elf 2>/dev/null; then chmod +x $w/p.elf; lim $w/p.elf > $w/elf.out 2>/dev/null; cmp -s $w/elf.out $w/exp && pass || fail "ELF cikti oracle'dan farkli ($(diff $w/elf.out $w/exp 2>&1 | head -3 | tr '\n' '|' | head -c 150))"; else fail "ELF uretilemedi"; fi
  if [ "${SVM:-1}" = 1 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=${LIMS_SVM:-300} lim $PM run $SVMB < $w/sv.in > $w/svm.out 2>/dev/null; cmp -s $w/svm.out $w/exp && pass || fail "svm cikti oracle'dan farkli ($(diff $w/svm.out $w/exp 2>&1 | head -3 | tr '\n' '|' | head -c 150))"; fi
  rm -rf $w/bin $w/p.elf
}
red() {   # olumsuz test
  f=$1; n=$(basename $f .cx); w=$T/r_$n; mkdir -p $w; fail() { echo "HATA: red/$n: $1" >> $T/hatalar; }; pass() { echo 1 >> $T/gecen; }
  code=$(head -1 $f | sed -n 's|^// RED: *\([A-Z_]*\).*|\1|p'); [ -n "$code" ] || { fail "ilk satir '// RED: KOD' degil"; return; }
  $PM ifade $f --pma $w/p.pma > /dev/null 2> $w/err; rc=$?
  [ $rc = 1 ] && grep -q "\[$code\]" $w/err && pass || fail "beklenen red [$code], rc=$rc, stderr: $(head -c 150 $w/err)"
  if grep -q '^// GPP: red' $f; then g++ -std=c++20 -fsyntax-only -w -include $D/oracle.h -x c++ $f > /dev/null 2>&1 && fail "g++ bu kodu KABUL etti (oracle ile uyumsuz red)" || pass; fi
}
trap_() {   # calisma zamani tuzagi testi: // TRAP: AD|AD2  -> pm rc=2 + TRAP adi; ELF ayni stdout ile tuzakla biter; svm @@TRAP adi
  f=$1; n=$(basename $f .cx); w=$T/t_$n; mkdir -p $w; fail() { echo "HATA: trap/$n: $1" >> $T/hatalar; }; pass() { echo 1 >> $T/gecen; }
  exp=$(head -1 $f | sed -n 's|^// TRAP: *\(.*\)$|\1|p'); [ -n "$exp" ] || { fail "ilk satir '// TRAP: AD' degil"; return; }
  $PM ifade $f --pma $w/p.pma -o $w/p.pmb --strip > /dev/null 2> $w/ifade.err || { fail "pm ifade basarisiz: $(head -c 150 $w/ifade.err)"; return; }
  lim $PM run $w/p.pmb > $w/pm.out 2> $w/pm.err; rc=$?
  [ $rc = 2 ] && grep -qE "^TRAP: ($exp)" $w/pm.err && pass || fail "pm: beklenen tuzak $exp, rc=$rc, stderr: $(head -c 100 $w/pm.err)"
  $PM liste $w/p.pmb > $w/p.l 2>/dev/null
  if lim $PM run $GEN < $w/p.l > $w/p.elf 2>/dev/null; then chmod +x $w/p.elf; lim $w/p.elf > $w/elf.out 2>/dev/null; erc=$?; [ $erc != 0 ] && cmp -s $w/elf.out $w/pm.out && pass || fail "ELF: tuzakla bitmeli (rc=$erc) ve ayni stdout"; else fail "ELF uretilemedi"; fi
  if [ "${SVM:-1}" = 1 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; lim $PM run $SVMB < $w/sv.in > $w/svm.out 2>/dev/null; { cat $w/pm.out; } > $w/svm.exp; grep -qE "^@@TRAP ($exp)" $w/svm.out && head -c $(stat -c %s $w/pm.out) $w/svm.out | cmp -s - $w/pm.out && pass || fail "svm: @@TRAP ($exp) ve ayni stdout beklenir: $(tail -c 80 $w/svm.out | tr '\n' '|')"; fi
}
export -f one red trap_
# operator tablosu: PMK1 bolum 11 (sayisal) = derlenmis C++ tablosu (pm ifade --ops) = uretilmis baslik
okh=0
{ echo "@@gor"; cat $TAB; } | $PM run $R/oz/arac/komut.pmb > $T/gor.txt 2>/dev/null
python3 $D/ops_liste.py < $T/gor.txt > $T/ops_tablo.txt 2>/dev/null && $PM ifade --ops > $T/ops_derli.txt 2>/dev/null && cmp -s $T/ops_tablo.txt $T/ops_derli.txt && echo 1 >> $T/gecen || echo "HATA: operator tablosu: komutlar.pmk bolum 11 ile derlenmis tablo (pm ifade --ops) ayni degil -- oz/komut/yenile_ifade.sh + make pm" >> $T/hatalar
python3 $R/oz/komut/gecis/ifade_ops_h.py < $T/gor.txt 2>/dev/null | cmp -s - $R/src/ifade_ops_gen.h && echo 1 >> $T/gecen || echo "HATA: src/ifade_ops_gen.h tablodan uretilmis haliyle ayni degil (oz/komut/yenile_ifade.sh)" >> $T/hatalar
ls testler/*.cx 2>/dev/null | { [ -n "$ONLY" ] && grep -E "$ONLY" || cat; } | xargs -P $(nproc) -I{} bash -c 'one {}'
ls trap/*.cx 2>/dev/null | { [ -n "$ONLY" ] && grep -E "$ONLY" || cat; } | xargs -P $(nproc) -I{} bash -c 'trap_ {}'
ls red/*.cx 2>/dev/null | { [ -n "$ONLY" ] && grep -E "$ONLY" || cat; } | xargs -P $(nproc) -I{} bash -c 'red {}'
# PMX1 sayi listesi bozma (fuzz): bozuk liste ya temiz hata (rc 1) ya da yine gecerli program (rc 0); asla cokme/sinyal
if [ -z "$ONLY" ]; then
  $PM ifade testler/uyg_agac.cx --pmx1 $T/f.pmx > /dev/null 2>&1; r=$(python3 $D/pmx_fuzz.py $PM $T/f.pmx 120 $T 2>&1); [ "$r" = "0" ] && echo 1 >> $T/gecen || echo "HATA: PMX1 fuzz: $r" >> $T/hatalar
fi
# rastgele ifade testleri (UB'siz uretec)
if [ "$NR" -gt 0 ] && [ -z "$ONLY" ]; then
  mkdir -p $T/rnd; python3 $D/rastgele_ifade.py $NR $T/rnd > /dev/null || echo "HATA: rastgele uretec basarisiz" >> $T/hatalar
  ls $T/rnd/*.cx 2>/dev/null | xargs -P $(nproc) -I{} bash -c 'one {}'
fi
ok=$(cat $T/gecen 2>/dev/null | wc -l); bad=$(grep -c . $T/hatalar 2>/dev/null); bad=${bad:-0}
[ -s $T/hatalar ] && sort $T/hatalar | head -40
echo "ifade: $ok gecti, $bad basarisiz"; rm -rf $T; [ "$bad" = 0 ]
