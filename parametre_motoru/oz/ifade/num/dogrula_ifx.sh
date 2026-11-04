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
imzayap() { { echo "@@imza"; cat $1; } | $T/bag/komut.elf | tr '\n' ' '; echo; }   # OPSIG dokumu (sayisal arac komut.elf); ifx ve tipdogr ayni imzayi okur
imzayap $TAB > $T/imza.txt; export IMZ=$T/imza.txt
$PM liste $R/oz/tip/tipdogr.pmb > $T/tip.l && $T/bag/gen2.elf < $T/tip.l > $T/tipdogr.elf && chmod +x $T/tipdogr.elf && pass || fail "tipdogr.elf uretilemedi"
export TIPE=$T/tipdogr.elf
one() {
  f=$1; n=$(basename $f .cx); w=$T/t_$n; mkdir -p $w
  if [ -f ${f%.cx}.out ]; then cp ${f%.cx}.out $w/exp   # g++ karsiligi olmayan yapilar (__match, korutin): bagimsiz elle/python ile hesaplanmis beklenen cikti
  else
  STDV=c++20; grep -q '^// STD: c++23' $f && STDV=c++23; g++ -std=$STDV -O0 -w -frounding-math -fext-numeric-literals -include $D/../oracle.h -x c++ -o $w/bin $f 2> $w/gpp.err && lim $w/bin > $w/exp || { fail "$n: g++ oracle"; return; }
  fi
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2> $w/fe.err; [ -s $w/x.pmx ] || { fail "$n: on yuz (PMX1 uretimi): $(head -c 120 $w/fe.err)"; return; }   # on yuz anlam asamasi reddetse de PMX1 yazilmistir
  { cat $TAB; cat $IMZ; echo; cat $w/x.pmx; } > $w/in.txt
  lim $IFXE < $w/in.txt > $w/p_elf.pma 2> $w/ifx.err || { fail "$n: ifx.elf basarisiz: $(grep -a HATA $w/p_elf.pma | head -c 150)"; return; }
  lim $PM run $IFXB < $w/in.txt > $w/p_pm.pma 2>/dev/null; cmp -s $w/p_elf.pma $w/p_pm.pma && pass || fail "$n: ifx.elf PMA'si ifx.pmb (pm) PMA'sindan farkli"
  lim $BAG/asm.elf < $w/p_elf.pma > $w/p.pmb 2> $w/asm.err && [ -s $w/p.pmb ] && pass || { fail "$n: asm.elf: $(head -c 120 $w/asm.err)"; return; }
  lim $BAG/coz.elf < $w/p.pmb > $w/p.l && pass || { fail "$n: coz.elf"; return; }
  # PMT1 tur notu (ifx.elf uretti) -> sayisal tur dogrulayicisi (tipdogr): OPSIG/HOSTSIG/donusum kurallari TABLODAN
  grep -a '^;;PMT1' $w/p_elf.pma | sed 's/^;;PMT1 //' > $w/p.t; [ -s $w/p.t ] || { fail "$n: PMT1 notu yok"; return; }
  tv=$( { echo '@@IMZA'; cat $IMZ; echo '@@PROGRAM'; cat $w/p.l; echo '@@TIPLER'; cat $w/p.t; } | lim $TIPE | head -2 | tr '\n' ' '); [ "${tv%% *}" = 1 ] && pass || fail "$n: tipdogr reddetti: $tv"
  v=$(lim $BAG/valid.elf < $w/p.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "$n: sayisal dogrulayici (valid.elf) reddetti: $v"
  lim $BAG/gen2.elf < $w/p.l > $w/p.elf && chmod +x $w/p.elf && lim $w/p.elf > $w/elf.out 2>/dev/null; cmp -s $w/elf.out $w/exp && pass || fail "$n: ELF cikti g++'dan farkli ($(diff $w/elf.out $w/exp 2>&1 | head -3 | tr '\n' '|' | head -c 150))"
  lim $PM run $w/p.pmb > $w/pm.out 2>/dev/null; cmp -s $w/pm.out $w/exp && pass || fail "$n: pm cikti g++'dan farkli"
  if [ "${SVM:-1}" = 1 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=300 lim $PM run $R/oz/sayisal/svm.pmb < $w/sv.in > $w/svm.out 2>/dev/null; cmp -s $w/svm.out $w/exp && pass || fail "$n: svm cikti g++'dan farkli"; fi
  # onceki (C++ yolu) ile ayni davranis
  case $n in f80_*|f128_*|ca_*|istisna_*|bellek_gorunum|bayt_gorunum) ;; *) $PM ifade $f -o $w/c.pmb --strip > /dev/null 2>&1 && lim $PM run $w/c.pmb > $w/cpp.out 2>/dev/null; cmp -s $w/cpp.out $w/exp && pass || fail "$n: C++ yolu cikti g++'dan farkli";; esac
}
red() {
  f=$1; grep -q '^// IFX: kabul' $f && return; n=$(basename $f .cx); w=$T/r_$n; mkdir -p $w; exp=$(head -1 $f | sed -n 's|^// HATA: *\(.*\)$|\1|p'); code=$(head -1 $f | sed -n 's|^// RED: *\([A-Z_]*\).*|\1|p')
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2> $w/fe.err
  if [ ! -s $w/x.pmx ]; then   # metin duzeyi (lexer/ayristirici) hatasi: on yuz reddeder, PMX1 hic olusmaz
    [ -n "$code" ] && grep -q "\[$code\]" $w/fe.err && pass || fail "red/$n: on yuz soz dizimi reddi beklenen [$code]: $(head -c 100 $w/fe.err)"; return
  fi
  { cat $TAB; cat $IMZ; echo; cat $w/x.pmx; } | lim $IFXE > $w/o.pma 2>/dev/null; rc=$?
  if [ -n "$code" ]; then [ $rc != 0 ] && grep -aq "@@HATA $code " $w/o.pma && pass || fail "red/$n: sayisal derleyici beklenen [$code], rc=$rc: $(grep -a HATA $w/o.pma | head -c 100)"
  else [ $rc != 0 ] && grep -aq "@@HATA .*$exp" $w/o.pma && pass || fail "red/$n: beklenen ayrik ret '$exp', rc=$rc: $(grep -a HATA $w/o.pma | head -c 100)"; fi
}
trap_n() {   # calisma zamani tuzagi (sayisal yol): // TRAP: AD -> pm rc=2 + stderr TRAP; ELF tuzakla biter; svm @@TRAP; tuzaktan once yazilan cikti .out ile ayni
  f=$1; n=$(basename $f .cx); w=$T/n_$n; mkdir -p $w; exp=$(head -1 $f | sed -n 's|^// TRAP: *\(.*\)$|\1|p'); [ -n "$exp" ] || { fail "trap/$n: ilk satir '// TRAP: AD' degil"; return; }
  $PM ifade $f --pmx1 $w/x.pmx > /dev/null 2>&1; [ -s $w/x.pmx ] || { fail "trap/$n: PMX1 yok"; return; }
  { cat $TAB; cat $IMZ; echo; cat $w/x.pmx; } | lim $IFXE > $w/p.pma 2>/dev/null || { fail "trap/$n: ifx basarisiz: $(grep -a HATA $w/p.pma | head -c 120)"; return; }
  lim $BAG/asm.elf < $w/p.pma > $w/p.pmb 2>/dev/null && lim $BAG/coz.elf < $w/p.pmb > $w/p.l || { fail "trap/$n: asm/coz"; return; }
  grep -a '^;;PMT1' $w/p.pma | sed 's/^;;PMT1 //' > $w/p.t; tv=$( { echo '@@IMZA'; cat $IMZ; echo '@@PROGRAM'; cat $w/p.l; echo '@@TIPLER'; cat $w/p.t; } | lim $TIPE | head -1 ); [ "${tv%% *}" = 1 ] && pass || fail "trap/$n: tipdogr reddetti: $tv"
  lim $PM run $w/p.pmb > $w/pm.out 2> $w/pm.err; rc=$?
  [ -f ${f%.cx}.out ] && { cmp -s $w/pm.out ${f%.cx}.out && pass || fail "trap/$n: pm: tuzaktan once yazilan cikti beklenenden farkli ($(diff $w/pm.out ${f%.cx}.out | head -3 | tr '\n' '|'))"; }
  [ $rc = 2 ] && grep -qE "^TRAP: ($exp)" $w/pm.err && pass || fail "trap/$n: pm: beklenen tuzak $exp, rc=$rc, stderr: $(head -c 100 $w/pm.err)"
  lim $BAG/gen2.elf < $w/p.l > $w/p.elf && chmod +x $w/p.elf && { lim $w/p.elf > $w/elf.out 2>/dev/null; erc=$?; [ $erc != 0 ] && cmp -s $w/elf.out $w/pm.out && pass || fail "trap/$n: ELF: tuzakla bitmeli (rc=$erc) ve ayni stdout"; } || fail "trap/$n: ELF uretilemedi"
  if [ "${SVM:-1}" = 1 ]; then { cat $TAB; echo "@@PROGRAM"; cat $w/p.l; echo "@@GIRDI"; } > $w/sv.in; LIMS=300 lim $PM run $R/oz/sayisal/svm.pmb < $w/sv.in > $w/svm.out 2>/dev/null; grep -qE "^@@TRAP ($exp)" $w/svm.out && head -c $(stat -c %s $w/pm.out) $w/svm.out | cmp -s - $w/pm.out && pass || fail "trap/$n: svm: @@TRAP ($exp) ve ayni stdout beklenir"; fi
}
export -f one red trap_n
ls testler/*.cx ../testler/*.cx | xargs -P $(nproc) -I{} bash -c 'one {}'
ls red/*.cx ../red/*.cx | xargs -P $(nproc) -I{} bash -c 'red {}'
ls trap/*.cx ../trap/*.cx | xargs -P $(nproc) -I{} bash -c 'trap_n {}'
# 2) kural degisikligi: tablo sayilari degisir, hicbir sey yeniden derlenmez
printf 'int main() { pi(7 + 3); pi(7 - 3); pi(7 * 3); return 0; }\n' > $T/k.cx
$PM ifade $T/k.cx --pmx1 $T/k.pmx > /dev/null
run_tab() { { cat $1; imzayap $1; echo; cat $T/k.pmx; } | $T/ifx.elf > $T/k.pma && $T/bag/asm.elf < $T/k.pma > $T/k.pmb && $PM run $T/k.pmb | tr '\n' ' '; }
a=$(run_tab $TAB)
python3 $D/kural_degistir.py '+' add sub < $TAB > $T/tab2.pmk && b=$(run_tab $T/tab2.pmk)
python3 $D/kural_degistir.py '*' mul add < $TAB > $T/tab3.pmk && c=$(run_tab $T/tab3.pmk)
[ "$a" = "10 4 21 " ] && pass || fail "kural: ana tablo cikti '$a'"
[ "$b" = "4 4 21 " ] && pass || fail "kural: '+' -> sub degisikligi davranisa yansimadi ('$b')"
[ "$c" = "10 4 10 " ] && pass || fail "kural: '*' -> add degisikligi davranisa yansimadi ('$c')"
# 2c) yasam dongusu parametreleri (bolum 12 sinif 6): yok etme sirasi (DT_REVERSE) ve alloca hizasi (AL_ALIGN) TABLO sayilariyla degisir; kaynak/ikili yeniden uretilmez
printf 'struct A { int id; ~A() { pi(id); } };\nint main() { A a{1}; A b{2}; A c{3}; return 0; }\n' > $T/lc.cx
printf 'int main() { long long bad = 0; for (int i = 0; i < 64; i = i + 1) { char* p = (char*)alloca(i + 1); bad = bad + (((long long)p & 31) != 0); } pi(bad); return 0; }\n' > $T/la.cx
$PM ifade $T/lc.cx --pmx1 $T/lc.pmx > /dev/null 2>&1; $PM ifade $T/la.cx --pmx1 $T/la.pmx > /dev/null 2>&1
run_tab2() { { cat $1; imzayap $1; echo; cat $2; } | $T/ifx.elf > $T/lq.pma && $T/bag/asm.elf < $T/lq.pma > $T/lq.pmb && $PM run $T/lq.pmb | tr '\n' ' '; }
python3 $D/yasam_degistir.py 16 0 < $TAB > $T/tabl1.pmk; python3 $D/yasam_degistir.py 13 4 < $TAB > $T/tabl2.pmk
[ "$(run_tab2 $TAB $T/lc.pmx)" = "3 2 1 " ] && pass || fail "yasam: ana tabloda yok etme sirasi ters degil"
[ "$(run_tab2 $T/tabl1.pmk $T/lc.pmx)" = "1 2 3 " ] && pass || fail "yasam: DT_REVERSE=0 davranisa yansimadi"
[ "$(run_tab2 $T/tabl2.pmk $T/la.pmx)" = "0 " ] && pass || fail "yasam: AL_ALIGN=4 alloca hizasina yansimadi"
[ "$(run_tab2 $TAB $T/la.pmx)" != "0 " ] && pass || fail "yasam: AL_ALIGN=2 ile 32 bayt hiza kendiliginden saglanmamali (parametre etkisiz)"
# 2b) tur notu bozulursa tipdogr reddetmeli (denetimin gercekligi)
{ cat $TAB; cat $IMZ; echo; cat $T/k.pmx; } | $T/ifx.elf > $T/k2.pma; $T/bag/asm.elf < $T/k2.pma > $T/k2.pmb && $T/bag/coz.elf < $T/k2.pmb > $T/k2.l
grep -a '^;;PMT1' $T/k2.pma | sed 's/^;;PMT1 //' > $T/k2.t
# her tur-hucresini (3,4,9,11 = i32,i64,u64,bool) tek tek f64 yap: yapisal olarak gecerli ama tur uyumsuz -> tipdogr cogunu reddetmeli
python3 - $T/k2.t $T/kbad <<'PYEOF'
import sys
v = open(sys.argv[1]).read().split()
n = 0
for i in range(5, len(v)):
    if v[i] in ('3', '4', '9', '11'):
        w = list(v); w[i] = '24'; open('%s%d.t' % (sys.argv[2], n), 'w').write(' '.join(w)); n += 1
print(n)
PYEOF
nb=$(ls $T/kbad*.t | wc -l); rej=0
for bt in $T/kbad*.t; do tb=$( { echo '@@IMZA'; cat $IMZ; echo '@@PROGRAM'; cat $T/k2.l; echo '@@TIPLER'; cat $bt; } | lim $TIPE | head -1 ); [ "${tb%% *}" = 0 ] && rej=$((rej+1)); done
[ $nb -ge 20 ] && [ $rej -ge 5 ] && pass || fail "bozuk tur notlari tipdogr tarafindan yeterince reddedilmedi ($rej/$nb)"
# 3) bozuk tablo/PMX1: temiz hata (cokme yok)
head -c 2000 $TAB > $T/kisa.pmk; cat $T/kisa.pmk $T/k.pmx | lim $T/ifx.elf > /dev/null 2>&1; [ $? -ne 139 ] && pass || fail "kisa tablo: cokme"
# 3c) atlama tablosu: yogun switch jtab uretir, seyrek switch ikili arama (karsilastirma) ; her ikisi de testlerde dogrulanir
{ cat $TAB; cat $IMZ; echo; $PM ifade $D/testler/ca_switch.cx --pmx1 /dev/stdout 2>/dev/null; } | lim $IFXE 2>/dev/null | grep -aq '^  jtab ' && pass || fail "ca_switch: yogun switch icin jtab uretilmedi"
# 3d) inline satir ici acilim (04.025): 'inline' anahtar sozcugu cagri sayisini dusurur; ayni program 'inline'siz daha cok 'call' uretir ve ciktilari ayni
sed 's/^inline //' $D/testler/ca_inline.cx > $T/ca_noinline.cx
ci1=$( { cat $TAB; cat $IMZ; echo; $PM ifade $D/testler/ca_inline.cx --pmx1 /dev/stdout 2>/dev/null; } | lim $IFXE 2>/dev/null | grep -ac '^  call ' )
ci2=$( { cat $TAB; cat $IMZ; echo; $PM ifade $T/ca_noinline.cx --pmx1 /dev/stdout 2>/dev/null; } | lim $IFXE 2>/dev/null | grep -ac '^  call ' )
[ "$ci1" -gt 0 ] && [ "$ci2" -ge $((ci1 + 15)) ] && pass || fail "ca_inline: satir ici acilim cagri sayisini dusurmedi (inline=$ci1, inline'siz=$ci2)"
# 4) OZ-BARINDIRMA (sabit nokta): ifx.cx'in PMX1'i -> ifx.elf -> PMA1 -> (asm.elf coz.elf valid.elf tipdogr gen2.elf) -> ifx2.elf ; ifx2.elf ayni girdiyi derler -> PMA2 ; PMA1 == PMA2
# (pm yok: yalniz PMX1 uretimi isteğe bagli on yuzden; derleyici kendini sayisal zincirle uretip yeniden uretebilir)
[ -n "$SABOTAJ" ] || {
  sh=$T/self; mkdir -p $sh; $PM ifade ifx.cx --pmx1 $sh/x.pmx > /dev/null 2>&1; { cat $TAB; cat $IMZ; echo; cat $sh/x.pmx; } > $sh/in
  LIMS=300 lim $IFXE < $sh/in > $sh/p1.pma 2>/dev/null && ! grep -aq '@@HATA' $sh/p1.pma && pass || fail "oz-barindirma: ifx.elf kendi kaynagini derleyemedi"
  lim $BAG/asm.elf < $sh/p1.pma > $sh/p1.pmb 2>/dev/null && $BAG/coz.elf < $sh/p1.pmb > $sh/p1.l && { $PM asm $sh/p1.pma -o $sh/p1r.pmb --strip > /dev/null 2>&1; cmp -s $sh/p1.pmb $sh/p1r.pmb; } && pass || fail "oz-barindirma: ifx2 asm.elf/coz.elf (pm asm ile bayt-bayt ayni olmali)"
  v=$(lim $BAG/valid.elf < $sh/p1.l | head -1); [ "${v%% *}" = 1 ] && pass || fail "oz-barindirma: valid.elf ifx2'yi reddetti: $v"
  grep -a '^;;PMT1' $sh/p1.pma | sed 's/^;;PMT1 //' > $sh/p1.t; tv=$( { echo '@@IMZA'; cat $IMZ; echo '@@PROGRAM'; cat $sh/p1.l; echo '@@TIPLER'; cat $sh/p1.t; } | LIMS=300 lim $TIPE | head -2 | tr '\n' ' '); [ "${tv%% *}" = 1 ] && pass || fail "oz-barindirma: tipdogr ifx2'yi reddetti: $tv"
  LIMS=300 lim $BAG/gen2.elf < $sh/p1.l > $sh/ifx2.elf && chmod +x $sh/ifx2.elf && LIMS=300 lim $sh/ifx2.elf < $sh/in > $sh/p2.pma 2>/dev/null; cmp -s $sh/p1.pma $sh/p2.pma && pass || fail "oz-barindirma: sabit nokta bozuk (ifx2 cikisi PMA1'den farkli)"
}
# 3b) bozuk PMX1 (sayisal derleyiciye): ifx.elf asla cokmez / takilmaz; kabul ettigi her cikti asm->coz->valid + tipdogr'den gecer (sessizce gecersiz program yok)
[ -n "$SABOTAJ" ] || for fz in testler/istisna_sinif ../testler/uyg_agac; do
  $PM ifade $D/$fz.cx --pmx1 $T/fz.pmx > /dev/null 2>&1; r=$(python3 $D/pmx_fuzz_ifx.py $BAG $IFXE $TIPE $TAB $IMZ $T/fz.pmx 100 $T 2>&1); [ "$r" = "0" ] && pass || fail "ifx PMX1 fuzz ($fz): $r"
done
ok=$(cat $T/gecen 2>/dev/null | wc -l); bad=$(grep -c . $T/hatalar 2>/dev/null); bad=${bad:-0}
[ -s $T/hatalar ] && sort $T/hatalar | head -30
echo "ifx: $ok gecti, $bad basarisiz"; rm -rf $T; [ "$bad" = 0 ]
