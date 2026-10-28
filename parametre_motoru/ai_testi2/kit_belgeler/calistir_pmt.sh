#!/bin/bash
# Kullanim: ./calistir.sh dosya.pmt [pm run bayraklari...]   (kayit.log'a hata katmaniyla ekler)
D=$(cd "$(dirname "$0")" && pwd); F=$1; shift; LOG=$D/kayit.log
n=$(( $(grep -c '' "$LOG" 2>/dev/null || echo 0) + 1 )); sum=$(md5sum < "$F" | cut -c1-8)
"$D/pm" run "$F" "$@" > "$D/.cikti" 2> "$D/.hata"; rc=$?
cat "$D/.cikti"; cat "$D/.hata" >&2
if [ $rc = 0 ]; then k=CALISTI; elif [ $rc = 2 ]; then k=CALISMA_TUZAGI; elif grep -q "HATA" "$D/.hata"; then k=DERLEME_DOGRULAMA; else k=BASKA_HATA; fi
echo "$n $(date +%s) $F $sum $k $(head -c 100 "$D/.hata" | tr '\n' ' ')" >> "$LOG"
exit $rc
