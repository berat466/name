# Seul arazisi (tarayıcı, Three.js)

Çalıştırmak için:

```
cd game
python3 -m http.server 8000
```

Sonra tarayıcıda http://localhost:8000 adresini aç.

- `data/heights.f32`, `data/terrain.json`: SRTM (N37E126, N37E127) verisinden `tools/build_terrain.py` ile üretildi.
- `data/roads.json`: OpenStreetMap yolları, `tools/build_roads.py` ile üretilir.
- `textures/`: Poly Haven PBR dokuları (CC0), 2K.
