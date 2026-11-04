export V33_BLAS=1 V36_C0=1 V37_BIAS=1
A="2 512 1 1 1 0.5 Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000"
timeout 100 ../cpp/tek_hamle_v39 $A > off_beta0.txt 2>&1
for b in 0.3 0.6; do V39=1 V39_BETA=$b timeout 100 ../cpp/tek_hamle_v39 $A > real_b$b.txt 2>&1; V39=1 V39_SHUF=1 V39_BETA=$b timeout 100 ../cpp/tek_hamle_v39 $A > shuf_b$b.txt 2>&1; done
touch DONE
