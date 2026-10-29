#!/bin/bash
# METIN -> SAYISAL NESNE (PMO1) ICE AKTARIMI (istege bagli on yuz; derleme zinciri yalniz sayisal nesneyi kullanir).
# Girdi: sablonlu kaynak (.pma.in; @ANAHTAR@ yer tutuculari) ya da duz .pma.  Cikti: PMO1 nesnesi (sayilar):
#   satir 1: 1347243825 1 nanahtar nbagil          (PMO1, surum 1)
#   nanahtar satir: tur(0 skaler,1 dizi) taban adlen ad..       (skaler: taban = taslakta kullanilan yer tutucu degeri; dizi: taban uzunluk 1)
#   nbagil satir: konum anahtar_no katsayi         (taslak listesinde konumdaki sayiya katsayi*(deger-taban) eklenir: veri adresleri, say/boyut alanlari)
#   sonra: taslak liste (pm liste bicimi; skaler yer tutucu = S1+no, dizi yer tutucusu = S2+no*65536 tek sozcuk; eksilmis (sub->addi) skalerler -S1-no)
# Yer tutucular tek basina `data AD = @ANAHTAR@` satirindaki ise DIZI, aksi halde SKALER. Bagil kayitlar, anahtar uzunlugu/degeri degistirilerek ayni kaynagin yeniden derlenip
# listelerin karsilastirilmasiyla ELDE EDILIR (assembler'a dokunulmaz); dogrusallik 2. bir degerle sinanir.
# Kullanim: ice_aktar.sh kaynak.pma.in cikti.pmo   (PM, ARAC ortam degiskenleri: arac.sh)
D=$(cd "$(dirname "$0")" && pwd); SRC=${1:?kaynak}; OUT=${2:?cikti}; PM=${PM:-$D/../../pm}; . $D/../arac/arac.sh
T=$(mktemp -d); trap 'rm -rf $T' EXIT
S1=1845493760; S2=1879048192
grep -o '@[A-Za-z0-9_]*@' "$SRC" | tr -d '@' | sort -u | grep -v '^$' > $T/anahtarlar
grep -E '^data[[:space:]]+[A-Za-z0-9_]+[[:space:]]*=[[:space:]]*@[A-Za-z0-9_]+@[[:space:]]*$' "$SRC" | sed -E 's/.*@([A-Za-z0-9_]+)@.*/\1/' | sort -u > $T/diziler
n=0; : > $T/tur; while read -r k; do if grep -qx "$k" $T/diziler; then echo "$n 1 $k"; else echo "$n 0 $k"; fi; n=$((n+1)); done < $T/anahtarlar > $T/tur
nk=$n
# bir varyant derle: arguman: dizi anahtari (uzunluk 2) ya da skaler anahtari (+7) ya da bos (taslak); cikti: $T/v.tok (her satirda bir sayi, satir sonlari '#' ile)
derle() {
  local ozel="$1" tip="$2" no dz k w v
  while read -r no dz k; do
    if [ $dz = 1 ]; then
      w="$((S2 + no*65536))"; [ "$tip" = dizi ] && [ "$k" = "$ozel" ] && w="$w $((S2 + no*65536 + 1))"
      echo "$k $w"
    else
      v=$((S1 + no)); [ "$tip" = skaler ] && [ "$k" = "$ozel" ] && v=$((v + 7)); echo "$k $v"
    fi
  done < $T/tur > $T/sz
  { cat $T/sz; echo "@@SABLON"; cat "$SRC"; } | arac doldur > $T/v.pma || { echo "ice_aktar: doldur basarisiz"; exit 1; }
  "$PM" asm $T/v.pma -o $T/v.pmb --strip > $T/asm.log 2>&1 || { echo "ice_aktar: derleme basarisiz: $(head -c 200 $T/asm.log)"; exit 1; }
  "$PM" liste $T/v.pmb | awk '{for(i=1;i<=NF;i++) print $i; print "#"}' > $T/$3
}
derle "" taslak taslak.tok
# taslakta gercek sabit cakismasi: yer tutucu araligindaki sayilar yalniz yer tutucu olmali (kontrol sonunda dogrulanir: ayni kaynak gercek sozlukle kurulunca ayni olmali -- testte)
: > $T/bagil
while read -r no dz k; do
  if [ $dz = 1 ]; then
    derle "$k" dizi v.tok
    # ek yer tutucu sozcugu (S2+no*65536+1) cikar
    awk -v w=$((S2 + no*65536 + 1)) '$0!=w' $T/v.tok > $T/v2.tok
    [ "$(wc -l < $T/v2.tok)" = "$(wc -l < $T/taslak.tok)" ] || { echo "ice_aktar: $k: dizi varyanti beklenmedik uzunlukta"; exit 1; }
    paste -d' ' $T/taslak.tok $T/v2.tok | awk -v no=$no '$1=="#" {next} {i++} $1!=$2 {print i-1, no, $2-$1}' >> $T/bagil
  else
    derle "$k" skaler v.tok
    paste -d' ' $T/taslak.tok $T/v.tok | awk -v no=$no -v a=$((S1+no)) -v b=$((S1+no+7)) '$1=="#" {next} {i++} $1!=$2 && !($1==a && $2==b) && !($1==-a && $2==-b) {print i-1, no, $2-$1}' >> $T/bagil
  fi
done < $T/tur
nb=$(wc -l < $T/bagil)
{
  echo "1347243825 1 $nk $nb"
  while read -r no dz k; do
    if [ $dz = 1 ]; then taban=1; else taban=$((S1+no)); fi
    printf '%s %s %s' $dz $taban ${#k}; for ((i=0;i<${#k};i++)); do printf ' %d' "'${k:$i:1}"; done; echo
  done < $T/tur
  cat $T/bagil
} > $T/ust
# taslak listesi (satirli): son varyant degil, taslagin kendisi
derle "" taslak taslak.tok
"$PM" liste $T/v.pmb > $T/taslak.l
{ cat $T/ust; cat $T/taslak.l; } > "$OUT"
echo "ice_aktar: $OUT ($nk anahtar, $nb bagil kayit)"
