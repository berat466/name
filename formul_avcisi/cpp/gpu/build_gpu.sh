#!/bin/bash
# Linux/WSL2. Gerekli: CUDA toolkit (nvcc, cuBLAS), python3 + scipy.
SP=$(python3 -c "import scipy,os;print(os.path.join(os.path.dirname(scipy.__file__),'..','scipy.libs'))")
LIB=$(ls $SP | grep libscipy_openblas | head -1)
nvcc -O3 -c gpu/gpu_shim.cu -o gpu/gpu_shim.o -Igpu
g++ -O3 -march=native -fopenmp -std=c++17 -DUSE_GPU -o tek_hamle_v51g tek_hamle_v51g.cpp gpu/gpu_shim.o -L$SP -l:$LIB -lcublas -lcudart -L/usr/local/cuda/lib64 -Wl,-rpath,$SP -Wl,-rpath,/usr/local/cuda/lib64
# Dogrulama: ayni komut CPU (tek_hamle_v51b) ve GPU (tek_hamle_v51g) ile calistirilir; dogrulama CE ve test PPL ayni cikmali.
# CPU referansi (temiz V50): V51_OOFPOOL=1 V51_GF=0 V51_GP=0 -> dogr CE 4.8318, test PPL 119.71
