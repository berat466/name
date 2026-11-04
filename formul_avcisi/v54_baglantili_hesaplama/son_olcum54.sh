#!/bin/bash
cd "$(dirname "$0")"
rm -f ham/dogr54_DONE ham/ab54/DONE ham/olcek3/DONE ham/abl54/DONE ham/test_son54_DONE
./dogrula54.sh
for cfg in full hafif; do source env_$cfg.sh; V53H_LEAN=1 V53H_FAST=1 V53H_TEST=1 ../cpp/tek_hamle_v54 $ARGS > ham/test_son54_$cfg.txt 2>&1; done; touch ham/test_son54_DONE
./ab54.sh
./olcek2.sh ../cpp/tek_hamle_v54 full ham/olcek3
./olcek2.sh ../cpp/tek_hamle_v54 hafif ham/olcek3
rm -rf ham/abl54; ./abl54.sh
touch ham/SON_DONE
