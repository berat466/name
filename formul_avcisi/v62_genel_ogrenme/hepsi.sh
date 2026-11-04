#!/bin/bash
cd /home/user/name/formul_avcisi/v62_genel_ogrenme
python3 grid.py /tmp/v62_bc > grid_bc.txt 2>&1
python3 grid.py /tmp/v62_wine > grid_wine.txt 2>&1
echo bitti > grid_bitti.txt
