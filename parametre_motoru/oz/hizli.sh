#!/bin/bash
# HIZLI TEST SISTEMI: her degisiklikten sonra yalniz ILGILI testleri ve KRITIK guvenlik denetimlerini calistirir (paralel); buyuk asamalardan sonra --tam ile hepsi.
# Mevcut test betiklerine DOKUNMAZ: onlari kendi argumanlariyla cagirir (hizli mod = ayni karsilastirma yontemi, daha az rastgele/mutasyon ornegi).
#
# Kullanim:
#   oz/hizli.sh                  degisen dosyalari (git: calisma agaci + izlenmeyen) alanlara esle; ilgili hizli testler + kritik denetimler
#   oz/hizli.sh --son            calisma agaci temizse son commit'in degistirdigi dosyalar
#   oz/hizli.sh --since REF      REF'ten bu yana degisenler
#   oz/hizli.sh ALAN...          alanlari elle sec (liste: --alanlar)
#   oz/hizli.sh --tam            tum dogrulama betikleri, KENDI varsayilan (tam) argumanlariyla (buyuk asama sonrasi)
#   oz/hizli.sh --tam --sabotaj  ayrica sabotaj (test-testi) betikleri
#   oz/hizli.sh --liste          ne calisacagini yazar, calistirmaz
#   oz/hizli.sh -j N             paralel is sayisi (varsayilan: cekirdek sayisi)
#   oz/hizli.sh --bag            yavas ELF zinciri testlerini (oz/bag) da ekle
# Cikis kodu: tum calisan testler gecerse 0; aksi halde 1. Basarisiz testin HATA satirlari ozetlenir, tam cikti $LOG dizininde kalir.
#
# KRITIK denetimler (her calistirmada, alan ne olursa olsun): ana test kosucusu (tests/), PMB1 dogrulayici (reddetme/guvenlik kapisi, azaltilmis mutasyon),
#   mikro-islem yerel uretimi (bellek siniri/tuzak/denetim akisi, pm ile bit-bit), arac/sozluk tazeligi. Bunlar kapatilamaz.
D=$(cd "$(dirname "$0")" && pwd); R=$(cd "$D/.." && pwd); cd "$R" || exit 1
PM=${PM:-$R/pm}
JOBS=$(nproc 2>/dev/null || echo 2)
MOD=degisen; SINCE=""; TAM=0; SAB=0; LISTE=0; BAG=0; ELLE=""

# ---- test paketleri:  ad | dizin | betik | hizli argumanlar | tam argumanlar | agirlik (buyuk once baslat) | aciklama
# (tam argumanlar bos = betigin kendi varsayilanlari; tests/ ve ikili farkli PM yolu bicimi kullanir, asagida ele alinir)
PAKET="
elf2|oz/elf2|dogrula_elf2.sh|6 8||100|gen2 ELF ureticisi, pm ile karsilastirma
oob|oz/elf|dogrula_oob.sh|||41|eski ELF ureticisi ld/st sinir denetimi
yap|oz/yap|dogrula_yap.sh|30 100||39|yapisal dil -> pma
ikili|oz|dogrula_ikili.sh|30||32|PMB1 kodlayici/cozucu
komut|oz/komut|dogrula_komut.sh|10||34|komut tablosu (PMK1)
valid|oz/dogr|dogrula_valid.sh|8 8||31|PMB1 dogrulayici (guvenlik kapisi)
asm|oz/asm|dogrula_asm.sh|20 100 20||30|assembler
kural|oz/asm|dogrula_kural.sh|||27|yeniden yazma kurallari
grafik|oz/elf2|dogrula_grafik.sh|||27|grafik ana makinesi
svm|oz/sayisal|dogrula_svm.sh|10||22|sayisal anlam yurutucusu (HIZLI=1)
nesne|oz/nesne|dogrula_nesne.sh|||15|PMO1 nesneleri ve baglayici
gorunum|oz/nesne|dogrula_gorunum.sh|10 10||40|PMO1 okunabilir gorunum ve duzenleme (gor/yaz, uctan uca duzenleme)
elf|oz/elf|dogrula_elf.sh|10||13|eski ELF ureticisi
mikro|oz/elf2|dogrula_mikro.sh|5||10|mikro-islem yerel uretimi (zorlanmis)
cagri|oz/elf2|dogrula_cagri.sh|||5|call/ret/halt/host mikro-islem sablonlari (yalniz SEM ile tanimli komutlar)
arac|oz/arac|dogrula_arac.sh|20||5|arac/sozluk tazeligi
testler|tests|run_tests.sh|||3|ana test kosucusu
ondalik|oz/asm|dogrula_ondalik.sh|5||1|ondalik sabit ayrisimi
float|oz/elf2|dogrula_float.sh|||1|ondalik makine kodu
bag|oz/bag|dogrula_bag.sh|8||300|ELF zinciri (kur.sh)
pmy2elf|oz/bag|dogrula_pmy2elf.sh|30||300|pmy -> ELF zinciri
yenile|oz/bag|dogrula_yenile.sh|||600|kendini yeniden uretme zinciri
"
SABOTAJ="
cagri|oz/elf2|sabotaj_cagri.sh
gorunum|oz/nesne|sabotaj_gorunum.sh
valid|oz/dogr|sabotaj_valid.sh
kural|oz/asm|sabotaj_kural.sh
ondalik|oz/asm|sabotaj_ondalik.sh
mikro|oz/elf2|sabotaj_mikro.sh
float|oz/elf2|sabotaj_float.sh
grafik|oz/elf2|sabotaj_grafik.sh
komut|oz/komut|sabotaj_komut.sh
svm|oz/sayisal|sabotaj_svm.sh
nesne|oz/nesne|sabotaj_nesne.sh
"
KRITIK="testler valid mikro arac"
TUMALAN="elf2 oob yap ikili komut valid asm kural grafik svm nesne gorunum elf mikro cagri arac testler ondalik float"

# ---- degisen dosya -> alanlar
alan_dosya() {
  case "$1" in
    *.md|*.txt|*.gitignore|LICENSE*|docs/*) ;;                                  # belge: test gerekmez
    src/*|Makefile|tests/*) echo "testler ikili elf elf2 oob mikro asm kural valid yap svm komut nesne arac ondalik float grafik" ;;   # C++ cekirdek = bagimsiz referans: her sey ona karsi karsilastirilir
    oz/komut/komutlar.pmk) echo "komut nesne gorunum kural mikro cagri svm arac asm valid elf2 ikili" ;;   # komut tablosu her araci uretir
    oz/komut/*) echo "komut" ;;                                                  # tablo disi: test/belge/gecis dosyalari
    oz/arac/*.sh|oz/arac/ref/*) echo "arac" ;;
    oz/arac/komut.*) echo "arac komut nesne gorunum asm valid elf2" ;;         # komut.pmb tum sozlukleri uretir
    oz/arac/gorunum.*) echo "arac gorunum" ;;
    oz/arac/*) echo "arac nesne gorunum komut" ;;
    oz/asm/dogrula_kural.sh|oz/asm/sabotaj_kural.sh) echo "kural" ;;
    oz/asm/dogrula_ondalik.sh|oz/asm/sabotaj_ondalik.sh|oz/asm/ondalik_gen.awk) echo "ondalik" ;;
    oz/asm/dogrula_asm.sh|oz/asm/mutpma.awk) echo "asm" ;;
    oz/asm/*) echo "asm kural ondalik" ;;
    oz/dogr/*) echo "valid" ;;
    oz/elf2/dogrula_*.sh|oz/elf2/sabotaj_*.sh|oz/elf2/*_prog.sh|oz/elf2/rastgele2.awk) echo "elf2 mikro float grafik cagri" ;;
    oz/elf2/*) echo "elf2 mikro float grafik cagri" ;;
    oz/elf/*) echo "elf oob" ;;
    oz/yap/*) echo "yap" ;;
    oz/nesne/dogrula_gorunum.sh|oz/nesne/sabotaj_gorunum.sh|oz/nesne/duzenle.sh|oz/nesne/ref/*) echo "gorunum" ;;
    oz/nesne/*.sh) echo "nesne" ;;
    oz/nesne/*) echo "nesne gorunum asm valid elf2 mikro" ;;
    oz/sayisal/*) echo "svm" ;;
    oz/bag/*) echo "bag pmy2elf yenile" ;;
    oz/hizli.sh) ;;
    oz/*) echo "ikili" ;;                                                      # oz/*.pmy, *.sayi.in, mutasyon.cpp, uret_ikili.sh ...
    *) echo "testler" ;;
  esac
}
paket_alan() { echo "$PAKET" | awk -F'|' -v a="$1" '$1==a {print}'; }
degisenler() {
  if [ -n "$HIZLI_DOSYALAR" ]; then printf '%s\n' $HIZLI_DOSYALAR; return; fi   # test kancasi: degisen dosya listesini disaridan ver
  case "$MOD" in
    since) git diff --relative --name-only "$SINCE" ; git ls-files --others --exclude-standard ;;
    son) git diff --relative --name-only HEAD~1 HEAD ;;
    *) { git diff --relative --name-only HEAD; git ls-files --others --exclude-standard; } ;;
  esac | sort -u
}

while [ $# -gt 0 ]; do
  case "$1" in
    --tam) TAM=1;; --sabotaj) SAB=1;; --liste) LISTE=1;; --bag) BAG=1;;
    --son) MOD=son;; --since) MOD=since; SINCE=${2:?REF}; shift;;
    -j) JOBS=${2:?N}; shift;;
    --alanlar) echo "$PAKET" | awk -F'|' 'NF>5 {printf "%-8s %-12s %s\n", $1, $2, $7}'; exit 0;;
    -h|--help) sed -n 2,19p "$0"; exit 0;;
    -*) echo "bilinmeyen secenek: $1" >&2; exit 2;;
    *) ELLE="$ELLE $1";;
  esac
  shift
done

# ---- calisacak kume
SEC=""
if [ -n "$ELLE" ]; then
  SEC="$ELLE"
elif [ $TAM = 1 ]; then
  SEC="$TUMALAN"; [ $BAG = 1 ] && SEC="$SEC bag pmy2elf yenile"
else
  GEC=$(mktemp); degisenler > $GEC
  while IFS= read -r f; do alan_dosya "$f"; done < $GEC | tr ' ' '\n' | grep . | sort -u > $GEC.a
  SEC="$(cat $GEC.a | tr '\n' ' ')"; NCH=$(grep -c . $GEC)
  rm -f $GEC $GEC.a
  [ $BAG = 1 ] && SEC="$SEC bag pmy2elf"
fi
SEC="$SEC $KRITIK"
# tekrarsiz, agirlik sirasi (buyuk once)
LIST=""
for a in $(echo $SEC | tr ' ' '\n' | sort -u); do
  p=$(paket_alan $a); [ -z "$p" ] && { echo "bilinmeyen alan: $a (liste: --alanlar)" >&2; exit 2; }
  LIST="$LIST$(echo "$p" | awk -F'|' '{print $6 "|" $0}')
"
done
LIST=$(echo "$LIST" | grep . | sort -t'|' -k1,1nr | cut -d'|' -f2-)

if [ -z "$ELLE" ] && [ $TAM = 0 ]; then
  echo "degisen dosya: ${NCH:-0}; alanlar: $(echo "$LIST" | cut -d'|' -f1 | tr '\n' ' ')"
  KR=""; for k in $KRITIK; do KR="$KR $k"; done; echo "kritik (her zaman):$KR"
fi

# ---- calistirma
cekirdek_hazir() {
  [ "$PM" != "$R/pm" ] && return    # disaridan verilen pm'e dokunma
  if [ ! -x "$PM" ] || [ -n "$(find src -newer "$PM" -print -quit 2>/dev/null)" ]; then
    echo "pm (C++ referans) derleniyor..."; make pm > /dev/null 2>&1 || { echo "HATA: make pm basarisiz"; exit 1; }
  fi
}
komut_satiri() {   # ad dizin betik hizli tam  -> calistirilacak komut
  local ad=$1 dz=$2 bt=$3 hz=$4 tm=$5 args
  [ $TAM = 1 ] && args="$tm" || args="$hz"
  [ -n "$ELLE" ] && [ $TAM = 0 ] && args="$hz"
  case "$ad" in
    svm) [ $TAM = 0 ] && echo "env HIZLI=1 ./$bt $PM $args" || echo "./$bt $PM $args";;
    *) echo "./$bt $PM $args";;
  esac
}
if [ $LISTE = 1 ]; then
  echo "$LIST" | while IFS='|' read -r ad dz bt hz tm w ac; do printf '%-8s %-14s %s\n' "$ad" "$dz" "$(komut_satiri "$ad" "$dz" "$bt" "$hz" "$tm")"; done
  [ $SAB = 1 ] && echo "$SABOTAJ" | grep . | while IFS='|' read -r ad dz bt; do printf 'sabotaj %-8s %-14s ./%s\n' "$ad" "$dz" "$bt"; done
  exit 0
fi
cekirdek_hazir
LOG=${LOG:-$(mktemp -d)}; mkdir -p "$LOG"
T0=$(date +%s)
bos_is() { local r; r=$(jobs -rp | wc -l); echo $r; }
basla() {   # ad dizin komut
  ( local t0=$(date +%s); cd "$2" && bash -c "ulimit -c 0; $3" > "$LOG/$1.log" 2>&1; echo "$? $(( $(date +%s) - t0 ))" > "$LOG/$1.res" ) &
}
while IFS='|' read -r ad dz bt hz tm w ac; do
  [ -z "$ad" ] && continue
  while [ "$(bos_is)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done
  basla "$ad" "$R/$dz" "$(komut_satiri "$ad" "$dz" "$bt" "$hz" "$tm")"
done <<< "$LIST"
wait
if [ $SAB = 1 ]; then
  while IFS='|' read -r ad dz bt; do
    [ -z "$ad" ] && continue
    case " $LIST" in *"
$ad|"*) ;; *) [ $TAM = 0 ] && [ -z "$ELLE" ] && continue;; esac
    while [ "$(bos_is)" -ge "$JOBS" ]; do wait -n 2>/dev/null || sleep 0.2; done
    basla "sabotaj_$ad" "$R/$dz" "./$bt $PM"
  done <<< "$SABOTAJ"
  wait
fi

# ---- ozet
FAIL=0; N=0
for r in $LOG/*.res; do
  ad=$(basename $r .res); read -r rc sn < $r; N=$((N+1))
  son=$(tail -n 1 "$LOG/$ad.log" | cut -c1-120)
  if [ "$rc" = 0 ]; then printf '  GECTI  %-14s %4ss  %s\n' "$ad" "$sn" "$son"
  else FAIL=$((FAIL+1)); printf '  KALDI  %-14s %4ss  rc=%s  %s\n' "$ad" "$sn" "$rc" "$son"; grep -a '^HATA' "$LOG/$ad.log" | head -5 | sed 's/^/           /'; fi
done
echo "toplam $N paket, $FAIL basarisiz, $(( $(date +%s) - T0 )) sn (paralel $JOBS); tam ciktilar: $LOG"
[ $TAM = 0 ] && [ -z "$ELLE" ] && echo "not: hizli mod azaltilmis ornek sayisiyla calisir; buyuk asama sonrasi oz/hizli.sh --tam"
[ $FAIL = 0 ]
