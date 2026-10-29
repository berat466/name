source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
$B $A /tmp/v53A_yeni 85 85 9000 > ham/C_k0.txt 2>&1
for k in 3 10 30; do V53_QKAP=$k V53_QPI=1 $B $A /tmp/v53A_yeni 85 85 9000 > ham/C_k${k}_kn.txt 2>&1; done
V53_QKAP=3 V53_QPI=0 $B $A /tmp/v53A_yeni 85 85 9000 > ham/C_k3_uni.txt 2>&1
touch ham/C_DONE
