#!/bin/bash
# PMO1 OKUNABILIR GORUNUM VE DUZENLEME ARACININ (oz/arac/gorunum.pmb, oz/nesne/duzenle.sh) DOGRULAMASI.  Kullanim: dogrula_gorunum.sh [pm] [rastgele_sayisi] [bozulmus_sayisi]
#  1) gercek nesneler (asm/valid/gen2): gor = bagimsiz referans (ref/pmo_ref.py) ; yaz(gor) == depodaki .pmo BAYT-BAYT ; adlar == .adlar ; sidecar'siz de kimlik; sidecar kaynaktan taze
#  2) rastgele gorunumler: yaz / gor / adlar, PMA araci = Python referansi (bayt-bayt, hata kodu dahil) ; gor -> yaz kimlik
#  3) bozulmus gorunumler (jeton/satir silme, cogaltma, degistirme, kesme): PMA = Python (ayni cikti ya da ayni HATA kodu)
#  4) elle yazilmis olumsuz ornekler: beklenen HATA kodu (gorunum -> sayi, sayi -> gorunum, sidecar)
#  5) UCTAN UCA DUZENLEME (kopya agacta): okunabilir gorunumde algoritma degistir -> sayilar -> baglayici -> yeni arac -> davranis degisti:
#       valid: yeni kural (komut ekleme + etiket) ; gen2: uretici varsayilani (yeni makine kodu = eski -6 1 ciktisi) ; asm: yorum karakteri ; yeni tablo komutu ; geri alma (uret basarisiz)
#  6) bagimsiz ELF: gorunum.pmb gen2 ile ELF'e cevrilir; ELF = pmb ; pm'siz (env -i, ARAC) gor/yaz
# Python yoksa bagimsiz-referans karsilastirmalari ATLANIR (atlanan sayisi raporlanir), diger denetimler calisir.
PM=${1:-../../pm}; NR=${2:-120}; NM=${3:-120}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/..; T=$(mktemp -d); ok=0; bad=0; skip=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
export PM; . $R/arac/arac.sh
G=$R/arac/gorunum.pmb; REF=$D/ref/pmo_ref.py; OPT=$R/komut/komutlar.pmk
HAVEPY=0; command -v python3 > /dev/null 2>&1 && HAVEPY=1
{ echo "@@tab"; cat $OPT; } | arac komut > $T/soz.txt
# ---- araclar
pmagor()  { { echo "@@gor"; cat $T/soz.txt; [ -n "$2" ] && { echo "@@ADLAR"; cat "$2"; }; echo "@@NESNE"; cat "$1"; } | (ulimit -v 900000; timeout 60 $PM run $G 2>/dev/null); }
pmayaz()  { { echo "@@yaz"; cat ${2:-$T/soz.txt}; echo "@@GORUNUM"; cat "$1"; } | (ulimit -v 900000; timeout 60 $PM run $G 2>/dev/null); }
pmaadlar() { { echo "@@adlar"; cat "$1"; } | (ulimit -v 900000; timeout 60 $PM run $G 2>/dev/null); }
pygor()   { python3 -I $REF gor $T/soz.txt "$@" 2>/dev/null; }
pyyaz()   { python3 -I $REF yaz ${2:-$T/soz.txt} "$1" 2>/dev/null; }
pyadlar() { python3 -I $REF adlar "$1" 2>/dev/null; }
kod() { head -1 | sed -n 's/^HATA: \([0-9]*\)$/\1/p'; }   # ilk satir HATA: n ise n
# ---- 0) tazelik
$PM asm $R/arac/gorunum.pma -o $T/g.pmb --strip > /dev/null 2>&1 && cmp -s $T/g.pmb $G && pass || fail "gorunum.pmb guncel degil"
# ---- 1) gercek nesneler
srcof() { case $1 in asm) echo $R/asm/asm.pma.in;; valid) echo $R/dogr/valid.pma.in;; gen2) echo $R/elf2/gen2.pma.in;; esac; }
for n in asm valid gen2; do
  pmagor $D/$n.pmo $D/$n.adlar > $T/$n.pmv
  [ -s $T/$n.pmv ] && ! head -1 $T/$n.pmv | grep -q '^HATA' && pass || fail "$n: gor basarisiz ($(head -1 $T/$n.pmv))"
  if [ $HAVEPY = 1 ]; then pygor $D/$n.pmo $D/$n.adlar | cmp -s - $T/$n.pmv && pass || fail "$n: gor ciktisi referanstan farkli"; else skip=$((skip+1)); fi
  pmayaz $T/$n.pmv | cmp -s - $D/$n.pmo && pass || fail "$n: gor -> yaz depodaki .pmo ile bayt-bayt ayni degil"
  if [ $HAVEPY = 1 ]; then pyyaz $T/$n.pmv | cmp -s - $D/$n.pmo && pass || fail "$n: referans yaz depodaki .pmo'dan farkli"; else skip=$((skip+1)); fi
  pmaadlar $T/$n.pmv | cmp -s - $D/$n.adlar && pass || fail "$n: adlar ciktisi .adlar ile ayni degil"
  # sidecar'siz kimlik
  pmagor $D/$n.pmo > $T/$n.ns.pmv; pmayaz $T/$n.ns.pmv | cmp -s - $D/$n.pmo && pass || fail "$n: sidecar'siz gor -> yaz kimlik bozuk"
  # sidecar kaynaktan taze (islev adlari + bss bolgeleri: ice_aktar.sh ile ayni kural)
  { grep -E '^func[[:space:]]' $(srcof $n) | sed -E 's/^func[[:space:]]+([A-Za-z0-9_]+).*/func \1/'; awk '/^space[ \t]/ { printf "space %s %d\n", $2, s; s += $3 }' $(srcof $n); } | cmp -s - $D/$n.adlar && pass || fail "$n.adlar kaynaktan uretilen ile ayni degil"
  # adlarin gercekten oturdugu: giris islevi 'main'; bss adreslerinde bolge adi cozulmus
  grep -q '^entry main$' $T/$n.pmv && pass || fail "$n: giris islevi main degil (islev adlari kaymis)"
  [ "$(grep -c '&bss' $T/$n.pmv)" = 0 ] && pass || fail "$n: bolge adina cozulmeyen bss adresi var"
done
grep -qE 'ldi r[0-9]+, &G$' $T/asm.pmv && grep -qE 'ldi r[0-9]+, &G$' $T/valid.pmv && grep -qE 'ldi r[0-9]+, &G$' $T/gen2.pmv && pass || fail "G bolgesinin taban adresi (ldi rN, &G) gorunumlerde yok: bolge ofsetleri kaymis"
# ---- 2) rastgele gorunumler
if [ $HAVEPY = 1 ]; then
  for s in $(seq 1 $NR); do
    awk -v seed=$s -v dict=$T/soz.txt -v hata=$((s % 4 == 0 ? 4 : 0)) -f $D/ref/rastgele_pmv.awk > $T/r.pmv
    pyyaz $T/r.pmv > $T/r.py; pmayaz $T/r.pmv > $T/r.pm
    cmp -s $T/r.py $T/r.pm || { fail "rastgele $s: yaz farkli (py '$(head -1 $T/r.py)' pma '$(head -1 $T/r.pm)')"; continue; }
    pass
    head -1 $T/r.py | grep -q '^HATA' && continue
    pyadlar $T/r.pmv > $T/r.ad.py; pmaadlar $T/r.pmv | cmp -s - $T/r.ad.py && pass || { fail "rastgele $s: adlar farkli"; continue; }
    for v in sc ns; do
      if [ $v = sc ]; then pygor $T/r.py $T/r.ad.py > $T/r.g.py; pmagor $T/r.py $T/r.ad.py > $T/r.g.pm; else pygor $T/r.py > $T/r.g.py; pmagor $T/r.py > $T/r.g.pm; fi
      cmp -s $T/r.g.py $T/r.g.pm && pass || { fail "rastgele $s ($v): gor farkli (py '$(head -1 $T/r.g.py)' pma '$(head -1 $T/r.g.pm)')"; continue; }
      head -1 $T/r.g.pm | grep -q '^HATA' && continue
      pmayaz $T/r.g.pm | cmp -s - $T/r.py && pass || fail "rastgele $s ($v): gor -> yaz kimlik bozuk"
    done
  done
  # ---- 3) bozulmus gorunumler
  for s in $(seq 1 $NM); do
    awk -v seed=$s -v dict=$T/soz.txt -f $D/ref/rastgele_pmv.awk > $T/m.base
    awk -v seed=$((s + 5000)) -v k=$((1 + s % 3)) -f $D/ref/mutpmv.awk < $T/m.base > $T/m.pmv
    pyyaz $T/m.pmv > $T/m.py; pmayaz $T/m.pmv > $T/m.pm
    cmp -s $T/m.py $T/m.pm && pass || { fail "bozulmus $s: yaz farkli (py '$(head -1 $T/m.py)' pma '$(head -1 $T/m.pm)')"; cp $T/m.pmv /tmp/gorunum_fark_$s.pmv; }
  done
else skip=$((skip+NR+NM)); echo "uyari: python3 yok; rastgele/bozulmus karsilastirmalar atlandi"; fi
# ---- 4) elle yazilmis olumsuz ornekler
cat > $T/base.pmv <<'EOF'
pmo 1
flags 0
memory 100
entry main
key S scalar
key A array
const 1 2
data
  array A
  words 5 $S
bss 10
space SP 2
func main 0 2
  ldi r0, &A
  ldi r1, $S
  jmp L1
L1:
  halt
end
EOF
pmayaz $T/base.pmv > $T/base.pmo; head -1 $T/base.pmo | grep -q '^1347243825 1 2 1$' && pass || fail "temel gorunum derlenmedi: $(head -1 $T/base.pmo)"
pmagor $T/base.pmo | grep -q '^  ldi r1, \$S$' && pass || fail "temel nesne gor: \$S beklenir"
neg() {   # etiket kod < gorunum (stdin)
  local et=$1 ek=$2; cat > $T/n.pmv; local a; a=$(pmayaz $T/n.pmv | kod)
  if [ "$a" = "$ek" ]; then pass; else fail "olumsuz '$et': beklenen HATA: $ek, PMA '$a'"; fi
  if [ $HAVEPY = 1 ]; then local b; b=$(pyyaz $T/n.pmv | kod); [ "$b" = "$ek" ] && pass || fail "olumsuz '$et': beklenen HATA: $ek, referans '$b'"; else skip=$((skip+1)); fi
}
ed() { sed "$@" $T/base.pmv; }   # temel gorunumde duzenleme
neg "pmo satiri yok" 40 < <(ed '/^pmo 1/d')
neg "surum 2" 41 < <(ed 's/^pmo 1/pmo 2/')
neg "flags 7" 42 < <(ed 's/^flags 0/flags 7/')
neg "memory degersiz" 43 < <(ed 's/^memory 100/memory/')
neg "entry fazla jeton" 44 < <(ed 's/^entry main/entry main x/')
neg "key turu" 45 < <(ed 's/^key S scalar/key S vector/')
neg "key tekrar" 45 < <(ed 's/^key A array/key S array/')
neg "key bss" 45 < <(ed 's/^key A array/key bss array/')
neg "const dizi anahtari" 46 < <(ed 's/^const 1 2/const $A/')
neg "const eksi yer tutucu" 46 < <(ed 's/^const 1 2/const -$S/')
neg "const yer tutucu degeri" 47 < <(ed 's/^const 1 2/const 1845493760/')
neg "words dizi yer tutucusu" 47 < <(ed 's/^  words 5 \$S/  words 1879113728/')
neg "array skaler anahtar" 48 < <(ed 's/^  array A/  array S/')
neg "array tekrar" 48 < <(ed 's/^  array A/  array A\n  array A/')
neg "bss fazla jeton" 49 < <(ed 's/^bss 10/bss 10 11/')
neg "func baslik eksik" 50 < <(ed 's/^func main 0 2/func main 0/')
neg "func ad gecersiz" 50 < <(ed 's/^func main 0 2/func 1main 0 2/')
neg "end yok" 51 < <(ed '/^end$/d')
neg "end fazla jeton" 51 < <(ed 's/^end$/end x/')
neg "islevlerden sonra artik satir" 52 < <(ed '$a foo bar')
neg "islev adi tekrari" 53 < <(ed '$a func main 0 1\n halt\nend')
neg "entry bilinmeyen islev" 54 < <(ed 's/^entry main/entry yok/')
neg "&B bilinmeyen anahtar" 55 < <(ed 's/&A/\&B/')
neg "&S skaler anahtar" 55 < <(ed 's/&A/\&S/')
neg "ham bagil kayit katsayisi" 56 < <(ed 's/&A/5{A*x}/')
neg "ham bagil kayit bilinmeyen" 56 < <(ed 's/&A/5{B*1}/')
neg "ham bagil kayit bos" 56 < <(ed 's/&A/5{}/')
neg "ham bagil kayit bos ad" 56 < <(ed 's/&A/5{*1}/')
neg "ham bagil kayit tekrar" 56 < <(ed 's/&A/5{A*1|A*2}/')
neg "bos adres" 57 < <(ed 's/&A/\&/')
neg "tanimsiz imm bicimi" 57 < <(ed 's/&A/%x/')
neg "etiket tekrar" 58 < <(ed 's/^L1:/L1:\nL1:/')
neg "etiket adi gecersiz" 58 < <(ed 's/^L1:/1x:/')
neg "bilinmeyen komut" 59 < <(ed 's/^  halt/  hlt/')
neg "operand fazla" 60 < <(ed 's/^  halt/  halt r1/')
neg "yazmac bicimi" 61 < <(ed 's/ldi r0, /ldi x0, /')
neg "bilinmeyen etiket" 62 < <(ed 's/jmp L1/jmp L9/')
neg "bilinmeyen islev" 63 < <(ed 's/jmp L1/call nofunc, 1/')
neg "sabit havuzu bicimi" 64 < <(ed 's/jmp L1/ldk r0, cx/')
neg "int32 disi" 65 < <(ed 's/ldi r0, &A/ldi r0, 2147483648/')
neg "space bss disi" 66 < <(ed 's/^space SP 2/space SP 11/')
neg "space anahtar adi" 66 < <(ed 's/^space SP 2/space A 2/')
neg "space tekrar" 66 < <(ed 's/^space SP 2/space SP 1\nspace SP 2/')
neg "kanonik olmayan sayi" 30 < <(ed 's/^bss 10/bss 007/')
neg "bss 2^64 uzeri" 31 < <(ed 's/^bss 10/bss 99999999999999999999/')
neg "bss eksi" 31 < <(ed 's/^bss 10/bss -3/')
neg "flags 1 islev adi 257 harf" 67 < <({ sed 's/^flags 0/flags 1/; s/^func main 0 2/func '"$(printf 'x%.0s' $(seq 1 257))"' 0 2/; s/^entry main/entry '"$(printf 'x%.0s' $(seq 1 257))"'/' $T/base.pmv; })
neg "4097 anahtar" 5 < <({ sed -n 1,4p $T/base.pmv; for i in $(seq 1 4097); do echo "key K$i scalar"; done; sed -n 7,99p $T/base.pmv; })
neg "sayi tasmasi" 31 < <({ sed -n 1,4p $T/base.pmv; echo "key S scalar"; echo "key A array"; echo "const"; echo "data"; echo "bss 0"; echo "func main 0 1"; echo "  ldi r0, $(printf '9%.0s' $(seq 1 30))"; echo "end"; })
LONG=$(printf 'x%.0s' $(seq 1 256))
sed "s/^flags 0/flags 1/; s/^func main 0 2/func $LONG 0 2/; s/^entry main/entry $LONG/" $T/base.pmv > $T/uzun.pmv; pmayaz $T/uzun.pmv > $T/uzun.pmo
head -1 $T/uzun.pmo | grep -q '^1347243825' && pmagor $T/uzun.pmo > $T/uzun.g.pmv && pmayaz $T/uzun.g.pmv | cmp -s - $T/uzun.pmo && grep -q "^func $LONG 0 2$" $T/uzun.g.pmv && pass || fail "flags 1 ve 256 harfli islev adi gidis-donus basarisiz"
# bir sayi -> gorunum yonu: nesne bozmalari (temel nesne satirlari: 1 baslik, 2-3 anahtar, 4 bagil, 5 bayrak/bellek/giris, 6 sabit, 7 veri, 8 bss, 9 nf, 10 islev, 11.. komutlar)
nobj() {  # etiket kod awk_programi
  local et=$1 ek=$2 pr=$3; awk "$pr" $T/base.pmo > $T/n.pmo
  local a; a=$(pmagor $T/n.pmo | kod)
  if [ "$ek" = 99 ]; then [ -z "$a" ] && pass || fail "nesne '$et': kabul edilmeliydi (PMA '$a')"; [ $HAVEPY = 1 ] && { [ -z "$(pygor $T/n.pmo | kod)" ] && pass || fail "nesne '$et': referans reddetti"; } || skip=$((skip+1)); return; fi
  [ "$a" = "$ek" ] && pass || fail "olumsuz nesne '$et': beklenen HATA: $ek, PMA '$a'"
  if [ $HAVEPY = 1 ]; then local b; b=$(pygor $T/n.pmo | kod); [ "$b" = "$ek" ] && pass || fail "olumsuz nesne '$et': beklenen HATA: $ek, referans '$b'"; else skip=$((skip+1)); fi
}
nobj "sihirli sayi" 4 'NR==1{$1=1347243826} {print}'
nobj "surum" 4 'NR==1{$2=2} {print}'
nobj "anahtar sayisi" 5 'NR==1{$3=5000} {print}'
nobj "kesik" 3 'NR<=3{print} NR==4{print $1}'
nobj "skaler taban" 6 'NR==2{$2=$2+1} {print}'
nobj "dizi taban" 6 'NR==3{$2=2} {print}'
nobj "anahtar turu" 6 'NR==3{$1=2} {print}'
nobj "anahtar adi" 7 'NR==2{$4=32} {print}'
nobj "anahtar adi tekrar" 7 'NR==3{$4=83} {print}'
nobj "bagil konum disi" 8 'NR==4{$1=600} {print}'
nobj "bagil katsayi 0" 8 'NR==4{$3=0} {print}'
nobj "bagil anahtar disi" 8 'NR==4{$2=7} {print}'
nobj "bagil skaler anahtar" 9 'NR==4{$2=0} {print}'
nobj "bagil tekrar" 9 'NR==1{$4=2} NR==4{print; print; next} {print}'
nobj "bagil yanlis konum" 8 'NR==4{$1=5} {print}'
nobj "bayrak 5" 12 'NR==5{$1=5} {print}'
nobj "veri basligi tekrar" 11 'NR==7{print "4 1879113728 1879113728 5 1845493760"; next} {print}'
nobj "komut kodu" 12 'NR==11{$1=999} {print}'
nobj "operand int32 disi" 12 'NR==11{$3=99999999999} {print}'
nobj "yazmac yer tutucu" 10 'NR==11{$2=1845493760} {print}'
nobj "bagil kayit yazmac isleneninde" 8 'NR==1{$4=2} NR==4{print; print "16 1 1"; next} {print}'
nobj "bellek 2^63" 12 'NR==5{$2="9223372036854775808"} {print}'
nobj "veri sozcugu 2^63" 12 'NR==7{$3="9223372036854775808"} {print}'
nobj "negatif sabit" 12 'NR==6{$2=-5} {print}'
nobj "operand int32 sinirinda" 99 'NR==11{$3=2147483647} {print}'
nobj "operand int32 ustu" 12 'NR==11{$3=2147483648} {print}'
nobj "operand int32 altinda" 12 'NR==11{$3=-2147483649} {print}'
nobj "artik sayi" 3 '{print} END{print 5}'
nobj "bagil kayit isleneni degil" 8 'NR==4{$1=3} {print}'
nobj "ad 257 harf" 13 'NR==5{$1=1} {print} END{printf "257"; for(i=0;i<257;i++) printf " 120"; print ""}'
nobj "adlar gecersiz" 13 'NR==5{$1=1} {print} END{print "2 32 33"}'
# sidecar
printf 'func main\nfunc fazla\n' > $T/sc1.adlar; pmagor $T/base.pmo $T/sc1.adlar | kod | grep -qx 14 && pass || fail "sidecar islev sayisi uyumsuz kabul edildi"
printf 'func 1x\n' > $T/sc2.adlar; pmagor $T/base.pmo $T/sc2.adlar | kod | grep -qx 14 && pass || fail "sidecar gecersiz ad kabul edildi"
printf 'func main\nspace SP 5\nspace SQ 5\n' > $T/sc3.adlar; pmagor $T/base.pmo $T/sc3.adlar | kod | grep -qx 14 && pass || fail "sidecar bolge ofseti artmiyor ama kabul edildi"
printf 'func main\nspace SP 2\nspace SP 3\n' > $T/sc4.adlar; pmagor $T/base.pmo $T/sc4.adlar | kod | grep -qx 14 && pass || fail "sidecar bolge adi tekrari kabul edildi"
printf 'func main\nspace S 2\n' > $T/sc5.adlar; pmagor $T/base.pmo $T/sc5.adlar | kod | grep -qx 14 && pass || fail "sidecar bolge adi anahtarla cakisiyor ama kabul edildi"
printf 'func main\nspace SP 99\n' > $T/sc6.adlar; pmagor $T/base.pmo $T/sc6.adlar | kod | grep -qx 14 && pass || fail "sidecar bolge ofseti bss disi kabul edildi"
{ cat $T/base.pmv; printf 'func other 0 1\n  halt\nend\n'; } > $T/iki.pmv; pmayaz $T/iki.pmv > $T/iki.pmo
printf 'func a\nfunc a\n' > $T/sc9.adlar; pmagor $T/iki.pmo $T/sc9.adlar | kod | grep -qx 14 && pass || fail "sidecar islev adi tekrari kabul edildi"
printf 'func a\nfunc b\n' > $T/sc10.adlar; pmagor $T/iki.pmo $T/sc10.adlar | grep -q '^func b 0 1$' && pass || fail "iki islevli gecerli sidecar reddedildi"
printf 'func main\nspace SP 3\n' > $T/sc7.adlar; pmagor $T/base.pmo $T/sc7.adlar | grep -q '^func main 0 2$' && pass || fail "gecerli sidecar reddedildi"
printf 'bilinmeyen satir\n' > $T/sc8.adlar; pmagor $T/base.pmo $T/sc8.adlar | kod | grep -qx 14 && pass || fail "sidecar bilinmeyen satir kabul edildi"
# bagil kayit ifadeleri: bss bolgesi, ~, ham bicim, coklu anahtar
cat > $T/ifade.pmv <<'EOF'
pmo 1
flags 0
memory 100
entry main
key S scalar
key A array
key B array
const
data
  words 1 2
  array A
  words 7 8 9 1879113729 -1845493761 2147483647
  array B
  words 0 0
bss 50
space P 0
space Q 10
func main 0 3
  ldi r0, &A
  ldi r1, &A+2
  ldi r2, ~A
  ldi r2, ~A+2
  ldi r2, &B
  ldi r2, &B+1
  ldi r2, ~B
  ldi r2, &P
  ldi r2, &Q+5
  ldi r2, &bss+3
  ldi r2, 4{A*1|B*1}
  ldi r2, 4{A*2}
  ldi r2, 4{B*1}
  ldi r2, 3{A*1|B*1}
  addi r1, r2, -$S
  addi r1, r2, $S
  call #5, 1
  call #1, 0
  jmp @99
  jmp Lend
  jmp @-1
  ldk r0, c-2
  host 77, 2
  halt
Lend:
end
EOF
pmayaz $T/ifade.pmv > $T/ifade.pmo; head -1 $T/ifade.pmo | grep -q '^1347243825 1 3 ' && pass || fail "ifade gorunumu derlenmedi: $(head -1 $T/ifade.pmo)"
pmaadlar $T/ifade.pmv > $T/ifade.adlar; pmagor $T/ifade.pmo $T/ifade.adlar > $T/ifade.g.pmv
pmayaz $T/ifade.g.pmv | cmp -s - $T/ifade.pmo && pass || fail "ifade: gor -> yaz kimlik bozuk"
pmagor $T/ifade.pmo > $T/ifade.ns.pmv; pmayaz $T/ifade.ns.pmv | cmp -s - $T/ifade.pmo && pass || fail "ifade: sidecar'siz gor -> yaz kimlik bozuk"
for ex in 'ldi r0, 3$' 'ldi r1, 5$' 'ldi r2, ~A$' 'ldi r2, ~A\+2$' 'ldi r2, &B$' 'ldi r2, &B\+1$' 'ldi r2, ~B$' 'ldi r2, &P$' 'ldi r2, &Q\+5$' 'ldi r2, &P\+3$' 'ldi r2, 4\{A\*1\|B\*1\}$' 'ldi r2, 4\{A\*2\}$' 'ldi r2, 4\{B\*1\}$' 'ldi r2, 3\{A\*1\|B\*1\}$' 'addi r1, r2, -\$S$' 'addi r1, r2, \$S$' 'call #5, 1$' 'call #1, 0$' 'jmp L[0-9]+$' 'jmp @99$' 'jmp @-1$' 'ldk r0, c-2$' 'host 77, 2$'; do
  grep -qE "^  $ex" $T/ifade.g.pmv && pass || fail "ifade gorunumunde beklenen bicim yok: $ex"
done
grep -B1 '^end$' $T/ifade.g.pmv | grep -q '^L[0-9]*:$' && pass || fail "ifade: islev sonundaki etiket (L25:) yazilmadi"
grep -q '^  words 7 8 9 1879113729 -1845493761 2147483647$' $T/ifade.g.pmv && grep -q '^  words 0 0$' $T/ifade.g.pmv && pass || fail "ifade: yer tutucuya benzeyen ama olmayan sozcukler duz kalmali; yalniz sifir sozcuklerine yorum eklenmemeli"
grep -q '^  words 7 8 9 1879113729 -1845493761 2147483647   ;' $T/ifade.g.pmv && fail "ifade: duz sayilarda ASCII yorumu olmamali"
grep -q '^  ldi r2, &bss+15$' $T/ifade.ns.pmv && pass || fail "ifade: sidecar'siz &Q+5 -> &bss+15 beklenir"
# KISA=1: yalniz 1-4 (sabotaj_gorunum.sh once bunu calistirir)
if [ -n "$KISA" ]; then rm -rf $T; echo "PMO1 gorunum (kisa): $ok gecti, $bad basarisiz"; [ $bad = 0 ]; exit; fi
# ---- 5) UCTAN UCA DUZENLEME (kopya agacta)
mkdir -p $T/w; cp -r $R $T/w/oz; cp -r $R/../src $R/../Makefile $T/w/ 2>/dev/null; rm -f $T/w/oz/arac/gorunum.elf
WD=$T/w/oz/nesne; export OPT
duz() { (cd $WD && ./duzenle.sh "$@"); }
lim() { (ulimit -v 900000; timeout 60 "$@"); }
mkprog() { local n=$1; { echo "func main() {"; for i in $(seq 1 $n); do echo "  mov a, 1"; done; echo "  halt"; echo "}"; } > $T/p$n.pma; $PM asm $T/p$n.pma -o $T/p$n.pmb --strip > /dev/null 2>&1; $PM liste $T/p$n.pmb > $T/p$n.l; }
# 5a) valid: yeni kural -- bir islev 100'den fazla komut icermesin (red kodu 99). Komutlar + yeni etiket eklenir; atlama hedefleri sembolik oldugu icin kendiliginden yeniden hesaplanir
duz gor valid.pmo $T/v.pmv && awk '/^func validate /{inf=1} inf && /^L30:$/ && !d {print; print "  gti r2, r12, 100"; print "  jz r2, Lok"; print "  ldi r35, 99"; print "  call rej, 35"; print "Lok:"; d=1; next} /^end/{inf=0} {print}' $T/v.pmv > $T/v2.pmv
cmp -s $T/v.pmv $T/v2.pmv && fail "valid duzenlemesi uygulanamadi" || pass
duz uygula $T/v2.pmv valid > $T/uyg.log 2>&1 && pass || fail "valid: uygula basarisiz: $(tail -2 $T/uyg.log)"
for n in 50 99 100 101 150; do mkprog $n; a=$(lim $PM run $R/dogr/valid.pmb < $T/p$n.l | head -1); b=$(lim $PM run $T/w/oz/dogr/valid.pmb < $T/p$n.l | head -1)
  [ "$a" = "1 1" ] || fail "valid eski: $n komutluk programi kabul edilmeliydi ($a)"
  if [ $((n + 1)) -gt 100 ]; then [ "$b" = "0 99" ] && pass || fail "valid yeni: $((n+1)) komut -> '0 99' beklenir, '$b'"; else [ "$b" = "1 1" ] && pass || fail "valid yeni: $((n+1)) komut kabul edilmeliydi ('$b')"; fi; done
# degismeyen davranis: rastgele programlar (kucuk islevler) yeni dogrulayici = C++ referansi (pm listedogrula)
for s in $(seq 1 12); do awk -v seed=$s -f $R/elf2/rastgele2.awk > $T/rp.pma; $PM asm $T/rp.pma -o $T/rp.pmb --strip > /dev/null 2>&1 || continue; $PM liste $T/rp.pmb > $T/rp.l
  a=$(lim $PM listedogrula $T/rp.l | head -1); b=$(lim $PM run $T/w/oz/dogr/valid.pmb < $T/rp.l | head -1)
  if [ "$b" = "0 99" ]; then [ "${a:0:1}" = 1 ] && pass || fail "valid yeni: rastgele $s: 99 kodu ama referans kabul etmemis ('$a')"   # yalniz 100'den uzun islev olabilir
  else [ "${a:0:1}" = "${b:0:1}" ] && pass || fail "valid yeni: rastgele $s: referans '$a' yeni '$b'"; fi; done
# 5b) gen2: uretici varsayilani -- mikro-islem yolu (-6 1) artik varsayilan: ayni girdiden FARKLI makine kodu = eski aracin `-6 1` ciktisi; calisan program ayni
duz gor gen2.pmo $T/g.pmv && awk '/^func main 0 8$/ {print "func main 0 9"; inm=1; next} inm && /^  ldi r6, &G$/ {print; print "  ldi r8, 1"; print "  st r8, r6, 58"; inm=0; next} {print}' $T/g.pmv > $T/g2.pmv
cmp -s $T/g.pmv $T/g2.pmv && fail "gen2 duzenlemesi uygulanamadi" || pass
duz uygula $T/g2.pmv gen2 > $T/uyg.log 2>&1 && pass || fail "gen2: uygula basarisiz: $(tail -2 $T/uyg.log)"
printf 'func fib(n) {\n lti c, n, 2\n jz c, r\n ret n\nr: addi a, n, -1\n call a, fib, a\n addi b, n, -2\n call b, fib, b\n add a, a, b\n ret a\n}\nfunc main() {\n mov x, 20\n call y, fib, x\n host puti, y\n host putc, 10\n halt\n}\nentry main\n' > $T/fib.pma
$PM asm $T/fib.pma -o $T/fib.pmb --strip > /dev/null 2>&1; $PM liste $T/fib.pmb > $T/fib.l
lim $PM run $R/elf2/gen2.pmb < $T/fib.l > $T/fib_eski.elf; { echo "-6 1"; cat $T/fib.l; } | lim $PM run $R/elf2/gen2.pmb > $T/fib_eski6.elf; lim $PM run $T/w/oz/elf2/gen2.pmb < $T/fib.l > $T/fib_yeni.elf
chmod +x $T/fib_*.elf
cmp -s $T/fib_eski.elf $T/fib_yeni.elf && fail "gen2 yeni ve eski varsayilan ayni makine kodunu uretti (duzenleme etkisiz)" || pass
cmp -s $T/fib_eski6.elf $T/fib_yeni.elf && pass || fail "gen2 yeni varsayilan eski '-6 1' ciktisindan farkli"
[ "$(lim $T/fib_yeni.elf)" = "6765" ] && [ "$(lim $T/fib_eski.elf)" = "6765" ] && pass || fail "fib ELF'leri 6765 vermedi"
# yeni uretici kendi baska bir uygulamasini (asm.pmb -> ELF) uretir ve o ELF dogru calisir (yeni makine kodu)
$PM liste $R/asm/asm.pmb > $T/asm.l; lim $PM run $T/w/oz/elf2/gen2.pmb < $T/asm.l > $T/asm_yeni.elf; chmod +x $T/asm_yeni.elf
lim $T/asm_yeni.elf < $T/fib.pma | cmp -s - $T/fib.pmb && pass || fail "yeni ureticinin urettigi asm ELF'i fib.pma'yi dogru derlemedi"
# 5c) asm: aciklama karakteri ';' yerine '#' (sozcuk analizinde karsilastirilan karakter kodu 59 -> 35)
duz gor asm.pmo $T/a.pmv && grep -c '^  eqi r4, r5, 59$' $T/a.pmv | grep -qx 1 && pass || fail "asm gorunumunde ';' karsilastirmasi (59) tek degil"
sed 's/^  eqi r4, r5, 59$/  eqi r4, r5, 35/' $T/a.pmv > $T/a2.pmv; duz uygula $T/a2.pmv asm > $T/uyg.log 2>&1 && pass || fail "asm: uygula basarisiz: $(tail -2 $T/uyg.log)"
printf 'func main() {\n mov a, 7 # yorum\n host puti, a\n halt\n}\n' > $T/c1.pma; printf 'func main() {\n mov a, 7 ; yorum\n host puti, a\n halt\n}\n' > $T/c2.pma
lim $PM run $T/w/oz/asm/asm.pmb < $T/c1.pma > $T/c1.pmb 2>/dev/null; rc1=$?; lim $PM run $T/w/oz/asm/asm.pmb < $T/c2.pma > $T/c2.pmb 2>/dev/null; rc2=$?
lim $PM run $R/asm/asm.pmb < $T/c2.pma > $T/c2e.pmb 2>/dev/null; lim $PM run $R/asm/asm.pmb < $T/c1.pma > /dev/null 2>&1; rc1e=$?
[ $rc1 = 0 ] && [ "$(lim $PM run $T/c1.pmb)" = "7" ] && pass || fail "asm yeni: '#' aciklamali program derlenmedi/calismadi (rc=$rc1)"
[ $rc1e != 0 ] && pass || fail "asm eski: '#' aciklamali program kabul edildi (eski davranis degismis olmamali)"
[ $rc2 != 0 ] && pass || fail "asm yeni: ';' artik aciklama olmamali ama kabul edildi"
# 5d) yeni tablo komutu: tabloya komut eklenir, gorunumde yeni komut yazilir
mod() { { echo "@@$1"; cat; } | arac komut; }
mod gor < $OPT | awk '{print} /^op 54 GEI/{print "op 55 XORI xori d,r,i"} /^sem GEI /{print "sem XORI rd t1 b ; im t2 c ; alu_xor t3 t1 t2 ; wr a t3"}' | mod yaz > $T/xo.pmk
[ "$(mod dogrula < $T/xo.pmk)" = "1" ] && pass || fail "genisletilmis tablo gecersiz"
{ echo "@@tab"; cat $T/xo.pmk; } | arac komut > $T/soz_xo.txt
sed 's/^  halt$/  xori r0, r1, 5\n  halt/' $T/base.pmv > $T/xo.pmv
[ "$(pmayaz $T/xo.pmv | kod)" = "59" ] && pass || fail "eski tabloda xori reddedilmedi"
pmayaz $T/xo.pmv $T/soz_xo.txt > $T/xo.pmo; head -1 $T/xo.pmo | grep -q '^1347243825' && pass || fail "yeni tabloda xori kabul edilmedi"
{ echo "@@gor"; cat $T/soz_xo.txt; echo "@@NESNE"; cat $T/xo.pmo; } | (ulimit -v 900000; timeout 60 $PM run $G 2>/dev/null) | grep -q '^  xori r0, r1, 5$' && pass || fail "yeni tabloda gor xori'yi yazmadi"
if [ $HAVEPY = 1 ]; then pyyaz $T/xo.pmv $T/soz_xo.txt | cmp -s - $T/xo.pmo && pass || fail "yeni tablo: referans yaz farkli"; else skip=$((skip+1)); fi
# 5e) geri alma: uret basarisiz olursa eski nesne, sidecar ve .pmb geri konur
cp $WD/valid.pmo $T/once.pmo; cp $WD/valid.adlar $T/once.adlar; cp $T/w/oz/dogr/valid.pmb $T/once.pmb
duz gor valid.pmo $T/v3.pmv; mv $T/w/oz/dogr/uret.sh $T/w/oz/dogr/uret.sh.x; printf '#!/bin/bash\nexit 1\n' > $T/w/oz/dogr/uret.sh; chmod +x $T/w/oz/dogr/uret.sh
sed 's/^  halt$/  halt\n  halt/' $T/v3.pmv > $T/v4.pmv
duz uygula $T/v4.pmv valid > $T/uyg.log 2>&1 && fail "bozuk uret.sh ile uygula basarili sayildi" || pass
cmp -s $WD/valid.pmo $T/once.pmo && cmp -s $WD/valid.adlar $T/once.adlar && cmp -s $T/w/oz/dogr/valid.pmb $T/once.pmb && pass || fail "geri alma: eski nesne/sidecar/pmb geri konmadi"
mv $T/w/oz/dogr/uret.sh.x $T/w/oz/dogr/uret.sh
# 5f) duzenle.sh: gor/yaz kimlik (depo nesneleri)
for n in asm valid gen2; do duz gor $D/$n.pmo $T/d_$n.pmv 2>/dev/null; duz yaz $T/d_$n.pmv $T/d_$n.pmo 2>/dev/null; cmp -s $T/d_$n.pmo $D/$n.pmo && cmp -s $T/d_$n.adlar $D/$n.adlar && pass || fail "duzenle.sh $n: gor -> yaz kimlik/adlar bozuk"; done
# ---- 6) bagimsiz ELF: gorunum.pmb -> gen2 -> ELF ; ELF = pmb ; pm'siz (env -i, yalniz ELF araclari)
$PM liste $G > $T/g.l; lim $PM run $R/elf2/gen2.pmb < $T/g.l > $T/gorunum.elf; chmod +x $T/gorunum.elf
for n in asm valid gen2; do
  { echo "@@gor"; cat $T/soz.txt; echo "@@ADLAR"; cat $D/$n.adlar; echo "@@NESNE"; cat $D/$n.pmo; } > $T/e_in.txt
  lim $T/gorunum.elf < $T/e_in.txt | cmp -s - $T/$n.pmv && pass || fail "gorunum.elf: $n gor ciktisi pmb ile farkli"
  { echo "@@yaz"; cat $T/soz.txt; echo "@@GORUNUM"; cat $T/$n.pmv; } | lim $T/gorunum.elf | cmp -s - $D/$n.pmo && pass || fail "gorunum.elf: $n yaz ciktisi .pmo ile farkli"
done
# araclari ELF olarak kur (komut + gorunum + gen2) ve pm OLMADAN calistir
mkdir -p $T/E $T/noo; cp -r $R $T/noo/oz; cp $T/gorunum.elf $T/E/gorunum.elf; $PM liste $R/arac/komut.pmb > $T/k.l; lim $PM run $R/elf2/gen2.pmb < $T/k.l > $T/E/komut.elf; chmod +x $T/E/*.elf
(cd $T/noo/oz/nesne && env -i PATH="$(dirname $(command -v bash)):/usr/bin:/bin" ARAC=$T/E PM=/yok/pm ./duzenle.sh gor asm.pmo $T/np.pmv > $T/np.log 2>&1 && env -i PATH="$(dirname $(command -v bash)):/usr/bin:/bin" ARAC=$T/E PM=/yok/pm ./duzenle.sh yaz $T/np.pmv $T/np.pmo >> $T/np.log 2>&1)
cmp -s $T/np.pmv $T/asm.pmv && cmp -s $T/np.pmo $D/asm.pmo && pass || fail "pm'siz (ARAC) duzenle.sh gor/yaz sonucu pm'li ile ayni degil: $(head -c 200 $T/np.log)"
rm -rf $T
echo "PMO1 gorunum ve duzenleme: $ok gecti, $bad basarisiz (atlanan referans karsilastirmasi: $skip)"
[ $bad = 0 ]
