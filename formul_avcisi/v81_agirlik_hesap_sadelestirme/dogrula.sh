#!/bin/bash
# dogrula.sh <etiket> <veri> : V80 <-> V81 hash + top-k + CE
cd /home/user/name/formul_avcisi/v81_agirlik_hesap_sadelestirme
et=$1; export DATA=$2; shift 2
./kos.sh ${et}_80 tek_hamle_v80 V53H_HASH=1 V76_DEBUG=1 V80_SHASH=1 V78_AUTO=0 | cut -c1-100
./kos.sh ${et}_81 tek_hamle_v81 V53H_HASH=1 V76_DEBUG=1 V80_SHASH=1 V78_AUTO=0 "$@" | cut -c1-100
for v in 80 81; do grep -E "V53H_HASH|V53H_SONUC|V76_DBG|V80_SHASH" ham/${et}_$v.txt | grep -v "T_ust\|lin e=" > ham/${et}_$v.cmp; done
echo "--- $et"; diff ham/${et}_80.cmp ham/${et}_81.cmp && echo "TUM HASH/CE/TOPK AYNI"
