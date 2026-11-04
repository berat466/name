export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V39_BETA=0.9 V40_STAGES=1 V40_W=512 V46=1 V46_ONLY0=1 V46_RHOS=-0.35,-0.25,-0.15,0
L=100,250,300,1000,3000
A="2 512 1 1 1 0.5 Z 1 Y - 85 85 $L"
V49=0 timeout 200 ../cpp/tek_hamle_v49 $A > A_kontrol.txt 2>&1
V49=1 V49_S=2 timeout 200 ../cpp/tek_hamle_v49 $A > B0_olcek2.txt 2>&1
V49=2 timeout 200 ../cpp/tek_hamle_v49 $A > B_kuvvet.txt 2>&1
V49=3 timeout 200 ../cpp/tek_hamle_v49 $A > C_etkilesim.txt 2>&1
touch DONE
