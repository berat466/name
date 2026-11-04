#!/bin/bash
cd /home/user/name/formul_avcisi/v80_ortuk_akisli_k
export DATA=/tmp/v76_20001
./kos.sh s20k_78 tek_hamle_v78 V76_DEBUG=1 | cut -c1-120
./kos.sh s20k_80 tek_hamle_v80 V76_DEBUG=1 V80_SHASH=1 | cut -c1-120
for v in 78 80; do grep -E "V53H_SONUC|V76_DBG" ham/s20k_$v.txt | grep -v "lin e=" | sed 's/|CE=[^|]*//; s/CE=[0-9.]* //' > ham/s20k_$v.cmp; done
diff ham/s20k_78.cmp ham/s20k_80.cmp && echo "20k TOPK/SECIM AYNI"
echo BITTI > ham/s20k_bitti.txt
