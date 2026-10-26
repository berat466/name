#!/bin/bash
cd /home/user/name/formul_avcisi/cpp
SL=$(python3 -c "import scipy,os;print(os.path.join(os.path.dirname(os.path.dirname(scipy.__file__)),'scipy.libs'))"); LIB=$(ls $SL/libscipy_openblas*.so | head -1)
s=$(date +%s.%N); g++ -O3 -march=native -fopenmp -std=c++17 -o tek_hamle_v74bin tek_hamle_v73.cpp -L$SL -l:$(basename $LIB) -Wl,-rpath,$SL; e=$(date +%s.%N)
echo "DERLEME_SURESI $(python3 -c "print(round($e-$s,2))") s; sha256(kaynak)=$(sha256sum tek_hamle_v73.cpp | cut -c1-16) sha256(ikili)=$(sha256sum tek_hamle_v74bin | cut -c1-16)"
