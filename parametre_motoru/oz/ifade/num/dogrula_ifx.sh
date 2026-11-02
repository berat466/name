#!/bin/bash
# PLAN BOLUM 02 (duzeltme) -- SAYISAL PMX1 DERLEYICISI (ifx.pmb) DOGRULAMASI
#  * ifx.pmb, komutlar.pmk bolum 12 kural tablosunu CALISMA ANINDA okur; C++ ifade derleyicisi (pm ifade) yalniz METIN -> PMX1 icin (istege bagli uyumluluk araci) cagrilir.
#  * Her cekirdek test:  g++ (bagimsiz oracle) = [ifx.elf (native) | ifx.pmb (pm)] ayni PMA ; asm.elf -> coz.elf -> valid.elf (kabul) -> gen2.elf (ELF) ; ELF = pm = svm = g++ ciktisi.
#  * pm'siz zincir: ifx.elf, asm.elf, coz.elf, valid.elf, gen2.elf (oz/bag/kur.sh onyuklemesi); pm yalniz onyukleme ve bagimsiz referans.
#  * Kural degisikligi: yalniz tablo sayilari degisir (ifx.pmb ve hicbir kaynak yeniden uretilmez), davranis degisir.
# Kullanim: dogrula_ifx.sh [pm]   (ORTAM: SVM=0 svm atla)
PM=${1:-../../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../../..; T=$(mktemp -d); export PM D R T
TAB=${TAB:-$R/oz/komut/komutlar.pmk}; IFXB=${IFXB:-$D/ifx.pmb}; export TAB IFXB   # TAB/IFXB disaridan verilebilir (sabotaj)
lim() { (ulimit -v 3000000; timeout ${LIMS:-180} "$@"); }; export -f lim
pass() { echo 1 >> $T/gecen; }; fail() { echo "HATA: $1" >> $T/hatalar; }; export -f pass fail
# 0) tazelik: ifx.pmb = ifx.cx'in (gelistirme gorunumu) derlemesi
[ -n "$SABOTAJ" ] || { $PM ifade ifx.cx -o $T/ifx_yeni.pmb --strip > /dev/null 2>&1 && cmp -s $T/ifx_yeni.pmb $IFXB; } && pass || fail "ifx.pmb guncel degil (pm ifade num/ifx.cx -o num/ifx.pmb --strip)"
# 1) pm'siz zincir araclari (onyukleme)
$R/oz/bag/kur.sh $PM $T/bag > /dev/null 2>&1 || { echo "kur.sh basarisiz"; exit 1; }
$PM liste $IFXB > $T/ifx.l && $T/bag/gen2.elf < $T/ifx.l > $T/ifx.elf && chmod +x $T/ifx.elf && pass || fail "ifx.elf uretilemedi"
export IFXE=$T/ifx.elf BAG=$T/bag
one() {
  f=$1; n=$(basename $f .cx); w=$T/t_$n; mkdir -p $w
  g++ -std=c++20 -O0 -w -include $D/../oracle.h -x c++ -o $w/bin $f 2> $w/gpp.err && lim $w/bin > $w/exp || { fail "$n: g++ oracle"; return; }
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2> $w/fe.err; [ -s $w/x.pmx ] || { fail "$n: on yuz (PMX1 uretimi): $(head -c 120 $w/fe.err)"; return; }   # on yuz anlam asamasi reddetse de PMX1 yazilmistir
  cat $TAB $w/x.pmx > $w/in.txt
  lim $IFXE < $w/in.txt > $w/p_elf.pma 2> $w/ifx.err || { fail "$n: ifx.elf basarisiz: $(grep -a HATA $w/p_elf.pma | head -c 150)"; return; }
  lim $PM run $IFXB < $w/in.txt > $w/p_pm.pma 2>/dev/null; cmp -s $w/p_elf.pma $w/p_pm.pma && pass || fail "$n: ifx.elf PMA'si ifx.pmb (pm) PMA'sindan farkli"
  lim $BAG/asm.elf < $w/p_elf.pma > $w/p.pmb 2> $w/asm.err && [ -s $w/p.pmb ] && pass || { fail "$n: asm.elf: $(head -c 120 $w/asm.err)"; return; }
  lim $BAG/coz.elf < $w/p.pmb > $w/p.l && pass || { fail "$n: coz.elf"; return; }
  v=$(lim $BAG/valid.elf < $w/p.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "$n: sayisal dogrulayici (valid.elf) reddetti: $v"
  lim $BAG/gen2.elf < $w/p.l > $w/p.elf && chmod +x $w/p.elf && lim $w/p.elf > $w/elf.out 2>/dev/null; cmp -s $w/elf.out $w/exp && pass || fail "$n: ELF cikti g++'dan farkli ($(diff $w/elf.out $w/exp 2>&1 | head -3 | tr '\n' '|' | head -c 150))"
  lim $PM run $w/p.pmb > $w/pm.out 2>/dev/null; cmp -s $w/pm.out $w/exp && pass || fail "$n: pm cikti g++'dan farkli"
  if [ "${SVM:-1}" = 1 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=300 lim $PM run $R/oz/sayisal/svm.pmb < $w/sv.in > $w/svm.out 2>/dev/null; cmp -s $w/svm.out $w/exp && pass || fail "$n: svm cikti g++'dan farkli"; fi
  # onceki (C++ yolu) ile ayni davranis
  case $n in f80_*) ;; *) $PM ifade $f -o $w/c.pmb --strip > /dev/null 2>&1 && lim $PM run $w/c.pmb > $w/cpp.out 2>/dev/null; cmp -s $w/cpp.out $w/exp && pass || fail "$n: C++ yolu cikti g++'dan farkli";; esac
}
red() {
  f=$1; grep -q '^// IFX: kabul' $f && return; n=$(basename $f .cx); w=$T/r_$n; mkdir -p $w; exp=$(head -1 $f | sed -n 's|^// HATA: *\(.*\)$|\1|p'); code=$(head -1 $f | sed -n 's|^// RED: *\([A-Z_]*\).*|\1|p')
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2> $w/fe.err
  if [ ! -s $w/x.pmx ]; then   # metin duzeyi (lexer/ayristirici) hatasi: on yuz reddeder, PMX1 hic olusmaz
    [ -n "$code" ] && grep -q "\[$code\]" $w/fe.err && pass || fail "red/$n: on yuz soz dizimi reddi beklenen [$code]: $(head -c 100 $w/fe.err)"; return
  fi
  cat $TAB $w/x.pmx | lim $IFXE > $w/o.pma 2>/dev/null; rc=$?
  if [ -n "$code" ]; then [ $rc != 0 ] && grep -aq "@@HATA $code " $w/o.pma && pass || fail "red/$n: sayisal derleyici beklenen [$code], rc=$rc: $(grep -a HATA $w/o.pma | head -c 100)"
  else [ $rc != 0 ] && grep -aq "@@HATA .*$exp" $w/o.pma && pass || fail "red/$n: beklenen ayrik ret '$exp', rc=$rc: $(grep -a HATA $w/o.pma | head -c 100)"; fi
}
export -f one red
ls testler/*.cx ../testler/*.cx | xargs -P $(nproc) -I{} bash -c 'one {}'
ls red/*.cx ../red/*.cx | xargs -P $(nproc) -I{} bash -c 'red {}'
# 2) kural degisikligi: tablo sayilari degisir, hicbir sey yeniden derlenmez
printf 'int main() { pi(7 + 3); pi(7 - 3); pi(7 * 3); return 0; }\n' > $T/k.cx
$PM ifade $T/k.cx --pmx1 $T/k.pmx > /dev/null
run_tab() { cat $1 $T/k.pmx | $T/ifx.elf > $T/k.pma && $T/bag/asm.elf < $T/k.pma > $T/k.pmb && $PM run $T/k.pmb | tr '\n' ' '; }
a=$(run_tab $TAB)
python3 $D/kural_degistir.py '+' add sub < $TAB > $T/tab2.pmk && b=$(run_tab $T/tab2.pmk)
python3 $D/kural_degistir.py '*' mul add < $TAB > $T/tab3.pmk && c=$(run_tab $T/tab3.pmk)
[ "$a" = "10 4 21 " ] && pass || fail "kural: ana tablo cikti '$a'"
[ "$b" = "4 4 21 " ] && pass || fail "kural: '+' -> sub degisikligi davranisa yansimadi ('$b')"
[ "$c" = "10 4 10 " ] && pass || fail "kural: '*' -> add degisikligi davranisa yansimadi ('$c')"
# 3) bozuk tablo/PMX1: temiz hata (cokme yok)
head -c 2000 $TAB > $T/kisa.pmk; cat $T/kisa.pmk $T/k.pmx | lim $T/ifx.elf > /dev/null 2>&1; [ $? -ne 139 ] && pass || fail "kisa tablo: cokme"
ok=$(cat $T/gecen 2>/dev/null | wc -l); bad=$(grep -c . $T/hatalar 2>/dev/null); bad=${bad:-0}
[ -s $T/hatalar ] && sort $T/hatalar | head -30
echo "ifx: $ok gecti, $bad basarisiz"; rm -rf $T; [ "$bad" = 0 ]
