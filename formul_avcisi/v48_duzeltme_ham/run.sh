export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V39_BETA=0.9 V40_STAGES=1 V40_W=512 V46=1 V48=1
V48_L=4 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L4_real.txt 2>&1
V48_L=4 V48_SHUF=1 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L4_shuf.txt 2>&1
V48_L=8 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L8_real.txt 2>&1
V48_L=4 V48_TAU=0.02 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L4_tau0.02.txt 2>&1
V48_L=4 V48_TAU=0.2 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L4_tau0.2.txt 2>&1
V48_L=2 V48_SHUF=1 timeout 300 ../cpp/tek_hamle_v48d 2 512 1 1 1 0.5 Z 1 Y - 85 85 1000 > L2_shuf.txt 2>&1
touch DONE
