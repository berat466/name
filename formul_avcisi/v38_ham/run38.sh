export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V36=1 V36_LOOF=100 V38=1 V35_RANK=1
for g in 0.1 1; do timeout 200 ../cpp/tek_hamle_v38 2 512 1 $g 1 $g Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > R_real_g$g.txt 2>&1; done
for g in 0.1 0.5 1; do V35_YSHUF=1 timeout 200 ../cpp/tek_hamle_v38 2 512 1 $g 1 $g Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > S_shuf_g$g.txt 2>&1; done
V38_EPS=0.5 timeout 200 ../cpp/tek_hamle_v38 2 512 1 0.5 1 0.5 Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > R_real_g0.5_eps0.5.txt 2>&1
touch DONE38
