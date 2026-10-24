export V33_BLAS=1 V36_C0=1 V37_BIAS=1
A="2 512 1 1 1 0.5 Z 1 Y - 100 20,100 1000000000,3000,10000,30000,100000"
for b in 0 0.9; do
 V39=1 V39_BETA=$b timeout 100 ../cpp/tek_hamle_v40 $A > f4x128_b$b.txt 2>&1
 for w in 512 128; do V40_STAGES=1 V40_W=$w V39=1 V39_BETA=$b timeout 100 ../cpp/tek_hamle_v40 $A > f1x${w}_b$b.txt 2>&1; done
done
for b in 0.6; do for w in 512 128; do V40_STAGES=1 V40_W=$w V39=1 V39_BETA=$b timeout 100 ../cpp/tek_hamle_v40 $A > f1x${w}_b$b.txt 2>&1; done; done
V40_STAGES=1 V40_W=512 V39=1 V39_SHUF=1 V39_BETA=0.9 timeout 100 ../cpp/tek_hamle_v40 $A > f1x512_shuf_b0.9.txt 2>&1
touch DONE
