#!/bin/bash
# V53 nihai konfigurasyon (A+C+D). Gereksinim: /tmp/pg1342.txt (Gutenberg #1342), /tmp/v8data (V51b verisi; eski gommeler buradan geri alinir)
set -e; cd "$(dirname "$0")"
python3 metin_hazirla_v53.py yeniemb /tmp/v53A_yeni
L=/usr/local/lib/python3.13/dist-packages/scipy.libs
g++ -O3 -march=native -fopenmp -std=c++17 -o ../cpp/tek_hamle_v53 ../cpp/tek_hamle_v53.cpp -L$L -l:libscipy_openblas-5f890258.so -Wl,-rpath,$L
source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15
export V53_PMIN=1 V53_LAMP=50 V53_QKAP=3 V53_QPI=1      # D: tum cift slotlari (cift lambda=50) ; C: Q Dirichlet kappa=3 + Kneser-Ney devam eden unigram
../cpp/tek_hamle_v53 2 512 1 1 1 0.5 Z 1 Y /tmp/v53A_yeni 85 85 3000 | tee ham/final_kosu.txt
