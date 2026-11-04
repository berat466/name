export V33_BLAS=1 V36_C0=1 V37_BIAS=1 V39=1 V40_STAGES=1 V40_W=512
A="2 512 1 1 1 0.5 Z 1 Y - 85 85 1000"
for t in 0.4 1.0; do V39_BETA=0.9 V45_TAU=$t timeout 100 ../cpp/tek_hamle_v45 $A > B_tau$t.txt 2>&1; done
for k in 100 1000; do for f in 0.5 0.8; do V39_BETA=0.9 V45_C=1 V45_NORHO=1 V45_KAPPA=$k V45_F=$f timeout 100 ../cpp/tek_hamle_v45 $A > D_norho_k${k}_f$f.txt 2>&1; done; done
touch DONEB
