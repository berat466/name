#!/bin/bash
# Kullanim: ./calistir.sh dosya.sayi [pm run bayraklari...]
# Sayilari baytlara cevirir, calistirir, kayit.log'a hata katmaniyla ekler.
D=$(cd "$(dirname "$0")" && pwd); F=$1; shift
B=${F%.sayi}.pmb; LOG=$D/kayit.log
n=$(( $(grep -c '' "$LOG" 2>/dev/null || echo 0) + 1 ))
sum=$(md5sum < "$F" | cut -c1-8)
if ! LC_ALL=C awk -f "$D/paketle.awk" "$F" > "$B" 2> "$D/.hata"; then
  echo "$n $(date +%s) $F $sum PAKETLEME $(head -c 120 "$D/.hata" | tr '\n' ' ')" >> "$LOG"; cat "$D/.hata"; exit 1; fi
"$D/pm" run "$B" "$@" > "$D/.cikti" 2> "$D/.hata"; rc=$?
cat "$D/.cikti"; cat "$D/.hata" >&2
if [ $rc = 0 ]; then k=CALISTI
elif [ $rc = 2 ]; then k=CALISMA_TUZAGI
elif grep -q "ikili:" "$D/.hata"; then k=IKILI_BICIM
elif grep -q "dogrulama:" "$D/.hata"; then k=DOGRULAMA
else k=BASKA_HATA; fi
echo "$n $(date +%s) $F $sum $k bayt=$(stat -c %s "$B") $(head -c 100 "$D/.hata" | tr '\n' ' ')" >> "$LOG"
exit $rc
