#!/bin/bash
# oz/hizli.sh'in kendi denetimi: dosya -> alan eslemesi, kritik denetimlerin her zaman secilmesi, cikis kodu, basarisiz testin gorunmesi.
# Kullanim: dogrula_hizli.sh   (hizli.sh ayri test kosullarini --liste ile inceler; yalniz iki gercek calistirma yapar)
D=$(cd "$(dirname "$0")" && pwd); H=$D/hizli.sh; T=$(mktemp -d); ok=0; bad=0
pass() { ok=$((ok+1)); }; fail() { bad=$((bad+1)); echo "HATA: $1"; }
alanlar() { HIZLI_DOSYALAR="$1" $H --liste | head -1 | sed 's/.*alanlar: //'; }
var() { case " $(alanlar "$1") " in *" $2 "*) pass;; *) fail "$1 -> '$2' secilmedi ($(alanlar "$1"))";; esac; }
yok() { case " $(alanlar "$1") " in *" $2 "*) fail "$1 -> '$2' gereksiz secildi";; *) pass;; esac; }
# esleme
var oz/asm/asm.pma.in asm; var oz/asm/asm.pma.in kural; yok oz/asm/asm.pma.in elf2; yok oz/asm/asm.pma.in svm
var oz/dogr/valid.pma.in valid; yok oz/dogr/valid.pma.in asm
var oz/elf2/gen2.pma.in elf2; var oz/elf2/gen2.pma.in mikro; yok oz/elf2/gen2.pma.in asm
var oz/komut/komutlar.pmk asm; var oz/komut/komutlar.pmk valid; var oz/komut/komutlar.pmk elf2; var oz/komut/komutlar.pmk svm; var oz/komut/komutlar.pmk nesne
var oz/komut/rastgele_pmk.awk komut; yok oz/komut/rastgele_pmk.awk asm
var oz/nesne/asm.pmo asm; var oz/nesne/asm.pmo nesne; var oz/nesne/asm.pmo valid
var oz/sayisal/svm.pma svm; yok oz/sayisal/svm.pma asm
var oz/arac/baglayici.pma nesne
var src/vm.cpp asm; var src/vm.cpp elf2; var src/vm.cpp valid; var src/vm.cpp ikili
var oz/yap/yap.pma.in yap; yok oz/yap/yap.pma.in asm
var oz/coz.pmy ikili
var oz/bag/kur.sh yenile; yok oz/asm/asm.pma.in yenile
yok README.md asm; yok OZ_BARINDIRMA.md elf2
# kritik denetimler hicbir degisiklikte, belge degisikliginde bile eksik olmamali
for k in testler valid mikro arac; do var README.md $k; var oz/asm/asm.pma.in $k; done
# tam mod her paketi icerir (yavas zincirler --bag olmadan haric)
n=$($H --tam --liste | grep -vc '^sabotaj'); [ "$n" -ge 17 ] && pass || fail "--tam paket sayisi $n"
$H --tam --sabotaj --liste | grep -q '^sabotaj valid' && pass || fail "--sabotaj listesi"
# gercek calistirma 1: gecen test -> cikis 0, sonuc GECTI
HIZLI_DOSYALAR=oz/asm/dogrula_ondalik.sh LOG=$T/l1 $H ondalik > $T/o1 2>&1; rc=$?
[ $rc = 0 ] && grep -q 'GECTI  ondalik' $T/o1 && pass || fail "ondalik hizli kosusu gecmedi ($rc): $(tail -3 $T/o1)"
# gercek calistirma 2: pm bozuk -> test basarisiz olmali, cikis !=0, KALDI gorunmeli (hizli sistem hatayi yutmuyor)
printf '#!/bin/bash\nexit 1\n' > $T/sahte_pm; chmod +x $T/sahte_pm
PM=$T/sahte_pm LOG=$T/l2 $H ondalik > $T/o2 2>&1; rc=$?
[ $rc != 0 ] && grep -q '^  KALDI  ' $T/o2 && pass || fail "bozuk pm ile hizli sistem hata bildirmedi (rc=$rc)"
# bilinmeyen alan reddedilir
$H yokboylealan > /dev/null 2>&1; [ $? = 2 ] && pass || fail "bilinmeyen alan reddedilmedi"
rm -rf $T
echo "hizli test sistemi: $ok gecti, $bad basarisiz"; [ $bad = 0 ]
