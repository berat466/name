#!/bin/bash
# Bolum 02 sabotaj (test-testi) kosucusu: bkz. sabotaj_ifade.py
D=$(cd "$(dirname "$0")" && pwd); exec python3 $D/sabotaj_ifade.py "$@"
