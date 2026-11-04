# Sunucudaki V81 kosusunun BIREBIR ortam degiskenleri (v54_baglantili_hesaplama/env_hafif.sh + v51_ek_deneyler_ham/env.sh + v81 kos.sh)
# kullanim: THREADS=4 source scripts/env_v81.sh
export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V39_BETA=0.9 V40_STAGES=1 V40_W=512 V46=1 V46_ONLY0=1 V46_RHOS=-0.25 V49=2 V50=1 V51=1 V50_RHO=-0.25
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15 V53_PMIN=2 V53_LAMP=50 V53_QKAP=3 V53_QPI=1
# hizli yol bayraklari (V75-V81 varsayilan hizli yol; geri kalanlari program kendisi varsayilan acar)
export V75_STREAM=1 V63_ARROW=2 V64_IMPL=1 V64_SPQ=1 V64_COMPACT=1 V72_UCSC=1 V73_RHS=1 V73_SYRK=1 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1
export OMP_NUM_THREADS=${THREADS:-4} OPENBLAS_NUM_THREADS=${THREADS:-4}
# program argumanlari (veri dizini sonda eklenir): <ARGS_ON> <veri_dizini> <ARGS_SON>
export TH_ARGS_ON="2 512 1 1 1 0.5 Z 1 Y"
export TH_ARGS_SON="85 85 9000"
export PYTHON=$(command -v python3 || command -v python)
