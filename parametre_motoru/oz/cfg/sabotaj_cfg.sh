#!/bin/bash
D=$(cd "$(dirname "$0")" && pwd); exec python3 $D/sabotaj_cfg.py "$@"
