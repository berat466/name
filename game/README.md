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

## Kontroller
- **Telefon:** uçuş modu. Sol yarıya bas ve sürükle → ilerle (joystick); sağ yarıyı sürükle → etrafına bak;
  ▲ ▼ → yüksel / alçal; yuvarlak hız düğmesi → 15 / 60 / 200 / 600 m/s; sağ üstteki düğme → harita moduna geç.
  Telefonda doku boyutu 1K ve kenar yumuşatma kapalı (bellek ve hız için). 2K için adrese `?q=2k` ekle.
- **Bilgisayar:** harita modu (fare). Sağ üstteki ✈ düğmesi uçuş moduna geçirir (WASD + fare sürükle, Q/E alçal-yüksel, Shift hızlı).
