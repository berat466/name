# Seul arazisi (tarayıcı, Three.js)

Çalıştırmak için:

```
cd game
python3 -m http.server 8000
```

Sonra tarayıcıda http://localhost:8000 adresini aç.

- `data/heights.f32`, `data/terrain.json`: SRTM (N37E126, N37E127) verisinden `tools/build_terrain.py` ile üretildi.
- `data/roads.json`: Seul'ün gerçek yol ağı (69.109 yol). Kaynak: BBBike Seul OSM özeti
  (https://download.bbbike.org/osm/bbbike/Seoul/Seoul.osm.pbf), `tools/build_roads.py` ile üretildi.
  Yol verisi © OpenStreetMap katkıda bulunanlar, ODbL lisansı.
- Yol genişlikleri Avrupa / GTA 5 ölçeğinde: otoban şeridi 3,75 m + 3 m emniyet şeridi,
  ana cadde şeridi 3,5 m + 4 m kaldırım, mahalle sokağı 2 × 3 m + 2,5 m kaldırım.
- `textures/`: Poly Haven PBR dokuları (CC0), 2K.
