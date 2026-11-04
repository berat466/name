source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
for r in 1 2; do
 $B $A /tmp/v8data 85 85 3000 > ham/T_ref_$r.txt 2>&1
 V53_PMIN=1 V53_QKAP=3 V53_QPI=1 V53_LAMP=50 $B $A /tmp/v53A_yeni 85 85 3000 > ham/T_final_$r.txt 2>&1
done
touch ham/T_DONE
