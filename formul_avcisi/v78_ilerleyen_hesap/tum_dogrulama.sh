#!/bin/bash
cd /home/user/name/formul_avcisi/v78_ilerleyen_hesap
./olcek.sh s2k /tmp/v59_2001 1 > ham/olcek_2k.log 2>&1
./olcek.sh s5k /tmp/v59_5001 1 > ham/olcek_5k.log 2>&1
./olcek.sh s20k /tmp/v76_20001 0 > ham/olcek_20k.log 2>&1
N=7 ./ab.sh > ham/ab_cikti.txt 2>&1
echo BITTI > ham/tum_bitti.txt
