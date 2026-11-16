# Açık dünya (tarayıcı, Three.js)

Çalıştırmak için:

```
cd game
python3 -m http.server 8000
```

Sonra tarayıcıda http://localhost:8000 adresini aç.

Harita verisi `data/` klasöründe durmalı (`heights.f32`, `terrain.json`, `roads.json`, `water.json`).
Bunlar `tools/build_terrain.py`, `tools/build_roads.py` ve `tools/build_water.py` ile üretilir.
Yol verisi © OpenStreetMap katkıda bulunanlar (ODbL). Dokular Poly Haven (CC0).

## Kontroller
- **Telefon:** uçuş modu. Sol yarıya bas ve sürükle → ilerle; sağ yarıyı sürükle → etrafına bak; ▲ ▼ → yüksel / alçal;
  yuvarlak düğme → hız; sağ üstteki düğme → harita modu. Telefonda doku 1K (2K için `?q=2k`).
- **Bilgisayar:** harita modu (fare). ✈ düğmesi uçuş moduna geçirir (WASD + fare, Q/E, Shift).
