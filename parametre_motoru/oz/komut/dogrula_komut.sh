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
mod tab < gecis/komutlar_v1.pmk | head -4 > $T/t4.new
arac opstab < gecis/ops.h.txt > $T/t4.old
[ "$(tr -s ' \n' ' ' < $T/t4.new)" = "$(tr -s ' \n' ' ' < $T/t4.old)" ] && pass || fail "sozluk (OPCOUNT/OPNAMES/NOPS/KIND) eski opstab ciktisindan farkli"
# eski ops.h'in X satirlari = tablodan uretilen X satirlari (bosluk duzeninden bagimsiz)
norm() { grep -E '^ *X\(' | sed -E 's/ *\\ *$//; s/ +//g; s/^X\(([A-Z0-9_]+),"([a-z0-9_]+)",([0-9]),"([a-zA-Z]*)"\)/\1 \2 \3 \4/' | grep -E '^[A-Z0-9_]+ [a-z0-9_]+ [0-9] [a-zA-Z]*$' ; }
mod opsh < gecis/komutlar_v1.pmk | norm > $T/x.new; norm < gecis/ops.h.txt > $T/x.old
[ -s $T/x.old ] && cmp -s $T/x.new $T/x.old && pass || fail "ops_gen.h komut satirlari eski ops.h ile farkli"
cmp -s $TAB gecis/komutlar_v1.pmk && echo "bilgi: komutlar.pmk == komutlar_v1.pmk (tablo genisletilmedi)" || echo "bilgi: komutlar.pmk komutlar_v1.pmk'den farkli (kasitli genisletme)"
# ---- 3) tablodan yeniden uretim (ops.h OKUNMADAN): bayt-bayt ayni
rm -rf $T/w; mkdir -p $T/w/oz; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $T/w/oz/
rm -f $T/w/oz/asm/asm.pmb $T/w/oz/dogr/valid.pmb $T/w/oz/elf2/gen2.pmb
(cd $T/w/oz/arac && ./uret.sh $PM > /dev/null 2>&1) && for t in asm dogr elf2; do (cd $T/w/oz/$t && ./uret.sh $PM > /dev/null 2>&1) || fail "$t: tablodan uretilemedi"; done
cmp -s $T/w/oz/asm/asm.pmb $R/oz/asm/asm.pmb && pass || fail "tablodan uretilen asm.pmb depodakiyle farkli"
cmp -s $T/w/oz/dogr/valid.pmb $R/oz/dogr/valid.pmb && pass || fail "tablodan uretilen valid.pmb depodakiyle farkli"
cmp -s $T/w/oz/elf2/gen2.pmb $R/oz/elf2/gen2.pmb && pass || fail "tablodan uretilen gen2.pmb depodakiyle farkli"
cmp -s $T/w/oz/arac/sablonc.pmb $R/oz/arac/sablonc.pmb && pass || fail "sablonc.pmb farkli"
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
sat 'NR==1{$4=4} {print}' > $T/n1; red "bolum sayisi fazla" $T/n1
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
sat 'NR==1{$4=4} {print} END{print "77 0 1 3 10 20 30"}' > $T/opt; kab "bilinmeyen istege bagli bolum" $T/opt
sat 'NR==1{$4=4} {print} END{print "77 1 1 3 10 20 30"}' > $T/req; red "bilinmeyen ZORUNLU bolum" $T/req
mod tab < $T/opt | cmp -s - <(mod tab < $TAB) && pass || fail "bilinmeyen bolum sozluk ciktisini etkiledi"
mod gor < $T/opt | mod yaz | cmp -s - <(mod kanon < $T/opt) && pass || fail "bilinmeyen bolum gidis-donusta kayboldu"
# kayit uzantilari: bir komuta ext (anahtar 9=99) + kuyruk (5 6 7) ekle: reclen +4, ext sayisi 0->1
sat 'NR==14{$4=$4+5} NR==16{n=NF; $1=$1+5; $n=1; $(n+1)=9; $(n+2)=99; $(n+3)=5; $(n+4)=6; $(n+5)=7} {print}' > $T/ext1
kab "kayit uzantisi + kuyruk" $T/ext1
mod tab < $T/ext1 | cmp -s - <(mod tab < $TAB) && pass || fail "kayit uzantisi sozluk ciktisini etkiledi"
mod gor < $T/ext1 | mod yaz | cmp -s - <(mod kanon < $T/ext1) && pass || fail "kayit uzantisi/kuyruk gidis-donusta kayboldu"
# profil: seyrek kimlik (komut 54 -> 60) format olarak gecerli, profil icin gecersiz
sat 'NR==70{$2=60} {print}' > $T/sp; mod kanon < $T/sp | head -1 | grep -q '^1347242801' && pass || fail "seyrek kimlik bicim olarak reddedildi"
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
# ---- 7) genisletme: yeni komut FMIN (d,f,f) = yalniz tablo satiri + tek sablon satiri
rm -rf $T/w3; mkdir -p $T/w3/oz $T/w3/src; cp -r $R/oz/komut $R/oz/arac $R/oz/asm $R/oz/dogr $R/oz/elf2 $T/w3/oz/; cp -r $R/src $R/Makefile $T/w3/; rm -f $T/w3/oz/asm/asm.pmb $T/w3/oz/dogr/valid.pmb $T/w3/oz/elf2/gen2.pmb
mod gor < $TAB | awk '{print} /^op 54 GEI/{print "op 55 FMIN fmin d,f,f"}' | mod yaz > $T/w3/oz/komut/komutlar.pmk
[ "$(mod dogrula < $T/w3/oz/komut/komutlar.pmk)" = "1" ] && pass || fail "genisletilmis tablo gecersiz"
echo 'fmin: F2 0F 10 83 R_B F2 0F 5D 83 R_C F2 0F 11 83 R_A' >> $T/w3/oz/elf2/sablon.txt
mod opsh < $T/w3/oz/komut/komutlar.pmk > $T/w3/src/ops_gen.h
for t in asm dogr elf2; do (cd $T/w3/oz/$t && ./uret.sh $PM > /dev/null 2>&1) || fail "genisletilmis tablodan $t uretilemedi"; done
(cd $T/w3 && make pm > $T/make.log 2>&1) || fail "genisletilmis ops_gen.h ile C++ derlenemedi: $(grep -m1 error $T/make.log)"
printf 'func main() {\n mov a, 1.5\n mov b, 2.5\n fmin c, a, b\n host putf, c\n host putc, 10\n mov d, -3.0\n fmin e, b, d\n host putf, e\n host putc, 10\n halt\n}\n' > $T/fm.pma
(ulimit -v 900000; $PM run $T/w3/oz/asm/asm.pmb < $T/fm.pma > $T/fm.pmb 2> $T/fm.err) && pass || fail "genisletilmis assembler fmin'i kabul etmedi: $(head -c 80 $T/fm.pmb)"
(ulimit -v 900000; $PM run $R/oz/asm/asm.pmb < $T/fm.pma > /dev/null 2>&1) && fail "eski assembler fmin'i kabul etti" || pass
$T/w3/pm liste $T/fm.pmb > $T/fm.l 2>/dev/null && pass || fail "yeni liste araci fmin'i bilmiyor"
v=$( (ulimit -v 900000; $PM run $T/w3/oz/dogr/valid.pmb < $T/fm.l 2>/dev/null) | head -c 1 ); [ "$v" = "1" ] && pass || fail "genisletilmis dogrulayici fmin programini kabul etmedi ($v)"
(ulimit -v 900000; $PM run $T/w3/oz/elf2/gen2.pmb < $T/fm.l > $T/fm.elf 2>/dev/null) && chmod +x $T/fm.elf && [ "$($T/fm.elf)" = "$(printf '1.5\n-3')" ] && pass || fail "fmin ELF'i beklenen cikti vermedi: $($T/fm.elf 2>&1 | head -c 60)"
echo "komut tablosu: $ok gecti, $bad basarisiz"
rm -rf $T
[ $bad = 0 ]
