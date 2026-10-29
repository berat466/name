#!/bin/bash
# make_data.sh <bin_dizini> <cikis_veri_dizini> : kaynak-veriden (data_src) 10.001 kelimelik V59 veri kumesini uretir ve SHA-256 ile dogrular
HERE="$(cd "$(dirname "$0")" && pwd)"; BIN="$1"; OUT="$2"; mkdir -p "$OUT"
"$BIN/th_gen_data$( [ -f "$BIN/th_gen_data.exe" ] && echo .exe )" "$HERE/../data_src" "$OUT" || exit 1
( cd "$OUT" && sha256sum -c "$HERE/../reference/data_sha256.txt" ) && echo "VERI SHA-256 DOGRULAMASI: TAMAM (sunucudaki /tmp/v59_10001 ile bayt-bayt ayni)"
