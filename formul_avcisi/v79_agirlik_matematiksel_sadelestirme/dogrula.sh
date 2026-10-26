#!/bin/bash
# dogrula.sh <etiket> <veri> : V78 <-> V79 hash + top-k + CE
cd /home/user/name/formul_avcisi/v79_agirlik_matematiksel_sadelestirme
et=$1; export DATA=$2
./kos.sh ${et}_78 tek_hamle_v78 V53H_HASH=1 V76_DEBUG=1 | cut -c1-100
./kos.sh ${et}_79 tek_hamle_v79 V53H_HASH=1 V76_DEBUG=1 | cut -c1-100
for v in 78 79; do grep -E "V53H_HASH|V53H_SONUC|V76_DBG" ham/${et}_$v.txt | grep -v "T_ust\|lin e=" > ham/${et}_$v.cmp; done
echo "--- $et"; diff ham/${et}_78.cmp ham/${et}_79.cmp && echo "TUM HASH/CE/TOPK AYNI"
