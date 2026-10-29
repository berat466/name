#!/bin/bash
# make_package.sh [cikis.zip] : depodaki V81/V80 kaynaklarini src/ altina KOPYALAYARAK tasinabilir paket zip'i uretir (depoya kopya eklenmez)
HERE="$(cd "$(dirname "$0")/.." && pwd)"; REPO="$(cd "$HERE/.." && pwd)"; OUT="${1:-$HERE/../V81_LOCAL_BENCHMARK.zip}"
STG=$(mktemp -d)/V81_LOCAL_BENCHMARK; mkdir -p "$STG/src"
cp "$REPO/cpp/tek_hamle_v81.cpp" "$REPO/cpp/tek_hamle_v80.cpp" "$REPO/cpp/knn_sparse_v80.h" "$REPO/cpp/tanhf_vec.h" "$STG/src/"
for d in compat tools scripts data_src reference; do cp -r "$HERE/$d" "$STG/"; done
cp "$HERE/CMakeLists.txt" "$HERE/RUN_ALL.ps1" "$HERE/LOCAL_BENCHMARK_README.md" "$STG/"
cd "$STG" && ( find . -type f ! -name MANIFEST_SHA256.txt | sort | xargs sha256sum ) > MANIFEST_SHA256.txt
cd "$(dirname "$STG")" && rm -f "$OUT" && zip -qr "$OUT" V81_LOCAL_BENCHMARK && ls -la "$OUT" && sha256sum "$OUT"
