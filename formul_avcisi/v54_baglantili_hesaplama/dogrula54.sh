#!/bin/bash
# V54 dogrulama: (o) V53 orijinal aritmetik yol [tek_hamle_v53h LEAN], (s) V54 siki mod (tum V54 secenekleri kapali, dgemm U), (d) V54 varsayilan
# her biri DUMP+HASH; orijinal v53 ikilisinin V33_DUMP agirliklari (/tmp/href_*) ile cmp; fark.py ile dizi dizi fark
cd "$(dirname "$0")"
for cfg in full hafif; do source env_$cfg.sh
  for m in o s d; do rm -rf /tmp/d54_${cfg}_$m; mkdir -p /tmp/d54_${cfg}_$m; done
  V53H_LEAN=1 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/d54_${cfg}_o ../cpp/tek_hamle_v53h $ARGS > ham/dogr54_${cfg}_v53_orijinal_yol.txt 2>&1
  V54_GATE=0 V54_TOPK=0 V54_REUSE=0 V54_TANH=0 V53H_U=0 V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/d54_${cfg}_s ../cpp/tek_hamle_v54 $ARGS > ham/dogr54_${cfg}_v54_siki.txt 2>&1
  V53H_LEAN=1 V53H_FAST=1 V53H_TEST=0 V53H_HASH=1 V53H_DUMP=/tmp/d54_${cfg}_d ../cpp/tek_hamle_v54 $ARGS > ham/dogr54_${cfg}_v54_varsayilan.txt 2>&1
  { echo "=== $cfg : orijinal v53 ikilisi (V33_DUMP) ile cmp"; for m in o s d; do for w in WC WF; do if cmp -s /tmp/d54_${cfg}_$m/$w.bin /tmp/href_${cfg}_$w.bin; then echo "$m $w: BIT-OZDES"; else echo "$m $w: FARKLI"; fi; done; done
    echo "--- V53 orijinal yol vs V54 siki"; python3 fark.py /tmp/d54_${cfg}_o /tmp/d54_${cfg}_s
    echo "--- V53 orijinal yol vs V54 varsayilan"; python3 fark.py /tmp/d54_${cfg}_o /tmp/d54_${cfg}_d; } > ham/dogr54_${cfg}_ozet.txt 2>&1
done
touch ham/dogr54_DONE
