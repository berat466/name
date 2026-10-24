export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V36_LOOF=100 V35_RANK=1
for g in 0.1 0.5 1; do
 V36=1 timeout 200 ../cpp/tek_hamle_v37 2 512 1 $g 1 $g Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > A_real_g$g.txt 2>&1
 V36=1 V35_YSHUF=1 timeout 200 ../cpp/tek_hamle_v37 2 512 1 $g 1 $g Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > B_shuf_g$g.txt 2>&1
 V36=1 V37_PCA=1 timeout 200 ../cpp/tek_hamle_v37 2 512 1 $g 1 $g Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > C_pca_g$g.txt 2>&1
done
timeout 200 ../cpp/tek_hamle_v37 2 512 1 1 1 0.5 Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > D_randF_g1.txt 2>&1
timeout 200 ../cpp/tek_hamle_v37 2 512 1 0.1 1 0.1 Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000 > D_randF_g0.1.txt 2>&1
touch ABCDONE
