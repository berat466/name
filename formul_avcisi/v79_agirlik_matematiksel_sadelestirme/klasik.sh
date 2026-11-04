#!/bin/bash
# klasik (hizli yol bayraklari kapali) agirlik uretimi: V76_LEGACY=1, hicbir V6x/V7x bayragi yok
cd /home/user/name/formul_avcisi/v79_agirlik_matematiksel_sadelestirme
cd ../v54_baglantili_hesaplama; source env_hafif.sh
A=$(echo "$ARGS" | sed "s#/tmp/v53A_yeni#/tmp/v59_10001#")
env V76_LEGACY=1 V53H_TEST=1 timeout 600 python3 calistir.py ../v79_agirlik_matematiksel_sadelestirme/ham/klasik ../cpp/tek_hamle_v78 $A >/dev/null 2>&1
echo BITTI > ../v79_agirlik_matematiksel_sadelestirme/ham/klasik_bitti.txt
