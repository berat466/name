#!/bin/bash
# dogrula.sh <etiket> <veri> : V78 <-> V80 hash + top-k + CE (V80_SHASH: s(K) tablo karmalari)
cd /home/user/name/formul_avcisi/v80_ortuk_akisli_k
et=$1; export DATA=$2; shift 2
./kos.sh ${et}_78 tek_hamle_v78 V53H_HASH=1 V76_DEBUG=1 | cut -c1-100
./kos.sh ${et}_80 tek_hamle_v80 V53H_HASH=1 V76_DEBUG=1 V80_SHASH=1 "$@" | cut -c1-100
for v in 78 80; do grep -E "V53H_HASH|V53H_SONUC|V76_DBG" ham/${et}_$v.txt | grep -v "T_ust\|lin e=" > ham/${et}_$v.cmp; done
echo "--- $et"; diff ham/${et}_78.cmp ham/${et}_80.cmp && echo "TUM HASH/CE/TOPK AYNI"
