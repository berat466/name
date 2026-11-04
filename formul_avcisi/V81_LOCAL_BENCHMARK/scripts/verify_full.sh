#!/bin/bash
# verify_full.sh <bin_dizini> <veri_dizini> [isa] : TAM karma modu (V53H_HASH=1; tepe RAM ~6.2 GB!). Fc/AFF/RF/ACF/RC/Y0f/X/U/W_F karmalarini reference/expected_v81.json ile karsilastirir.
HERE="$(cd "$(dirname "$0")" && pwd)"; source "$HERE/env_v81.sh"; BIN="$1"; DATA="$2"; ISA="${3:-avx2}"; EXE=""; [ -f "$BIN/th_gen_data.exe" ] && EXE=".exe"
OUT="$HERE/../results"; mkdir -p "$OUT"; LOG="$OUT/verify_full_${ISA}.log"
"$HERE/run_v81.sh" "$BIN/tek_hamle_v81_${ISA}${EXE}" "$DATA" V53H_HASH=1 V78_AUTO=0 V76_DEBUG=1 > "$LOG" 2>&1
$PYTHON - "$LOG" "$HERE/../reference/expected_v81.json" <<'PY'
import re, sys, json
log = open(sys.argv[1], errors="ignore").read(); ref = json.load(open(sys.argv[2]))["tam_karma_modu_V53H_HASH_1"]
ok = True
for k, v in ref.items():
    m = re.search(r"V53H_HASH\|%s\|([0-9a-f]{16})" % k, log); got = m.group(1) if m else None
    exp = v.split()[0]; same = (got == exp); note = " (T: BLAS surumune bagli; W_F esitse sorun degil)" if k == "T" and not same else ""
    print(f"{k:4s} beklenen={exp} bulunan={got} {'ESIT' if same else 'FARKLI'}{note}")
    if not same and k != "T": ok = False
print("TAM KARMA DURUMU:", "TUM KRITIK KARMALAR ESIT" if ok else "FARK VAR (ilk farkli ara buyukluk yukaridaki sirada ilk FARKLI satirdir)")
PY
