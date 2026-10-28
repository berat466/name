#!/bin/bash
# Bir kit dizinini degerlendirir: ilk deneme (deneme1) ve son teslim (son); ajan kayit istatistigi.
# Kullanim: ANAHTAR=... ENC4=... ./topla4.sh KIT_DIZINI   (cikti: tek satir gorev kip k + iki SONUC)
K=${1%/}; b=$(basename $K); g=${b%%_*}; r=${b#*_}; f=${r%%_*}; k=${r##*_}; D=$(cd "$(dirname "$0")" && pwd)
ext=sayi; [ $f = pmt ] && ext=pmt; kip=$f
d1=$K/deneme1.$ext; so=$K/son.$ext
[ -f $so ] || so=$(ls -t $K/deneme*.$ext 2>/dev/null | head -1)
log=$K/kayit.log; tur=$(awk "{print \$4}" $log 2>/dev/null | sort -u | wc -l); hata=$(grep -vc " CALISTI" $log 2>/dev/null || echo 0)
sinif=$(awk '{ for (i = 1; i <= NF; i++) if ($i ~ /^(CALISTI|PAKETLEME|CALISMA_TUZAGI|IKILI_BICIM|DOGRULAMA|TABLO_DOGRULAMA|DAHILI_DERLEME|DERLEME_DOGRULAMA|BASKA_HATA)$/) { c[$i]++; break } } END { for (x in c) printf "%s=%d,", x, c[x] }' $log 2>/dev/null)
echo "KIT $b farkli_deneme=$tur hatali_calistirma=$hata siniflar=$sinif"
if [ -f $d1 ]; then echo "ILK $($D/degerlendir4.sh $g $kip $d1 | sed 's/^SONUC //')"; else echo "ILK yok"; fi
if [ -f $so ] && ! cmp -s $so $d1; then echo "SON $($D/degerlendir4.sh $g $kip $so | sed 's/^SONUC //')"; elif [ -f $so ]; then echo "SON =ILK"; else echo "SON yok"; fi
