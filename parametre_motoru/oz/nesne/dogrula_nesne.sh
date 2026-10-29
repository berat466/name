#!/bin/bash
# SAYISAL NESNE (PMO1) + BAGLAYICI denetimi.  Kanonik derleme girdisi: oz/nesne/*.pmo (sayilar); metin (.pma.in) istege bagli on yuzdur.
#  1) UYUMLULUK KAPISI: metin kaynaklari ice aktarilinca depodaki .pmo ile AYNI (metin ile sayilar ayrismamali); rt.pma = rt.pmb
#  2) nesne + mevcut tablo -> baglayici -> PMB1 = depodaki asm/valid/gen2.pmb (bayt-bayt); metin kullanilmaz
#  3) DEGISTIRILMIS TABLOLAR (ad degisikligi, yeni komut + anlam + kural, yeni takma komut): baglayici cikti = metin yolu (doldur + assembler) cikti, bayt-bayt (asm, valid, gen2)
#  4) olumsuz ornekler: baglayici bozuk girdiyi reddeder
#  5) ice aktarma belirlenimci
# Kullanim: dogrula_nesne.sh [pm]
PM=${1:-../../pm}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
export PM; . $R/arac/arac.sh
mod() { { echo "@@$1"; cat; } | arac komut; }
TAB=$R/komut/komutlar.pmk
# ---- 0) arac guncel
$PM asm $R/arac/baglayici.pma -o $T/b.pmb --strip > /dev/null 2>&1 && cmp -s $T/b.pmb $R/arac/baglayici.pmb && pass || fail "baglayici.pmb guncel degil"
# ---- 1) kapi
for t in asm:asm/asm.pma.in valid:dogr/valid.pma.in gen2:elf2/gen2.pma.in; do n=${t%%:*}; ./ice_aktar.sh $R/${t#*:} $T/$n.pmo > /dev/null && cmp -s $T/$n.pmo $n.pmo && pass || fail "$n.pmo metin kaynagindan uretilenle ayni degil (nesne/metinden.sh)"; done
$PM asm $R/elf2/rt.pma -o $T/rt.pmb --strip > /dev/null 2>&1 && cmp -s $T/rt.pmb $R/elf2/rt.pmb && pass || fail "rt.pmb rt.pma ile ayni degil"
# ---- 2) tablodan metinsiz uretim = depodaki
mkdir -p $T/m; cp -r $R/nesne $R/komut $R/arac $T/m/; for d in asm dogr elf2; do mkdir -p $T/m/$d; done; cp $R/elf2/rt.pmb $R/elf2/statik.sozluk $T/m/elf2/
for t in asm:asm:asm.pmb valid:dogr:valid.pmb gen2:elf2:gen2.pmb; do n=${t%%:*}; r=${t#*:}; dir=${r%%:*}; f=${r#*:}
  cp $R/$dir/uret.sh $T/m/$dir/; (cd $T/m/$dir && ./uret.sh $PM > /dev/null 2>&1) && cmp -s $T/m/$dir/$f $R/$dir/$f && pass || fail "$f nesneden uretilen depodakiyle ayni degil"; done
# ---- 3) degistirilmis tablolar: baglayici = metin yolu
dolasma() {   # etiket tablo : asm/valid/gen2 icin baglayici ciktisi ile doldur+assembler ciktisi ayni
  local et=$1 tab=$2
  mod tab < $tab > $T/d.txt || { fail "$et: sozluk uretilemedi"; return; }
  RTLL=$($PM liste $R/elf2/rt.pmb); set -- $RTLL; RTLN=$#
  for t in asm:asm/asm.pma.in:asm.pmo:"" valid:dogr/valid.pma.in:valid.pmo:"" gen2:elf2/gen2.pma.in:gen2.pmo:ek; do
    IFS=: read -r n src pmo ek <<< "$t"
    { cat $T/d.txt; [ -n "$ek" ] && { cat $R/elf2/statik.sozluk; echo RTL $RTLL; echo "RTLN $RTLN"; }; echo "@@SABLON"; cat $R/$src; } | arac doldur > $T/x.pma 2>/dev/null && $PM asm $T/x.pma -o $T/x.pmb --strip > /dev/null 2>&1 || { fail "$et/$n: metin yolu derlenemedi"; continue; }
    { cat $T/d.txt; [ -n "$ek" ] && { cat $R/elf2/statik.sozluk; echo RTL $RTLL; echo "RTLN $RTLN"; }; echo "@@NESNE"; cat $D/$pmo; } | arac baglayici | arac_pmb $T/y.pmb 2>/dev/null
    cmp -s $T/x.pmb $T/y.pmb && pass || fail "$et/$n: baglayici cikti metin yolundan farkli"
  done
}
dolasma "mevcut tablo" $TAB
mod gor < $TAB | sed 's/^op 3 ADD add /op 3 ADD toplam /' | mod yaz > $T/t1.pmk; dolasma "ad degisikligi (add->toplam)" $T/t1.pmk
mod gor < $TAB | awk '/^section 7 /{$5=$5+1; print $0 " 18 0 10 2 1 -2147483648 2147483647 1 1 55 2 -2147483648 2147483647 2 2 0 1 55 0"; next} {print} /^op 54 GEI/{print "op 55 XORI xori d,r,i"; print "op 56 GOTO goto l"; print "op 57 FMIN fmin d,f,f"} /^sem GEI /{print "sem XORI rd t1 b ; im t2 c ; alu_xor t3 t1 t2 ; wr a t3"; print "sem GOTO br a"; print "sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4"}' | mod yaz > $T/t2.pmk; dolasma "yeni komutlar + kural" $T/t2.pmk
mod gor < $TAB | awk '/^section 6 /{ $5=$5+1; print $0 " 9 4 2 90 87 2 117 108 3 0"; next} {print}' | mod yaz > $T/t3.pmk; [ "$(mod dogrula < $T/t3.pmk)" = "1" ] && dolasma "yeni takma komut (ul = lt)" $T/t3.pmk || pass
# ---- 3b) kucuk sablon: iki dizi + veri/bss adresleri (dizi uzunluklarina bagli yerlesim); skaler yer tutucunun yerlesimi (space boyutu) etkilemesi ice aktarmada reddedilir
cat > $T/ornek.pma.in <<'EOT'
memory 4096
entry main
space A 16
data D = @ARR@
data E = 9 8 7
data F = @BRR@
space B 16
func main() {
    mov p, A
    mov q, B
    add r, p, q
    host puti, r
    host putc, 32
    mov e, E
    host puti, e
    mov f, F
    host puti, f
    halt
}
EOT
./ice_aktar.sh $T/ornek.pma.in $T/ornek.pmo > /dev/null || fail "ornek sablon ice aktarilamadi"
for arr in "5" "5 6 7 8" "1 2 3 4 5 6 7 8 9 10 11"; do for brr in "1" "2 3" "4 5 6 7 8 9 10 11 12"; do
  { echo "ARR $arr"; echo "BRR $brr"; echo "@@SABLON"; cat $T/ornek.pma.in; } | arac doldur > $T/o.pma && $PM asm $T/o.pma -o $T/o_ref.pmb --strip > /dev/null 2>&1
  { echo "ARR $arr"; echo "BRR $brr"; echo "@@NESNE"; cat $T/ornek.pmo; } | arac baglayici | arac_pmb $T/o_bag.pmb 2>/dev/null
  cmp -s $T/o_ref.pmb $T/o_bag.pmb && pass || fail "ornek (ARR=$arr, BRR=$brr): baglayici cikti metin yolundan farkli"
done; done
printf 'memory 4096\nentry main\nspace A @N@\nfunc main() {\n halt\n}\n' > $T/skaler_space.pma.in
./ice_aktar.sh $T/skaler_space.pma.in $T/x.pmo > /dev/null 2>&1 && fail "skaler yer tutucu space boyutu olarak ice aktarildi (reddedilmeliydi)" || pass
# ---- 4) olumsuz ornekler
{ cat $T/d.txt; } > /dev/null
mod tab < $TAB > $T/d.txt
neg() {   # etiket : stdin girdi -> baglayici reddetmeli (cikis kodu != 0)
  local et=$1 kod=$2; ( ulimit -v 900000; timeout 30 $PM run $R/arac/baglayici.pmb > $T/neg.out 2>/dev/null ); local rc=$?
  [ $rc != 0 ] && [ "$(head -1 $T/neg.out)" = "HATA: $kod" ] && pass || fail "baglayici beklenen hatayla reddetmedi: $et (beklenen HATA: $kod, bulunan '$(head -1 $T/neg.out)' rc=$rc)"
}
neg "yanlis sihirli sayi" 7 < <( { cat $T/d.txt; echo "@@NESNE"; sed '1s/1347243825/1347243826/' $D/asm.pmo; } )
neg "sozlukte eksik anahtar" 11 < <( { grep -v '^OPCOUNT' $T/d.txt; echo "@@NESNE"; cat $D/asm.pmo; } )
neg "kesik nesne" 3 < <( { cat $T/d.txt; echo "@@NESNE"; head -c 1000 $D/asm.pmo; } )
neg "bagil kayit sayisi uyumsuz" 13 < <( { cat $T/d.txt; echo "@@NESNE"; awk 'NR==1{$4=$4+1} {print}' $D/asm.pmo; } )
neg "skaler anahtarin birden cok degeri var" 12 < <( { sed 's/^OPCOUNT .*/OPCOUNT 1 2/' $T/d.txt; echo "@@NESNE"; cat $D/asm.pmo; } )
neg "@@NESNE satiri yok" 6 < <( { cat $T/d.txt; cat $D/asm.pmo; } )
# ---- 5) belirlenimcilik
./ice_aktar.sh $R/dogr/valid.pma.in $T/v1.pmo > /dev/null; ./ice_aktar.sh $R/dogr/valid.pma.in $T/v2.pmo > /dev/null; cmp -s $T/v1.pmo $T/v2.pmo && pass || fail "ice_aktar belirlenimci degil"
echo "sayisal nesne ve baglayici: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
