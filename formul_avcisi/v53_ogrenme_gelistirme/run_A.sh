source ../v51_ek_deneyler_ham/env.sh
export V51_OOFPOOL=1 V51_GP=0 V51_GF=0.15
B=../cpp/tek_hamle_v53; A="2 512 1 1 1 0.5 Z 1 Y"
for d in v8data:ref v53A_eski:A_eski v53A_yeni:A_yeni; do dir=/tmp/${d%%:*}; n=${d##*:}
  $B $A $dir 85 85 3000 > ham/${n}.txt 2>&1; done
touch ham/A_DONE
