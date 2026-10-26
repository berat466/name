#!/bin/bash
# V76 dogrulama: (1) hizli-yol bayraklari, (2) V73 referans karmalari (AFF/RF/ACF/X/U/WF), (3) V75 ile CE/PPL/Top-k, (4) skaler<->AVX 15 hane
cd /home/user/name/formul_avcisi/v76_sadelestirme
./kos.sh dg_hash tek_hamle_v76 V53H_HASH=1 V76_DEBUG=1 >/dev/null
echo "bayraklar: $(grep V76_YOL ham/dg_hash.txt)"
for k in AFF RF ACF X U WF; do echo -n "$k=$(grep -E "HASH\|$k\|" ham/dg_hash.txt | cut -d'|' -f3) "; done; echo
echo "beklenen(V73): AFF=26401df5ea0b1dea RF=18b4555f8bd71339 ACF=8ea5f941b27af912 X=e2858fa1ad353f51 U=bc55ec1d32364d8d WF=d54c462f219c825b"
./kos.sh dg_leg tek_hamle_v76 V76_VEC=0 V76_CH=2048 V76_DEBUG=1 >/dev/null
diff <(grep V76_DBG ham/dg_hash.txt | grep -v "lin e=") <(grep V76_DBG ham/dg_leg.txt | grep -v "lin e=") && echo "alfa/test CE(15 hane)+top1/top5: skaler(V75 kodu) == AVX-512 ESIT"
grep -E "V53H_SONUC" ham/dg_hash.txt
