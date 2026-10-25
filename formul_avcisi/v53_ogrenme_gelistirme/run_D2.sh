source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15 V53_PMIN=1 V53_QKAP=3 V53_QPI=1
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
V53_LAMP=35 $B $A /tmp/v53A_yeni 85 85 9000 > ham/D2_lp35_lf9000.txt 2>&1
V53_LAMP=70 $B $A /tmp/v53A_yeni 85 85 9000 > ham/D2_lp70_lf9000.txt 2>&1
V53_LAMP=50 $B $A /tmp/v53A_yeni 85 85 3000 > ham/D2_lp50_lf3000.txt 2>&1
V53_LAMP=50 $B $A /tmp/v53A_yeni 85 85 27000 > ham/D2_lp50_lf27000.txt 2>&1
touch ham/D2_DONE
