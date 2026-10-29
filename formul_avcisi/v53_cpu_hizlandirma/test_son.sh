#!/bin/bash
# YALNIZ nihai dogrulama: hizli-tam yol ile test metrikleri (orijinal v53 ile karsilastirmak icin)
cd "$(dirname "$0")"
for cfg in full hafif; do source env_$cfg.sh; V53H_LEAN=1 V53H_FAST=1 V53H_U=0 V53H_TEST=1 ../cpp/tek_hamle_v53h $ARGS > ham/test_son_$cfg.txt 2>&1; done
touch ham/test_son_DONE
