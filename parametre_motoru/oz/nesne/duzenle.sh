#!/bin/bash
# PMO1 SAYISAL DUZENLEME: asil kaynak sayisal nesnedir (.pmo); okunabilir gorunum (.pmv) ISTEGE BAGLIDIR ve kayipsizdir.
#   sayisal parametreler (.pmo) --gor--> okunabilir gorunum (.pmv) --elle/arac duzenleme--> --yaz--> guncellenmis sayilar (.pmo) --baglayici--> .pmb --gen2--> makine kodu (ELF)
# Kullanim:
#   duzenle.sh gor    NESNE.pmo [GORUNUM.pmv]   sayilar -> okunabilir metin. NESNE.adlar varsa islev adlari ve bss bolgeleri kullanilir (yoksa f0.. ve &bss+n)
#   duzenle.sh yaz    GORUNUM.pmv [NESNE.pmo]   okunabilir metin -> sayilar (+ NESNE.adlar). gor'un tam tersi: gor -> yaz bayt-bayt aynidir
#   duzenle.sh uygula GORUNUM.pmv ARAC          ARAC = asm | valid | gen2: oz/nesne/ARAC.pmo ve .adlar yerine yazilir, ARAC yeniden uretilir (.pmb). Hata olursa eski dosyalar geri konur
#   duzenle.sh elf    ARAC [CIKTI.elf]          ARAC'in .pmb'sinden bagimsiz makine kodu (ELF) uretir (gen2 ile; tum zincir icin oz/bag/kur.sh)
#   duzenle.sh sozluk                           komut tablosundan sozluk satirlari (stdout)
# Ortam: PM (pm yolu; varsayilan ../../pm) ya da ARAC (bagimsiz .elf araclari dizini); OPT (komut tablosu; varsayilan oz/komut/komutlar.pmk)
D=$(cd "$(dirname "$0")" && pwd); R=$D/..; PM=${PM:-$R/../pm}; [ -x "$PM" ] && PM=$(realpath "$PM"); export PM; . $R/arac/arac.sh
OPT=${OPT:-$R/komut/komutlar.pmk}
sozluk() { { echo "@@tab"; cat "$OPT"; } | arac komut; }
hata() { echo "duzenle: $*" >&2; exit 1; }
dizin() { case "$1" in asm) echo asm;; valid) echo dogr;; gen2) echo elf2;; *) hata "bilinmeyen arac: $1 (asm | valid | gen2)";; esac; }

gor() {   # pmo [cikti]
  local pmo=$1 out=$2 ad="${1%.pmo}.adlar" tmp; tmp=$(mktemp)
  [ -f "$pmo" ] || hata "yok: $pmo"
  { echo "@@gor"; sozluk; [ -f "$ad" ] && { echo "@@ADLAR"; cat "$ad"; }; echo "@@NESNE"; cat "$pmo"; } | arac gorunum > "$tmp" 2>&1 || { local m; m=$(head -c 200 "$tmp"); rm -f "$tmp"; hata "gor basarisiz: $m"; }
  if [ -n "$out" ]; then mv "$tmp" "$out"; else cat "$tmp"; rm -f "$tmp"; fi
}
yaz() {   # pmv [cikti]
  local pmv=$1 out=${2:-${1%.pmv}.pmo}
  [ -f "$pmv" ] || hata "yok: $pmv"
  { echo "@@yaz"; sozluk; echo "@@GORUNUM"; cat "$pmv"; } | arac gorunum > "$out.tmp$$" 2> /dev/null || { local m; m=$(head -c 200 "$out.tmp$$"); rm -f "$out.tmp$$"; hata "yaz basarisiz: $m"; }
  { echo "@@adlar"; cat "$pmv"; } | arac gorunum > "${out%.pmo}.adlar.tmp$$" 2> /dev/null || { rm -f "$out.tmp$$" "${out%.pmo}.adlar.tmp$$"; hata "adlar cikarilamadi"; }
  mv "$out.tmp$$" "$out"; mv "${out%.pmo}.adlar.tmp$$" "${out%.pmo}.adlar"
}
uygula() {  # pmv arac
  local pmv=$1 t=$2 dz; dz=$(dizin "$t"); local pmo=$D/$t.pmo ad=$D/$t.adlar y; y=$(mktemp -d)
  cp "$pmo" "$y/eski.pmo"; cp "$ad" "$y/eski.adlar" 2>/dev/null; cp "$R/$dz/$t.pmb" "$y/eski.pmb"
  yaz "$pmv" "$y/yeni.pmo" || { rm -rf "$y"; exit 1; }
  cp "$y/yeni.pmo" "$pmo"; cp "$y/yeni.adlar" "$ad"
  if ! (cd "$R/$dz" && OPT=$OPT ./uret.sh "$PM") > "$y/uret.log" 2>&1; then
    cp "$y/eski.pmo" "$pmo"; [ -f "$y/eski.adlar" ] && cp "$y/eski.adlar" "$ad"; cp "$y/eski.pmb" "$R/$dz/$t.pmb"
    local m; m=$(tail -n 3 "$y/uret.log"); rm -rf "$y"; hata "$t uretilemedi (eski dosyalar geri konuldu): $m"
  fi
  rm -rf "$y"; echo "duzenle: $t guncellendi: oz/nesne/$t.pmo, $t.adlar, oz/$dz/$t.pmb"
}
elf() {  # arac [cikti]
  local t=$1 dz; dz=$(dizin "$t"); local out=${2:-$D/$t.elf}
  local l; l=$(mktemp); "$PM" liste "$R/$dz/$t.pmb" > "$l" || hata "liste basarisiz"
  if [ -n "$ARAC" ] && [ -x "$ARAC/gen2.elf" ]; then (ulimit -v 900000; "$ARAC/gen2.elf" < "$l" > "$out"); else (ulimit -v 900000; "$PM" run "$R/elf2/gen2.pmb" < "$l" > "$out"); fi || { rm -f "$l"; hata "ELF uretilemedi"; }
  chmod +x "$out"; rm -f "$l"; echo "duzenle: $out ($(stat -c %s "$out") bayt)"
}
case "$1" in
  gor) shift; gor "$@";;
  yaz) shift; yaz "$@";;
  uygula) shift; uygula "$@";;
  elf) shift; elf "$@";;
  sozluk) sozluk;;
  *) sed -n 2,13p "$0"; exit 2;;
esac
