#!/bin/bash
# Kanonik sayisal komut tablosu (PMK1) denetimi. Kullanim: dogrula_komut.sh [pm] [rastgele_tablo_sayisi]   (ARAC=ELF dizini ile araclar ELF olarak calisir)
#  1) tablo gecerli + kanonik yerlesimde; src/ops_gen.h tablodan uretilmis ve guncel
#  2) GECIS ESDEGERLIGI (donmus ops.h + bagimsiz awk): awk gecisi = tablo; sozluk = eski opstab; ops_gen.h = eski X-makro bilgileri
#  3) tablodan yeniden uretim: asm/valid/gen2 PMB1'leri (ops.h OKUNMADAN) depodakilerle BAYT-BAYT ayni
#  4) KAYIPSIZ gidis-donus: sayilar -> okunabilir metin -> sayilar (gercek tablo + rastgele gecerli tablolar); metin duzenlemesi sayisal tabloya islenir
#  5) olumsuz ornekler (bicim/profil) ve uyumluluk: bilinmeyen istege bagli bolum/ext/kuyruk korunur, bilinmeyen zorunlu bolum reddedilir, kucuk surum kabul, buyuk surum ret
#  6) DAVRANIS YANSIMASI: tabloda mnemonik adi degisince (add -> topla) uretilen assembler yeni adi kabul eder, eskisini reddeder; ayni PMB1 ve ELF davranisi
#  7) GENISLETME: tabloya yeni komut (FMIN) + tek sablon satiri eklenir -> assembler, dogrulayici, ELF uretici ve (yeniden uretilen C++ basligiyla) liste araci yeni komutu isler
PM=${1:-../../pm}; NR=${2:-100}; D=$(cd "$(dirname "$0")" && pwd); cd "$D"; PM=$(realpath "$PM"); R=$D/../..; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
. ../arac/arac.sh
K() { arac komut; }                     # stdin -> stdout
mod() { { echo "@@$1"; cat; } | K; }    # mod <ad> : stdin sayisal/metin tablo -> cikti
TAB=$D/komutlar.pmk
# ---- 0) arac guncel
$PM asm ../arac/komut.pma -o $T/k.pmb --strip > /dev/null 2>&1 && cmp -s $T/k.pmb ../arac/komut.pmb && pass || fail "komut.pmb guncel degil (oz/arac/uret.sh)"
# ---- 1) tablo + uretilmis ust bilgiler
[ "$(mod dogrula < $TAB)" = "1" ] && pass || fail "komutlar.pmk gecerli degil: $(mod dogrula < $TAB)"
mod kanon < $TAB | cmp -s - $TAB && pass || fail "komutlar.pmk kanonik yerlesimde degil (komut @@kanon)"
mod opsh < $TAB | cmp -s - $R/src/ops_gen.h && pass || fail "src/ops_gen.h tablodan uretilmis ve guncel degil (komut @@opsh)"
grep -qE '^ *X\([A-Z0-9_]+,' $R/src/ops.h && fail "src/ops.h hala komut tanimi iceriyor" || pass
# ---- 2) gecis esdegerligi (donmus ops.h.txt + bagimsiz awk)
awk -f gecis/ops_to_pmk.awk gecis/ops.h.txt | mod kanon > $T/gecis.pmk
cmp -s $T/gecis.pmk gecis/komutlar_v1.pmk && pass || fail "awk gecisi (ops.h.txt) komutlar_v1.pmk ile ayni degil"
mod tab < gecis/komutlar_v1.pmk | grep -E '^(OPCOUNT|OPNAMES|NOPS|KIND) ' > $T/t4.new
arac opstab < gecis/ops.h.txt > $T/t4.old
[ "$(tr -s ' \n' ' ' < $T/t4.new)" = "$(tr -s ' \n' ' ' < $T/t4.old)" ] && pass || fail "sozluk (OPCOUNT/OPNAMES/NOPS/KIND) eski opstab ciktisindan farkli"
# eski ops.h'in X satirlari = tablodan uretilen X satirlari (bosluk duzeninden bagimsiz)
norm() { grep -E '^ *X\(' | sed -E 's/ *\\ *$//; s/ +//g; s/^X\(([A-Z0-9_]+),"([a-z0-9_]+)",([0-9]),"([a-zA-Z]*)"\)/\1 \2 \3 \4/' | grep -E '^[A-Z0-9_]+ [a-z0-9_]+ [0-9] [a-zA-Z]*$' ; }
mod opsh < gecis/komutlar_v1.pmk | norm > $T/x.new; norm < gecis/ops.h.txt > $T/x.old
[ -s $T/x.old ] && cmp -s $T/x.new $T/x.old && pass || fail "ops_gen.h komut satirlari eski ops.h ile farkli"
cmp -s $TAB gecis/komutlar_v1.pmk && echo "bilgi: komutlar.pmk == komutlar_v1.pmk (tablo genisletilmedi)" || echo "bilgi: komutlar.pmk komutlar_v1.pmk'den farkli (kasitli genisletme)"
# ---- 3) tablodan yeniden uretim (ops.h OKUNMADAN): bayt-bayt ayni
rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $T/w/oz/; cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $T/w/oz/
rm -f $T/w/oz/asm/asm.pmb $T/w/oz/dogr/valid.pmb $T/w/oz/elf2/gen2.pmb
(cd $T/w/oz/arac && ./uret.sh $PM > /dev/null 2>&1) && for t in asm dogr elf2; do (cd $T/w/oz/$t && ./uret.sh $PM > /dev/null 2>&1) || fail "$t: tablodan uretilemedi"; done
cmp -s $T/w/oz/asm/asm.pmb $R/oz/asm/asm.pmb && pass || fail "tablodan uretilen asm.pmb depodakiyle farkli"
cmp -s $T/w/oz/dogr/valid.pmb $R/oz/dogr/valid.pmb && pass || fail "tablodan uretilen valid.pmb depodakiyle farkli"
cmp -s $T/w/oz/elf2/gen2.pmb $R/oz/elf2/gen2.pmb && pass || fail "tablodan uretilen gen2.pmb depodakiyle farkli"
cmp -s $T/w/oz/arac/sablonc.pmb $R/oz/arac/sablonc.pmb && pass || fail "sablonc.pmb farkli"
# PMB1 cozucu/kodlayici tablolari (NOPS dizisi + komut siniri) komut tablosundan uretilir
(cd $T/w/oz && ./uret_ikili.sh $PM $T/w/oz > /dev/null 2>&1) || fail "coz/yaz tablosu uretilemedi"
cmp -s $T/w/oz/coz.pmy $R/oz/coz.pmy && pass || fail "tablodan uretilen coz.pmy depodakiyle farkli"
cmp -s $T/w/oz/yaz.pmy $R/oz/yaz.pmy && pass || fail "tablodan uretilen yaz.pmy depodakiyle farkli"
$PM yapisal $R/oz/paketle.pmy -o $T/pk.pmb --strip > /dev/null 2>&1; cmp -s $T/pk.pmb ../arac/paketle.pmb && pass || fail "paketle.pmb guncel degil"
# ---- 4) kayipsiz gidis-donus
rt() {   # etiket sayisal_tablo_dosyasi
  mod kanon < "$2" > $T/r.k 2> /dev/null || { fail "$1: kanon reddetti ($(head -c 30 $T/r.k))"; return; }
  grep -q '^HATA' $T/r.k && { fail "$1: kanon: $(head -1 $T/r.k)"; return; }
  mod gor < $T/r.k > $T/r.g; mod yaz < $T/r.g > $T/r.y
  cmp -s $T/r.k $T/r.y && { mod gor < $T/r.y | cmp -s - $T/r.g && pass || fail "$1: metin sabit nokta degil"; } || fail "$1: sayilar -> metin -> sayilar kayipli"
}
rt "gercek tablo" $TAB; rt "v1 tablosu" gecis/komutlar_v1.pmk
for s in $(seq 1 $NR); do awk -v seed=$s -v dense=0 -f rastgele_pmk.awk > $T/rr.txt; rt "rastgele-seyrek-$s" $T/rr.txt; awk -v seed=$s -v dense=1 -f rastgele_pmk.awk > $T/rr.txt; rt "rastgele-yogun-$s" $T/rr.txt; done
mod gor < $TAB > $T/g0.txt; grep -q '^op 3 ADD add d,r,r$' $T/g0.txt && grep -q '^host 4 present 3 ext 1=3$' $T/g0.txt && grep -q '^kind 1 d 2$' $T/g0.txt && pass || fail "okunabilir gorunum beklenen satirlari icermiyor"
# metin duzenlemesi sayisal tabloya islenir (gercek tabloda ad degistir, ext ekle)
mod gor < $TAB | sed 's/^op 3 ADD add /op 3 ADD toplam /' | mod yaz > $T/ed.pmk
[ "$(mod dogrula < $T/ed.pmk)" = "1" ] && mod tab < $T/ed.pmk | grep -q '^OPNAMES' && pass || fail "duzenlenmis metin gecerli sayisal tabloya donmedi"
[ "$(mod tab < $T/ed.pmk | sed -n 2p | tr -s ' ' | cut -d' ' -f1-25)" != "$(mod tab < $TAB | sed -n 2p | tr -s ' ' | cut -d' ' -f1-25)" ] && pass || fail "ad degisikligi sozluge yansimadi"
# ---- 5) olumsuz ornekler ve uyumluluk (kanonik yerlesim: satir 1 baslik, 2-3 KIND, 4-13 kindler, 14-15 OP, 16-70 komutlar, 71-72 HOST, 73-79 hostlar)
sat() { awk "$1" $TAB; }   # awk programi canonik tablo uzerinde
red() { out=$(mod dogrula < "$2"); [ "${out:0:1}" = "0" ] && pass || fail "$1: gecersiz tablo kabul edildi ($out)"; }
kab() { out=$(mod dogrula < "$2"); [ "$out" = "1" ] && pass || fail "$1: gecerli tablo reddedildi ($out)"; }
sat 'NR==1{$1=1347242800} {print}' > $T/n1; red "magic yanlis" $T/n1
sat 'NR==1{$2=2} {print}' > $T/n1; red "buyuk surum 2" $T/n1
sat 'NR==1{$3=7} {print}' > $T/n1; kab "kucuk surum 7 kabul" $T/n1
sat 'NR==1{$4=$4+1} {print}' > $T/n1; red "bolum sayisi fazla" $T/n1
sat 'NR<70' > $T/n1; red "kesik tablo" $T/n1
sat 'NR==16{$2=1} {print}' > $T/n1; red "opcode yinelenen (0 yerine 1)" $T/n1
sat 'NR==17{$2=0} {print}' > $T/n1; red "opcode yinelenen (1 yerine 0)" $T/n1
sat 'NR==18{$3=2; $4=76; $5=68; $6=75; $7=3} {print}' > $T/n1; red "bozuk kayit" $T/n1
sat 'NR==19{$1=$1+1} {print}' > $T/n1; red "kayit uzunlugu uyumsuz" $T/n1
sat 'NR==4{$2=2} {print}' > $T/n1; red "kind kimligi yinelenen" $T/n1
sat 'NR==4{$3=114} {print}' > $T/n1; red "kind karakteri yinelenen" $T/n1
sat 'NR==4{$3=35} {print}' > $T/n1; red "kind karakteri '#' (metin gorunumunu bozar)" $T/n1
sat 'NR==19{$4=97} {print}' > $T/n1; red "sembol kucuk harfle basliyor" $T/n1
sat 'NR==19{$8=65} {print}' > $T/n1; red "komut adinda buyuk harf" $T/n1
sat 'NR==19{$11=11} {print}' > $T/n1; red "bilinmeyen tur kimligi" $T/n1
sat 'NR==73{$3=9} {print}' > $T/n1; red "host arguman sayisi 9" $T/n1
sat 'NR==74{$2=0} {print}' > $T/n1; red "host kimligi yinelenen" $T/n1
sat 'NR==2{$2=1} NR==71{$2=1} {print}' > $T/n1; kab "bolum bayragi zorunlu-bilinen kabul" $T/n1
sat 'NR==2{$3=2; $2=1} {print}' > $T/n1; red "bilinen bolum surum 2 + zorunlu" $T/n1
sat 'NR==71{$2=0; $3=2} {print}' > $T/n1; mod kanon < $T/n1 | head -1 | grep -q '^1347242801' && mod gor < $T/n1 | grep -q '^section 3 0 2' && pass || fail "bilinen bolum surum 2 istege bagli: opak korunmadi"
# yeni bilinmeyen bolum eklemek: baslikta bolum sayisi +1, sona bolum
sat 'NR==1{$4=$4+1} {print} END{print "77 0 1 3 10 20 30"}' > $T/opt; kab "bilinmeyen istege bagli bolum" $T/opt
sat 'NR==1{$4=$4+1} {print} END{print "77 1 1 3 10 20 30"}' > $T/req; red "bilinmeyen ZORUNLU bolum" $T/req
mod tab < $T/opt | cmp -s - <(mod tab < $TAB) && pass || fail "bilinmeyen bolum sozluk ciktisini etkiledi"
mod gor < $T/opt | mod yaz | cmp -s - <(mod kanon < $T/opt) && pass || fail "bilinmeyen bolum gidis-donusta kayboldu"
# kayit uzantilari: bir komuta ext (anahtar 9=99) + kuyruk (5 6 7) ekle: reclen +4, ext sayisi 0->1
sat 'NR==14{$4=$4+5} NR==16{n=NF; $1=$1+5; $n=1; $(n+1)=9; $(n+2)=99; $(n+3)=5; $(n+4)=6; $(n+5)=7} {print}' > $T/ext1
kab "kayit uzantisi + kuyruk" $T/ext1
mod tab < $T/ext1 | cmp -s - <(mod tab < $TAB) && pass || fail "kayit uzantisi sozluk ciktisini etkiledi"
mod gor < $T/ext1 | mod yaz | cmp -s - <(mod kanon < $T/ext1) && pass || fail "kayit uzantisi/kuyruk gidis-donusta kayboldu"
# profil: seyrek kimlik (komut 54 -> 60) format olarak gecerli, profil icin gecersiz
mod gor < $TAB | sed 's/^op 54 GEI /op 60 GEI /' | mod yaz > $T/sp; mod kanon < $T/sp | head -1 | grep -q '^1347242801' && pass || fail "seyrek kimlik bicim olarak reddedildi"
out=$(mod dogrula < $T/sp); [ "${out:0:1}" = "0" ] && pass || fail "seyrek kimlik profil denetimini gecti ($out)"
mod tab < $T/sp 2>&1 | head -1 | grep -q '^HATA' && pass || fail "seyrek kimlikli tablo sozluk uretti"
# bolum sirasi degisimi: HOST, KIND, OP sirasi -> ayni sozluk
awk 'NR==1{print;next} NR>=2&&NR<=13{k[++a]=$0;next} NR>=14&&NR<=70{o[++b]=$0;next} {h[++c]=$0} END{for(i=1;i<=c;i++)print h[i]; for(i=1;i<=a;i++)print k[i]; for(i=1;i<=b;i++)print o[i]}' $TAB > $T/perm
kab "bolum sirasi degisti" $T/perm
mod tab < $T/perm | cmp -s - <(mod tab < $TAB) && pass || fail "bolum sirasi sozluk ciktisini etkiledi"
mod gor < $T/perm | mod yaz | cmp -s - <(mod kanon < $T/perm) && pass || fail "bolum sirasi degismis tablo gidis-donusta bozuldu"
# ---- 6) davranis yansimasi: add -> topla (yalnizca sayisal tablo degisti)
rm -rf $T/w2; mkdir -p $T/w2/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $T/w2/oz/; rm -f $T/w2/oz/asm/asm.pmb $T/w2/oz/elf2/gen2.pmb
cp $T/ed.pmk $T/w2/oz/komut/komutlar.pmk
(cd $T/w2/oz/asm && ./uret.sh $PM > /dev/null 2>&1) && (cd $T/w2/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || fail "adi degisen tablodan arac uretilemedi"
printf 'func main() {\n ldi a, 40\n ldi c, 2\n toplam b, a, c\n host puti, b\n host putc, 10\n halt\n}\n' > $T/p_new.pma
sed 's/toplam/add/' $T/p_new.pma > $T/p_old.pma
(ulimit -v 900000; $PM run $T/w2/oz/asm/asm.pmb < $T/p_new.pma > $T/p_new.pmb 2>/dev/null); rn=$?
(ulimit -v 900000; $PM run $T/w2/oz/asm/asm.pmb < $T/p_old.pma > /dev/null 2>&1); ro=$?
(ulimit -v 900000; $PM run $R/oz/asm/asm.pmb < $T/p_old.pma > $T/p_old.pmb 2>/dev/null); rr=$?
(ulimit -v 900000; $PM run $R/oz/asm/asm.pmb < $T/p_new.pma > /dev/null 2>&1); rx=$?
[ $rn = 0 ] && [ $ro != 0 ] && [ $rr = 0 ] && [ $rx != 0 ] && pass || fail "mnemonik yansimasi: yeni(toplam)=$rn yeni(add)=$ro eski(add)=$rr eski(toplam)=$rx"
cmp -s $T/p_new.pmb $T/p_old.pmb && pass || fail "farkli mnemonikle ayni komut ayni PMB1'i uretmedi"
[ "$($PM run $T/p_new.pmb)" = "42" ] && pass || fail "yeni adli program calismadi"
$PM liste $T/p_new.pmb > $T/p.l; (ulimit -v 900000; $PM run $T/w2/oz/elf2/gen2.pmb < $T/p.l > $T/p.elf 2>/dev/null) && chmod +x $T/p.elf && [ "$($T/p.elf)" = "42" ] && pass || fail "adi degisen tablodan uretilen gen2 ile ELF calismadi"
# ---- 7) genisletme: yeni komut FMIN (d,f,f) = yalniz sayisal tablo: OP kaydi + SEM (anlam) kaydi. x86 sablonu YOK, C++ anlam kodu YOK.
#   anlam: fmin a,b,c = (b < c) ? b : c  (mikro-islemler: rd rd flt sel wr). Yurutme: svm (tabloyu calisma aninda okur) ve ELF (mikro-islem sablonlarindan).
#   Bagimsiz oracle: elle yazilmis C++ (b < c ? b : c) -- NaN, +-sonsuz, +-0, alt normal dahil tum cift desenleri bit duzeyinde.
rm -rf $T/w3; mkdir -p $T/w3/oz $T/w3/src; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $R/oz/sayisal $T/w3/oz/; cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $T/w3/oz/; cp -r $R/src $R/Makefile $T/w3/; rm -f $T/w3/oz/asm/asm.pmb $T/w3/oz/dogr/valid.pmb $T/w3/oz/elf2/gen2.pmb
mod gor < $TAB | awk '{print} /^op 54 GEI/{print "op 55 FMIN fmin d,f,f"} /^sem GEI /{print "sem FMIN rd t1 b ; rd t2 c ; flt t3 t1 t2 ; sel t4 t3 t1 t2 ; wr a t4"}' | mod yaz > $T/w3/oz/komut/komutlar.pmk
[ "$(mod dogrula < $T/w3/oz/komut/komutlar.pmk)" = "1" ] && pass || fail "genisletilmis tablo gecersiz"
mod opsh < $T/w3/oz/komut/komutlar.pmk > $T/w3/src/ops_gen.h
for t in asm dogr elf2; do (cd $T/w3/oz/$t && ./uret.sh $PM > /dev/null 2>&1) || fail "genisletilmis tablodan $t uretilemedi"; done
(cd $T/w3 && make pm > $T/make.log 2>&1) || fail "genisletilmis ops_gen.h ile C++ derlenemedi: $(grep -m1 error $T/make.log)"
printf 'func main() {\n mov a, 1.5\n mov b, 2.5\n fmin c, a, b\n host putf, c\n host putc, 10\n mov d, -3.0\n fmin e, b, d\n host putf, e\n host putc, 10\n halt\n}\n' > $T/fm.pma
(ulimit -v 900000; $PM run $T/w3/oz/asm/asm.pmb < $T/fm.pma > $T/fm.pmb 2> $T/fm.err) && pass || fail "genisletilmis assembler fmin'i kabul etmedi: $(head -c 80 $T/fm.pmb)"
(ulimit -v 900000; $PM run $R/oz/asm/asm.pmb < $T/fm.pma > /dev/null 2>&1) && fail "eski assembler fmin'i kabul etti" || pass
$T/w3/pm liste $T/fm.pmb > $T/fm.cpp.l 2>/dev/null && pass || fail "yeni C++ liste araci fmin'i bilmiyor"
(cd $T/w3/oz && ./uret_ikili.sh $PM $T/w3/oz > /dev/null 2>&1) || fail "genisletilmis tablodan coz/yaz uretilemedi"
(ulimit -v 900000; $PM run $T/w3/oz/coz.pmy < $T/fm.pmb > $T/fm.l 2>/dev/null) && pass || fail "tablodan uretilen cozucu fmin'li PMB1'i cozemedi"
cmp -s $T/fm.l $T/fm.cpp.l && pass || fail "tablodan uretilen cozucu ile yeni C++ liste araci farkli liste uretti"
v=$( (ulimit -v 900000; $PM run $T/w3/oz/dogr/valid.pmb < $T/fm.l 2>/dev/null) | head -c 1 ); [ "$v" = "1" ] && pass || fail "genisletilmis dogrulayici fmin programini kabul etmedi ($v)"
(ulimit -v 900000; $PM run $T/w3/oz/elf2/gen2.pmb < $T/fm.l > $T/fm.elf 2>/dev/null) && chmod +x $T/fm.elf && [ "$($T/fm.elf)" = "$(printf '1.5\n-3')" ] && pass || fail "fmin ELF'i (yalniz SEM'den uretildi) beklenen cikti vermedi: $($T/fm.elf 2>&1 | head -c 60)"
# svm: ayni program, tablo calisma aninda okunur (derleme yok)
{ cat $T/w3/oz/komut/komutlar.pmk; echo "@@PROGRAM"; cat $T/fm.l; echo "@@GIRDI"; } > $T/fm.svm.in
[ "$( (ulimit -v 900000; $PM run $T/w3/oz/sayisal/svm.pmb < $T/fm.svm.in 2>/dev/null) )" = "$(printf '1.5\n-3')" ] && pass || fail "svm fmin'i SEM'den yurutemedi"
# tam desen taramasi: oracle (C++) = svm = ELF (yalniz-SEM) = ELF (minsd sablonu) = ELF (mikro-islemler zorlanmis)
$R/oz/elf2/ondalik_prog.sh $T/fo > /dev/null 2>&1
OZ=$(grep '^data tab' $T/fo/ozel.pma); NP=$(echo "$OZ" | wc -w); NP=$((NP-3))
{ echo "memory 4096"; echo "$OZ"; cat <<EOT
func main() {
    mov i, 0
li: gei z, i, $NP
    jnz z, ld
    add a, i, tab
    ld x, a, 0
    mov j, 0
lj: gei z, j, $NP
    jnz z, nj
    add a, j, tab
    ld y, a, 0
    fmin r, x, y
    host puti, r
    host putc, 32
    addi j, j, 1
    jmp lj
nj: host putc, 10
    addi i, i, 1
    jmp li
ld: halt
}
entry main
EOT
} > $T/fmall.pma
(ulimit -v 900000; $PM run $T/w3/oz/asm/asm.pmb < $T/fmall.pma > $T/fmall.pmb 2>/dev/null) && pass || fail "fmin tarama programi derlenemedi"
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
HEX=$(sed -n 's/^OZEL="\(.*\)"$/\1/p' $R/oz/elf2/ondalik_prog.sh | head -1 | tr ' ' '\n' | head -$NP | tr '\n' ' ')
g++ -O0 -o $T/fmin_oracle $T/fmin_oracle.cpp 2>/dev/null && $T/fmin_oracle $HEX > $T/fm.exp || fail "fmin oracle derlenemedi"
(ulimit -v 900000; $PM run $T/w3/oz/coz.pmy < $T/fmall.pmb > $T/fmall.l 2>/dev/null)
[ -s $T/fm.exp ] && [ "$(wc -l < $T/fm.exp)" = "$NP" ] && pass || fail "oracle ciktisi beklenmedik"
{ cat $T/w3/oz/komut/komutlar.pmk; echo "@@PROGRAM"; cat $T/fmall.l; echo "@@GIRDI"; } > $T/fmall.svm.in
(ulimit -v 900000; timeout 120 $PM run $T/w3/oz/sayisal/svm.pmb < $T/fmall.svm.in > $T/fm.o_svm 2>/dev/null); cmp -s $T/fm.o_svm $T/fm.exp && pass || fail "svm fmin cikti oracle'dan farkli: $(cmp $T/fm.o_svm $T/fm.exp 2>&1 | head -c 80)"
(ulimit -v 900000; $PM run $T/w3/oz/elf2/gen2.pmb < $T/fmall.l > $T/fmall.elf 2>/dev/null) && chmod +x $T/fmall.elf && $T/fmall.elf > $T/fm.o_elf; cmp -s $T/fm.o_elf $T/fm.exp && pass || fail "ELF (yalniz SEM) fmin cikti oracle'dan farkli: $(cmp $T/fm.o_elf $T/fm.exp 2>&1 | head -c 80)"
# elle yazilmis x86 sablonu (minsd) eklenirse SEM yolu ile ayni sonucu vermeli (iki farkli yerel uretim = ayni davranis)
cp -r $T/w3/oz/elf2 $T/w3/oz/elf2b; echo 'fmin: F2 0F 10 83 R_B F2 0F 5D 83 R_C F2 0F 11 83 R_A' >> $T/w3/oz/elf2b/sablon.txt; rm -f $T/w3/oz/elf2b/gen2.pmb
(cd $T/w3/oz/elf2b && ./uret.sh $PM > /dev/null 2>&1) || fail "sablonlu gen2 uretilemedi"
(ulimit -v 900000; $PM run $T/w3/oz/elf2b/gen2.pmb < $T/fmall.l > $T/fmall_b.elf 2>/dev/null) && chmod +x $T/fmall_b.elf && $T/fmall_b.elf > $T/fm.o_b; cmp -s $T/fm.o_b $T/fm.exp && pass || fail "ELF (minsd sablonu) fmin cikti oracle'dan farkli"
cmp -s $T/fmall.elf $T/fmall_b.elf && fail "sablonlu ve SEM'li ELF ayni bayt (SEM yolu kullanilmadi?)" || pass
(ulimit -v 900000; { echo "-6 1"; cat $T/fmall.l; } | $PM run $T/w3/oz/elf2b/gen2.pmb > $T/fmall_z.elf 2>/dev/null) && chmod +x $T/fmall_z.elf && $T/fmall_z.elf > $T/fm.o_z; cmp -s $T/fm.o_z $T/fm.exp && pass || fail "ELF (-6 1: sablonlu komut da SEM'den) fmin cikti oracle'dan farkli"
(ulimit -v 900000; { echo "-6 1"; cat $T/fmall.l; } | $PM run $T/w3/oz/elf2/gen2.pmb > $T/fmall_z2.elf 2>/dev/null); cmp -s $T/fmall_z.elf $T/fmall_z2.elf && pass || fail "-6 1 ELF'i sablonlu/sablonsuz gen2 ile ayni degil (mikro yolu ayni olmali)"
# ---- 8) ANLAM (PRIM + SEM) bolumleri: C++ referansiyla esitlik, olumsuz ornekler, genisletme noktasi
g++ -std=c++17 -I $R/src -o $T/anlam_dump gecis/anlam_dump.cpp 2>/dev/null && $T/anlam_dump | sort > $T/sem_cpp.txt || fail "anlam_dump derlenemedi"
mod gor < $TAB | grep '^sem ' | grep -v '^sem FMIN ' | sort > $T/sem_tab.txt
cmp -s $T/sem_cpp.txt $T/sem_tab.txt && pass || fail "tablodaki SEM tanimlari src/anlam.h (C++ referansi) ile ayni degil: $(diff $T/sem_cpp.txt $T/sem_tab.txt | head -3 | tr '\n' '|')"
mod gor < $TAB | grep -q '^prim 47 sel to,ti,ti,ti ext 1=0$' && pass || fail "prim satiri beklenen bicimde degil"
sred() { y=$(mod gor < $TAB | sed "$2" | mod yaz 2>/dev/null); if [ "${y:0:4}" = "HATA" ]; then pass; else out=$(printf '%s\n' "$y" | mod dogrula 2>/dev/null); [ "${out:0:1}" = "0" ] && pass || fail "SEM olumsuz ornek kabul edildi: $1 ($out)"; fi; }
sred "bilinmeyen islem" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV zzz t1 b ; wr a t1/'
sred "gecici once-tanimsiz" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV rd t1 b ; wr a t2/'
sred "operand yuvasi komutta yok (MOV'da c)" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV rd t1 c ; wr a t1/'
sred "bilinmeyen komut" 's/^sem MOV rd t1 b ; wr a t1$/sem NOSUCH rd t1 b ; wr a t1/'
sred "alan sayisi eksik" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV rd t1 ; wr a t1/'
sred "yinelenen anlam" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV rd t1 b ; wr a t1\nsem MOV rd t1 b ; wr a t1/'
sred "gecici sinir disi" 's/^sem MOV rd t1 b ; wr a t1$/sem MOV rd t9 b ; wr a t9/'
# genisletme noktasi: yeni islem (prim) tanimi + onu kullanan komut. Yurutuculer bilmedigi islemi SESSIZCE yok saymaz: reddeder.
mod gor < $TAB | awk '{print} /^prim 47 /{print "prim 48 xyz to,ti,ti ext 1=0"} /^op 54 GEI/{print "op 55 XYZ xyz d,r,r"} /^sem GEI /{print "sem XYZ rd t1 b ; rd t2 c ; xyz t3 t1 t2 ; wr a t3"}' | mod yaz > $T/xyz.pmk
[ "$(mod dogrula < $T/xyz.pmk)" = "1" ] && pass || fail "yeni islemli tablo gecersiz"
mod gor < $T/xyz.pmk | mod yaz | cmp -s - <(mod kanon < $T/xyz.pmk) && pass || fail "yeni islemli tablo gidis-donusta bozuldu"
mod tab < $T/xyz.pmk | grep -q '^PR_XYZ 48$' && pass || fail "yeni islem sozlukte yok (PR_XYZ)"
printf 'func main() {\n ldi a, 6\n ldi b, 7\n xyz c, a, b\n host puti, c\n halt\n}\n' > $T/xyz.pma
rm -rf $T/w4; mkdir -p $T/w4/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/elf2 $R/oz/sayisal $T/w4/oz/; rm -f $T/w4/oz/asm/asm.pmb $T/w4/oz/elf2/gen2.pmb; cp $T/xyz.pmk $T/w4/oz/komut/komutlar.pmk
(cd $T/w4/oz/asm && ./uret.sh $PM > /dev/null 2>&1) && (cd $T/w4/oz/elf2 && ./uret.sh $PM > /dev/null 2>&1) || fail "yeni islemli tablodan arac uretilemedi"
(ulimit -v 900000; $PM run $T/w4/oz/asm/asm.pmb < $T/xyz.pma > $T/xyz.pmb 2>/dev/null)
$PM run $R/oz/asm/asm.pmb < $T/xyz.pma > /dev/null 2>&1 && fail "eski assembler xyz'yi kabul etti" || pass
if [ -s $T/xyz.pmb ]; then
  cp $R/oz/uret_ikili.sh $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/paketle.pmy $T/w4/oz/; (cd $T/w4/oz && ./uret_ikili.sh $PM $T/w4/oz > /dev/null 2>&1) || fail "xyz tablosundan coz/yaz uretilemedi"
  (ulimit -v 900000; $PM run $T/w4/oz/coz.pmy < $T/xyz.pmb > $T/xyz.l 2>/dev/null)
  (ulimit -v 900000; $PM run $T/w4/oz/elf2/gen2.pmb < $T/xyz.l > $T/xyz.elf 2>/dev/null) && fail "gen2 sablonu olmayan islemli komutu reddetmedi" || { [ ! -s $T/xyz.elf ] && pass || fail "gen2 reddetti ama cikti verdi"; }
  { cat $T/xyz.pmk; echo "@@PROGRAM"; cat $T/xyz.l; echo "@@GIRDI"; } > $T/xyz.in
  (ulimit -v 900000; $PM run $T/w4/oz/sayisal/svm.pmb < $T/xyz.in > $T/xyz.o 2>/dev/null); [ "$(cat $T/xyz.o)" = "42" ] && fail "svm bilmedigi islemi uydurma yurutdu" || pass
else fail "xyz programi derlenemedi"; fi
# ---- 9) ILISKILER sayidir: yeni komut XORI (anlik XOR) + yeniden yazma kurali (XOR -> XORI), yeni atlama komutu GOTO (yalniz SEM: br). Kod degisikligi yok.
#   asm kuralı REWRITE bolumunden okur; dogrulayici sonlandirici/atlama/akis ozelligini SEM'den turetir; svm SEM'den calistirir; ELF XORI'yi mikro-islemlerden uretir.
rm -rf $T/w5; mkdir -p $T/w5/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $R/oz/sayisal $T/w5/oz/; cp $R/oz/paketle.pmy $R/oz/coz.sayi.in $R/oz/yaz.sayi.in $R/oz/uret_ikili.sh $T/w5/oz/; rm -f $T/w5/oz/asm/asm.pmb $T/w5/oz/dogr/valid.pmb $T/w5/oz/elf2/gen2.pmb
# XOR (kimlik 10) icin vakalar: c anlik -> XORI(55); b anlik -> takas + XORI (kayit uzunlugu 18, kaynak_turu 0)
mod gor < $TAB | awk '/^section 7 /{$5=$5+1; print $0 " 18 0 10 2 1 -2147483648 2147483647 1 1 55 2 -2147483648 2147483647 2 2 0 1 55 0"; next} {print} /^op 54 GEI/{print "op 55 XORI xori d,r,i"; print "op 56 GOTO goto l"} /^sem GEI /{print "sem XORI rd t1 b ; im t2 c ; alu_xor t3 t1 t2 ; wr a t3"; print "sem GOTO br a"}' | mod yaz > $T/w5/oz/komut/komutlar.pmk
[ "$(mod dogrula < $T/w5/oz/komut/komutlar.pmk)" = "1" ] && pass || fail "XORI/GOTO tablosu gecersiz: $(mod dogrula < $T/w5/oz/komut/komutlar.pmk)"
for t in asm dogr elf2; do (cd $T/w5/oz/$t && ./uret.sh $PM > /dev/null 2>&1) || fail "XORI/GOTO tablosundan $t uretilemedi"; done
(cd $T/w5/oz && ./uret_ikili.sh $PM $T/w5/oz > /dev/null 2>&1) || fail "XORI/GOTO tablosundan coz/yaz uretilemedi"
printf 'func main() {\n ldi a, 1000\n xor b, a, 5\n host puti, b\n host putc, 32\n xor c, 7, a\n host puti, c\n host putc, 32\n ldi d, 6\n xor e, a, d\n host puti, e\n host putc, 10\n halt\n}\n' > $T/xr.pma
(ulimit -v 900000; $PM run $T/w5/oz/asm/asm.pmb < $T/xr.pma > $T/xr.pmb 2>/dev/null) && pass || fail "genisletilmis asm xor programini derlemedi"
(ulimit -v 900000; $PM run $T/w5/oz/coz.pmy < $T/xr.pmb > $T/xr.l 2>/dev/null)
n_new=$(grep -c '^55 ' $T/xr.l); [ "$n_new" = "2" ] && pass || fail "yeniden yazma kurali: 2 XORI beklenirdi, bulunan $n_new"
$PM asm $T/xr.pma -o $T/xr_old.pmb --strip > /dev/null 2>&1; $PM liste $T/xr_old.pmb > $T/xr_old.l; grep -q '^55 ' $T/xr_old.l && fail "eski asm XORI uretti" || pass
v=$( (ulimit -v 900000; $PM run $T/w5/oz/dogr/valid.pmb < $T/xr.l 2>/dev/null) | head -c 1 ); [ "$v" = "1" ] && pass || fail "dogrulayici XORI'li programi reddetti ($v)"
exp="$((1000 ^ 5)) $((7 ^ 1000)) $((1000 ^ 6))"
(ulimit -v 900000; $PM run $T/w5/oz/elf2/gen2.pmb < $T/xr.l > $T/xr.elf 2>/dev/null) && chmod +x $T/xr.elf && [ "$($T/xr.elf)" = "$exp" ] && pass || fail "XORI ELF cikti: $($T/xr.elf 2>&1 | head -c 60) beklenen $exp"
{ cat $T/w5/oz/komut/komutlar.pmk; echo "@@PROGRAM"; cat $T/xr.l; echo "@@GIRDI"; } > $T/xr.in
[ "$( (ulimit -v 900000; $PM run $T/w5/oz/sayisal/svm.pmb < $T/xr.in 2>/dev/null) )" = "$exp" ] && pass || fail "svm XORI cikti beklenenden farkli"
# GOTO: kosulsuz atlama (yalniz SEM'de 'br'); asm kabul eder, dogrulayici sonlandirici/akis ozelligini SEM'den turetir, svm calistirir
printf 'func main() {\n ldi a, 1\n goto L1\n ldi a, 2\nL1: host puti, a\n host putc, 10\n halt\n}\n' > $T/gt.pma
(ulimit -v 900000; $PM run $T/w5/oz/asm/asm.pmb < $T/gt.pma > $T/gt.pmb 2>/dev/null) && pass || fail "genisletilmis asm goto'yu kabul etmedi"
$PM run $R/oz/asm/asm.pmb < $T/gt.pma > /dev/null 2>&1 && fail "eski asm goto'yu kabul etti" || pass
(ulimit -v 900000; $PM run $T/w5/oz/coz.pmy < $T/gt.pmb > $T/gt.l 2>/dev/null)
v=$( (ulimit -v 900000; $PM run $T/w5/oz/dogr/valid.pmb < $T/gt.l 2>/dev/null) | head -c 1 ); [ "$v" = "1" ] && pass || fail "dogrulayici goto'lu programi reddetti ($v)"
{ cat $T/w5/oz/komut/komutlar.pmk; echo "@@PROGRAM"; cat $T/gt.l; echo "@@GIRDI"; } > $T/gt.in
[ "$( (ulimit -v 900000; $PM run $T/w5/oz/sayisal/svm.pmb < $T/gt.in 2>/dev/null) )" = "1" ] && pass || fail "svm goto'yu yurutemedi"
# goto'dan sonra sonlandirici yok -> dogrulayici reddetmeli (OPFLOW=2 sonlandirici sayilir; JZ gibi kosullu olsaydi reddedilirdi): hedefi gecersiz yap
printf 'func main() {\n ldi a, 1\n goto L1\nL1: host puti, a\n goto L1\n}\n' > $T/gt2.pma
(ulimit -v 900000; $PM run $T/w5/oz/asm/asm.pmb < $T/gt2.pma > $T/gt2.pmb 2>/dev/null) && (ulimit -v 900000; $PM run $T/w5/oz/coz.pmy < $T/gt2.pmb > $T/gt2.l 2>/dev/null)
v=$( (ulimit -v 900000; $PM run $T/w5/oz/dogr/valid.pmb < $T/gt2.l 2>/dev/null) | head -c 1 ); [ "$v" = "1" ] && pass || fail "dogrulayici goto ile biten fonksiyonu reddetti ($v)"
echo "komut tablosu: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
