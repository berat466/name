import * as THREE from 'three';
import { MapControls } from 'three/addons/controls/MapControls.js';

// ---------------------------------------------------------------------------
// Sahne
// ---------------------------------------------------------------------------
const renderer = new THREE.WebGLRenderer({ antialias: true });
renderer.setPixelRatio(Math.min(devicePixelRatio, 2));
renderer.setSize(innerWidth, innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.0;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
const SKY = new THREE.Color(0x9fb7cc);
scene.background = SKY;
scene.fog = new THREE.FogExp2(SKY, 0.000045);

const camera = new THREE.PerspectiveCamera(55, innerWidth / innerHeight, 2, 80000);

scene.add(new THREE.HemisphereLight(0xcfe3ff, 0x4a4030, 0.9));
const sun = new THREE.DirectionalLight(0xfff1dc, 2.6);
sun.position.set(-0.5, 0.8, 0.35).multiplyScalar(10000);
scene.add(sun);

const controls = new MapControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.maxPolarAngle = Math.PI * 0.49;
controls.minDistance = 20;
controls.maxDistance = 45000;
controls.screenSpacePanning = false;
controls.zoomToCursor = true;

// ---------------------------------------------------------------------------
// Dokular (Poly Haven, CC0, 2K) — birden çok PBR seti doku dizisi (texture array) olarak
// ---------------------------------------------------------------------------
const maxAniso = renderer.capabilities.getMaxAnisotropy();
const TEX = 2048;

async function loadImg(url) {
  const img = new Image();
  img.src = url;
  await img.decode();
  return img;
}
// Her set için iki katman üretir:
//   albedo  : diff.jpg (sRGB)
//   nra     : R,G = normal.xy · B = pürüzlülük · A = ortam kapatma (AO)
async function loadPbrArray(names) {
  const n = names.length, layer = TEX * TEX * 4;
  const alb = new Uint8Array(layer * n), nra = new Uint8Array(layer * n);
  const cv = document.createElement('canvas');
  cv.width = cv.height = TEX;
  const ctx = cv.getContext('2d', { willReadFrequently: true });
  const read = (img) => { ctx.drawImage(img, 0, 0, TEX, TEX); return ctx.getImageData(0, 0, TEX, TEX).data; };

  // tüm görselleri aynı anda indir ve çöz (sırayla beklemek açılışı uzatıyordu)
  const loading = names.map((nm0) => Promise.all(['diff', 'nor_gl', 'arm'].map((m) => loadImg(`./textures/${nm0}/${m}.jpg`))));
  for (let i = 0; i < n; i++) {
    const [d, nm, arm] = await loading[i];
    alb.set(read(d), layer * i);
    const N = read(nm), A = read(arm), o = layer * i;
    for (let p = 0; p < layer; p += 4) {
      nra[o + p] = N[p]; nra[o + p + 1] = N[p + 1]; nra[o + p + 2] = A[p + 1]; nra[o + p + 3] = A[p];
    }
  }
  const mk = (data, srgb) => {
    const t = new THREE.DataArrayTexture(data, TEX, TEX, n);
    t.wrapS = t.wrapT = THREE.RepeatWrapping;
    t.minFilter = THREE.LinearMipmapLinearFilter;
    t.magFilter = THREE.LinearFilter;
    t.generateMipmaps = true;
    t.anisotropy = maxAniso;
    if (srgb) t.colorSpace = THREE.SRGBColorSpace;
    t.needsUpdate = true;
    return t;
  };
  return { alb: mk(alb, true), nra: mk(nra, false) };
}

// Arazi katmanları (sıra shader ile aynı olmalı)
const TERRAIN_SETS = [
  'forrest_ground_01', // 0 çim
  'aerial_grass_rock', // 1 çim + taş
  'dirt_floor',        // 2 toprak lekeleri
  'forest_leaves_02',  // 3 orman zemini
  'aerial_rocks_02',   // 4 kaya
  'mossy_rock',        // 5 yosunlu kaya
];
// Yol katmanları
const ROAD_SETS = ['asphalt_02', 'aerial_asphalt_01', 'asphalt_04'];
// Kaldırım katmanları
const WALK_SETS = ['herringbone_pavement_04', 'concrete_pavement_02'];

const [terrainTex, roadTex, walkTex] = await Promise.all(
  [TERRAIN_SETS, ROAD_SETS, WALK_SETS].map(loadPbrArray));

// Ortak GLSL: gürültü, iki ölçekli örnekleme, nra çözme
const GLSL_COMMON = `
  float hash(vec2 p){ vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
  float vnoise(vec2 p){
    vec2 i = floor(p), f = fract(p); f = f*f*(3.0-2.0*f);
    return mix(mix(hash(i), hash(i+vec2(1,0)), f.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y);
  }
  // Tekrar desenini kırmak için aynı katmanı iki ölçekte örnekle
  vec4 smp(highp sampler2DArray t, vec2 uv, float l){
    return mix(texture(t, vec3(uv, l)), texture(t, vec3(uv * 0.137 + 0.31, l)), 0.35);
  }
  vec3 accA; vec4 accN; float accW;
  void addLayer(highp sampler2DArray A, highp sampler2DArray N, vec2 uv, float l, float w){
    if (w < 0.004) return;
    accA += smp(A, uv, l).rgb * w;
    accN += smp(N, uv, l) * w;
    accW += w;
  }
`;
// Ortak parça: birikmiş katmanlardan normal / pürüzlülük / AO çıkar (dünya uzayı)
const GLSL_RESOLVE = `
  accA /= accW; accN /= accW;
  vec2 nxy = accN.xy * 2.0 - 1.0;
  nxy.y = -nxy.y; // doku dizisi Y çevrilmeden yüklendi
  vec3 tn = vec3(nxy, sqrt(max(0.0, 1.0 - dot(nxy, nxy))));
  blendR = accN.z; blendAO = accN.w;
`;

let shaderId = 0;
function patchStandard(mat, uniforms, vtxExtra, fragDecl, fragBody) {
  // her malzeme kendi shader'ını derlesin (aksi halde three.js aynı programı paylaştırır)
  const key = 'patched-' + (shaderId++);
  mat.customProgramCacheKey = () => key;
  mat.onBeforeCompile = (sh) => {
    Object.assign(sh.uniforms, uniforms);
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec3 vWP;\nvarying vec3 vWN;\n' + (vtxExtra.decl || ''))
      .replace('#include <project_vertex>', `#include <project_vertex>
        vWP = (modelMatrix * vec4(transformed, 1.0)).xyz;
        vWN = normalize(mat3(modelMatrix) * objectNormal);
        ${vtxExtra.body || ''}`);
    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', `#include <common>
        varying vec3 vWP;
        varying vec3 vWN;
        uniform highp sampler2DArray tA, tN;
        vec3 blendN; float blendR, blendAO;
        ${GLSL_COMMON}
        ${fragDecl}`)
      .replace('#include <map_fragment>', fragBody)
      .replace('#include <roughnessmap_fragment>', 'float roughnessFactor = roughness * blendR;')
      .replace('#include <normal_fragment_maps>', 'normal = normalize((viewMatrix * vec4(blendN, 0.0)).xyz);')
      .replace('#include <aomap_fragment>', 'reflectedLight.indirectDiffuse *= mix(1.0, blendAO, 0.8);');
  };
}

// ---------------------------------------------------------------------------
// Arazi malzemesi: eğim, yükseklik ve gürültüye göre 6 PBR setinin karışımı
// ---------------------------------------------------------------------------
function terrainMaterial() {
  const mat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0,
    polygonOffset: true, polygonOffsetFactor: 2, polygonOffsetUnits: 4 });  // arazi yolun gerisinde: taşma olmaz
  patchStandard(mat, { tA: { value: terrainTex.alb }, tN: { value: terrainTex.nra } }, {}, '', `
    vec3 N0 = normalize(vWN);
    float slope = 1.0 - N0.y;
    float n1 = vnoise(vWP.xz * 0.004) * 0.6 + vnoise(vWP.xz * 0.02) * 0.4;
    float n2 = vnoise(vWP.xz * 0.011 + 17.0);
    float n3 = vnoise(vWP.xz * 0.012 + 91.0) * 0.7 + vnoise(vWP.xz * 0.06 + 5.0) * 0.3;

    // bölgeler
    float rock = smoothstep(0.30, 0.50, slope + (n1 - 0.5) * 0.15);
    float forest = smoothstep(55.0, 110.0, vWP.y + (n1 - 0.5) * 70.0) * (1.0 - rock);
    forest = max(forest, smoothstep(0.10, 0.22, slope) * (1.0 - rock) * 0.9);
    float grass = max(0.0, 1.0 - rock - forest);

    // bölge içi set karışımı
    float gB = smoothstep(0.35, 0.65, n2);            // çim A <-> çim+taş
    float dirt = smoothstep(0.58, 0.85, n3) * 0.55;   // toprak lekeleri (yumuşak geçiş)
    float rB = smoothstep(0.4, 0.6, n2);              // kaya <-> yosunlu kaya
    float fB = smoothstep(0.45, 0.75, n3) * 0.5;      // orman zemininde çimli açıklıklar

    float w0 = grass * (1.0 - gB) * (1.0 - dirt);
    float w1 = grass * gB * (1.0 - dirt) + forest * fB;
    float w2 = grass * dirt;
    float w3 = forest * (1.0 - fB);
    float w4 = rock * (1.0 - rB);
    float w5 = rock * rB;

    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vWP.xz / 5.0, 0.0, w0);
    addLayer(tA, tN, vWP.xz / 16.0, 1.0, w1);
    addLayer(tA, tN, vWP.xz / 4.0, 2.0, w2);
    addLayer(tA, tN, vWP.xz / 3.0, 3.0, w3);
    addLayer(tA, tN, vWP.xz / 14.0, 4.0, w4);
    addLayer(tA, tN, vWP.xz / 6.0, 5.0, w5);
    ${GLSL_RESOLVE}

    vec3 col = accA;
    col *= mix(vec3(1.0), vec3(0.85, 1.0, 0.7), grass);                // çimi biraz canlandır
    col = mix(col, vec3(0.10, 0.16, 0.06), forest * 0.45);              // yukarıdan ağaç örtüsü
    col *= 0.85 + 0.3 * vnoise(vWP.xz * 0.0015);                        // makro varyasyon
    diffuseColor.rgb *= col;

    vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
    vec3 Bt = normalize(cross(Tg, N0));
    blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Yol malzemesi: 3 asfalt PBR seti + lastik izi, yağ lekesi, kenar kiri + şerit çizgileri + yaya geçidi
//   ruv    = (yol boyunca metre, enine metre; sol kenar = -yarı genişlik)
//   rdir   = yol yönü (x, z)
//   rinfo  = (şerit sayısı, tek yön mü, şerit genişliği, kavşağa uzaklık)
//   rinfo2 = (sol omuz, sağ omuz, yarı genişlik, yaya geçidi var mı)
// ---------------------------------------------------------------------------
const ROAD_VTX = {
  decl: `attribute vec2 ruv; attribute vec2 rdir; attribute vec4 rinfo; attribute vec4 rinfo2;
    varying vec2 vRuv; varying vec2 vRdir; varying vec4 vInfo; varying vec4 vInfo2;`,
  body: 'vRuv = ruv; vRdir = rdir; vInfo = rinfo; vInfo2 = rinfo2;',
};
const ROAD_FRAG_DECL = 'varying vec2 vRuv; varying vec2 vRdir; varying vec4 vInfo; varying vec4 vInfo2;';
// yol boyunca teğet uzay (Tg = boyuna, Bt = enine)
const GLSL_ROAD_TBN = `
  vec3 N0 = normalize(vWN);
  vec3 Tg = normalize(vec3(vRdir.x, 0.0, vRdir.y));
  vec3 Bt = normalize(cross(N0, Tg));
  blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
`;

function roadMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL + `
    float AA;  // türev döngü dışında hesaplanır (döngü içinde tanımsız olur)
    float aaLine(float d, float w){
      float cov = clamp(w / AA, 0.0, 1.0);  // piksel altı çizgiyi soldur, titremesin
      return (1.0 - smoothstep(max(0.0, w - AA), w + AA, abs(d))) * cov;
    }
    float band(float d, float w, float soft){ return 1.0 - smoothstep(w, w + soft, abs(d)); }
  `, `
    float along = vRuv.x, u = vRuv.y;
    float lanes = vInfo.x, oneway = vInfo.y, lw = vInfo.z, jd = vInfo.w;
    float shL = vInfo2.x, shR = vInfo2.y, halfW = vInfo2.z, zebra = vInfo2.w;
    AA = fwidth(u) + 0.005;

    // --- 3 asfalt PBR seti ---
    float n1 = vnoise(vWP.xz * 0.02);
    float n2 = vnoise(vWP.xz * 0.07 + 33.0);
    float wB = smoothstep(0.4, 0.8, n1) * 0.6;     // farklı asfalt yüzeyi
    float wC = smoothstep(0.55, 0.85, n2) * 0.5;   // çatlak / yama
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 5.0, 0.0, (1.0 - wB) * (1.0 - wC));
    addLayer(tA, tN, vRuv / 8.0, 1.0, wB * (1.0 - wC));
    addLayer(tA, tN, vRuv / 4.0, 2.0, wC);
    ${GLSL_RESOLVE}
    vec3 col = accA * vec3(0.62, 0.63, 0.66);      // koyu, hafif mavimsi şehir asfaltı

    // carriageway (omuzlar hariç) ve şerit merkezleri
    float cL = -halfW + shL, cR = halfW - shR;
    float laneF = (u - cL) / lw;                    // soldan kaçıncı şerit (kesirli)
    float inLane = fract(laneF) - 0.5;              // şerit içi konum (-0.5..0.5)
    float onCarriage = step(cL, u) * step(u, cR);

    // lastik izleri: her şeritte iki tekerlek yolu, daha koyu ve daha az pürüzlü
    float tire = (band(inLane * lw - 0.85, 0.25, 0.35) + band(inLane * lw + 0.85, 0.25, 0.35)) * onCarriage;
    tire *= 0.6 + 0.4 * vnoise(vec2(along * 0.05, laneF * 3.0));
    // yağ lekesi: şerit ortasında, kesik kesik
    float oil = band(inLane * lw, 0.35, 0.4) * smoothstep(0.55, 0.8, vnoise(vec2(along * 0.12, floor(laneF) * 7.0))) * onCarriage;
    // kenar kiri: bordüre yakın birikmiş kir
    float edge = 1.0 - smoothstep(0.0, 0.9, min(u + halfW, halfW - u));
    col *= 1.0 - 0.18 * tire - 0.25 * oil;
    col = mix(col, col * vec3(0.85, 0.8, 0.72), edge * 0.6);
    blendR = clamp(blendR - 0.12 * tire - 0.25 * oil, 0.2, 1.0);

    // --- şerit çizgileri (orta sarı çift çizgi, beyaz kesikli şerit, beyaz kenar çizgisi) ---
    float dash = step(fract(along / 15.0), 0.4);     // 6 m çizgi, 9 m boşluk
    float white = max(aaLine(u - cL, 0.10), aaLine(u - cR, 0.10)) * step(0.2, min(shL, shR) + 0.2);
    float yellow = 0.0;
    if (oneway < 0.5) {
      float mid = (cL + cR) * 0.5;
      yellow = max(aaLine(u - mid - 0.15, 0.08), aaLine(u - mid + 0.15, 0.08));
      float side = max(1.0, floor(lanes / 2.0));
      float slw = (cR - cL) * 0.5 / side;
      for (int k = 1; k < 5; k++) {
        if (float(k) >= side) break;
        white = max(white, max(aaLine(u - mid - slw * float(k), 0.08), aaLine(u - mid + slw * float(k), 0.08)) * dash);
      }
    } else {
      for (int k = 1; k < 10; k++) {
        if (float(k) >= lanes) break;
        white = max(white, aaLine(u - cL - lw * float(k), 0.08) * dash);
      }
    }
    float wear = mix(0.65, 1.0, vnoise(vec2(along * 0.25, 7.0)));
    float fade = smoothstep(0.0, 4.0, jd);           // kavşak içinde çizgi yok
    white *= fade * wear; yellow *= fade * wear;

    // yaya geçidi (zebra): kavşağa 1-5 m uzaklıkta, 0.5 m beyaz / 0.5 m boşluk
    float zb = zebra * step(1.0, jd) * step(jd, 5.0) * step(0.5, fract(u / 1.0)) * onCarriage;
    white = max(white, zb * wear);

    col = mix(col, vec3(0.8), white);
    col = mix(col, vec3(0.85, 0.6, 0.07), yellow);
    blendR = mix(blendR, 0.5, max(white, yellow));
    diffuseColor.rgb *= col;
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// Kaldırım / bordür / bariyer malzemesi: 2 PBR seti (parke taşı, beton)
function walkMaterial() {
  const mat = new THREE.MeshStandardMaterial({
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  patchStandard(mat, { tA: { value: walkTex.alb }, tN: { value: walkTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL, `
    float layer = vInfo.x;  // 0 = parke, 1 = beton
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / (layer < 0.5 ? 3.0 : 2.0), layer < 0.5 ? 0.0 : 1.0, 1.0);
    ${GLSL_RESOLVE}
    diffuseColor.rgb *= accA * (0.85 + 0.25 * vnoise(vWP.xz * 0.05));
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Yükseklik verisi
// ---------------------------------------------------------------------------
const meta = await (await fetch('./data/terrain.json')).json();
const H = new Float32Array(await (await fetch('./data/heights.f32')).arrayBuffer());
const { cols, rows, width: Wm, depth: Dm } = meta;
const dx = Wm / (cols - 1), dz = Dm / (rows - 1);

const hAt = (c, r) => H[Math.min(rows - 1, Math.max(0, r)) * cols + Math.min(cols - 1, Math.max(0, c))];
// Dünya koordinatında (x, z) çift doğrusal yükseklik
export function heightAt(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  return (hAt(c, r) * (1 - u) + hAt(c + 1, r) * u) * (1 - v) + (hAt(c, r + 1) * (1 - u) + hAt(c + 1, r + 1) * u) * v;
}

// ---------------------------------------------------------------------------
// Yollar (OpenStreetMap, gerçek Seul yol ağı)
//   1) her yolu örnekle, araziye göre eğim sınırlı profil çıkar
//   2) kavşakları bul, yolları kavşakta aynı yükseklikte buluştur
//   3) araziyi yola oturt (yarma / dolgu + şev)
//   4) görüntüyü kameranın çevresinde akış halinde üret (GTA tarzı streaming)
// ---------------------------------------------------------------------------
const roadData = await fetch('./data/roads.json').then((r) => (r.ok ? r.json() : null)).catch(() => null);
const ROAD_ORDER = { residential: 0, unclassified: 0, tertiary: 1, secondary: 2, primary: 3, trunk: 4, motorway: 5 };
const MINOR = new Set(['residential', 'unclassified', 'tertiary']);
// sınıf -> [örnekleme aralığı m, profil yumuşatma (örnek), en fazla eğim]
const PROFILE = {
  motorway: [6], trunk: [6], primary: [6], secondary: [8],
  tertiary: [10], unclassified: [10], residential: [10],
};
const PIECE = 48;          // parça başına en fazla nokta
const TILE = 1500;         // akış karosu (m)
const CURB_H = 0.15, SKIRT = 2.0, LIFT = 0.12;   // yol, arazinin 12 cm üstünde (z-çakışması olmasın)
const smooth = (a, b, x) => { const t = Math.min(1, Math.max(0, (x - a) / (b - a))); return t * t * (3 - 2 * t); };

const preparedRoads = [];
const roadTileMap = new Map();   // "tx,tz" -> { pieces: [], built: { major, minor } }

function prepareRoads() {
  if (!roadData) return;
  // Kavşaklar: birden fazla yolun paylaştığı OSM noktaları
  const jkey = (x, z) => `${Math.round(x * 2)},${Math.round(z * 2)}`;
  const jmap = new Map();
  for (const road of roadData.roads) {
    const reach = road.w / 2 + road.s;
    for (let i = 0; i < road.p.length; i += 2) {
      const k = jkey(road.p[i], road.p[i + 1]);
      let e = jmap.get(k);
      if (!e) jmap.set(k, (e = { n: 0, mid: 0, r: 0, o: 0, h: heightAt(road.p[i], road.p[i + 1]) }));
      e.n++; e.r = Math.max(e.r, reach); e.o = Math.max(e.o, ROAD_ORDER[road.c]);
      if (i > 0 && i < road.p.length - 2) e.mid++;   // yolun ortasından geçiyor
    }
  }

  for (const road of roadData.roads) {
    const [STEP] = PROFILE[road.c];
    const p = road.p;
    const xs = [], zs = [], junctions = [];
    for (let i = 0; i < p.length; i += 2) {
      const e = jmap.get(jkey(p[i], p[i + 1]));
      // gerçek kavşak: 3+ yol uçu ya da bir yol başka yolun ortasından geçiyor.
      // İki yolun yalnızca uç uca eklendiği nokta (OSM'in yolu bölmesi) kavşak değildir.
      if (e && (e.n > 2 || (e.n === 2 && e.mid > 0))) junctions.push([xs.length, e.r + 1, e]);
      if (i >= p.length - 2) { xs.push(p[i]); zs.push(p[i + 1]); break; }
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / STEP));
      for (let s = 0; s < n; s++) { xs.push(ax + (bx - ax) * s / n); zs.push(az + (bz - az) * s / n); }
    }
    const N = xs.length;
    if (N < 2) continue;
    const dist = new Float32Array(N);
    for (let i = 1; i < N; i++) dist[i] = dist[i - 1] + Math.hypot(xs[i] - xs[i - 1], zs[i] - zs[i - 1]);

    // Arazi olduğu gibi kalır: yol arazinin yüksekliğini izler (yarma/dolgu/düzleme yok).
    // Aynı noktada buluşan yollar aynı arazi yüksekliğini okuduğu için kavşakta zaten eşit olur.
    const raw = xs.map((x, i) => heightMesh(x, zs[i]));
    let ys = raw.map((y) => y + LIFT);
    if (road.b) {
      // köprü: uçlar arasında düz tabliye, altındaki araziden en az 1 m yukarıda
      const y0 = ys[0], y1 = ys[N - 1];
      ys = ys.map((y, i) => Math.max(raw[i] + 1, y0 + (y1 - y0) * i / (N - 1)));
    }
    // kavşağa uzaklık (kavşak yarıçapı çıkarılmış); < 0 = kavşak içinde
    const jd = new Float32Array(N).fill(1e6);
    for (const [si, r] of junctions) {
      const d0 = dist[Math.min(si, N - 1)];
      for (let i = 0; i < N; i++) jd[i] = Math.min(jd[i], Math.abs(dist[i] - d0) - r);
    }
    // yön ve sağ vektör
    const dirx = new Float32Array(N), dirz = new Float32Array(N);
    for (let i = 0; i < N; i++) {
      const a = Math.max(0, i - 1), c = Math.min(N - 1, i + 1);
      let tx = xs[c] - xs[a], tz = zs[c] - zs[a];
      const l = Math.hypot(tx, tz) || 1;
      dirx[i] = tx / l; dirz[i] = tz / l;
    }
    const R = { road, N, xs: Float32Array.from(xs), zs: Float32Array.from(zs), ys: Float32Array.from(ys), dist, jd, dirx, dirz,
      zebra: !['motorway', 'trunk'].includes(road.c) && road.s > 0 ? 1 : 0 };
    preparedRoads.push(R);

    for (let a = 0; a < N - 1; a += PIECE - 1) {
      const e = Math.min(N, a + PIECE), m = (a + e) >> 1;
      const key = `${Math.floor(R.xs[m] / TILE)},${Math.floor(R.zs[m] / TILE)}`;
      let t = roadTileMap.get(key);
      if (!t) {
        const [tx, tz] = key.split(',').map(Number);
        roadTileMap.set(key, (t = { pieces: [], built: {}, cx: (tx + 0.5) * TILE, cz: (tz + 0.5) * TILE }));
      }
      t.pieces.push({ R, a, e });
    }
  }
}

class StripBuilder {
  constructor() { this.pos = []; this.nor = []; this.ruv = []; this.rdir = []; this.rinfo = []; this.rinfo2 = []; this.idx = []; }
  // n satır; row(j) -> [vA, vB], vertex = [x,y,z, nx,ny,nz, u,v]
  strip(n, row, dir, info, info2) {
    const base = this.pos.length / 3;
    for (let j = 0; j < n; j++) {
      const pair = row(j), d = dir(j), i1 = info(j);
      for (const v of pair) {
        this.pos.push(v[0], v[1], v[2]); this.nor.push(v[3], v[4], v[5]); this.ruv.push(v[6], v[7]);
        this.rdir.push(d[0], d[1]); this.rinfo.push(...i1); this.rinfo2.push(...info2);
      }
      if (j > 0) {
        const q = base + (j - 1) * 2;
        this.idx.push(q, q + 2, q + 1, q + 1, q + 2, q + 3);
      }
    }
  }
  mesh(mat) {
    if (!this.idx.length) return null;
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(this.pos, 3));
    g.setAttribute('normal', new THREE.Float32BufferAttribute(this.nor, 3));
    g.setAttribute('ruv', new THREE.Float32BufferAttribute(this.ruv, 2));
    g.setAttribute('rdir', new THREE.Float32BufferAttribute(this.rdir, 2));
    g.setAttribute('rinfo', new THREE.Float32BufferAttribute(this.rinfo, 4));
    g.setAttribute('rinfo2', new THREE.Float32BufferAttribute(this.rinfo2, 4));
    g.setIndex(this.pos.length / 3 > 65535 ? new THREE.Uint32BufferAttribute(this.idx, 1) : new THREE.Uint16BufferAttribute(this.idx, 1));
    g.computeBoundingSphere();
    const m = new THREE.Mesh(g, mat);
    m.renderOrder = 1;
    return m;
  }
}

// Bir yol parçasının (a..e) geometrisi. GTA / BeamNG yaklaşımı:
//  - zemindeki yol araziye oturur; her yan duvar araziye kadar iner (altı boş kalmaz)
//  - yükselen yerde beton istinat duvarı, köprüde kalın tabliye + korkuluk + ayaklar
function emitPiece(rb, wb, { R, a, e }) {
  const { road, xs, zs, ys, dist, jd, dirx, dirz } = R;
  const n = e - a, half = road.w / 2, up = [0, 1, 0], down = [0, -1, 0];
  const I = (j) => a + j;
  const px = (i, o) => xs[i] + dirz[i] * o, pz = (i, o) => zs[i] - dirx[i] * o;
  const at = (i, o, y, nrm, u, v) => [px(i, o), y, pz(i, o), nrm[0], nrm[1], nrm[2], u, v];
  const ground = (i, o) => heightMesh(px(i, o), pz(i, o));
  // duvar alt ucu: araziye kadar (biraz içine), en az 0.5 m
  const foot = (i, o, y) => Math.min(y - 0.5, ground(i, o) - 0.5);
  const dir = (j) => [dirx[I(j)], dirz[I(j)]];
  const info2 = [road.sl, road.sr, half, R.zebra];
  const C = () => [1, 0, 0, 0], P = () => [0, 0, 0, 0];
  const bridge = !!road.b, DECK = 1.4;
  // her enine noktada arazi yüksekliği (köprüde tabliye düz)
  // yol sınıfına göre 1,5 cm yükseklik farkı: kavşakta üst üste binen yollarda ana yol üstte kalır,
  // aynı düzlemde titreme (z-çakışması) olmaz
  const bias = ROAD_ORDER[road.c] * 0.015;
  const gy = (i, o) => (bridge ? ys[i] : ground(i, o) + LIFT + bias);
  // şev: kenardan (o, yTop) araziye doğru iner; arazi yukarıdaysa (yarma) yukarı çıkar
  const graded = (Wf, o, side, yTop) => {
    const outv = (i) => [dirz[i] * side, 0, -dirx[i] * side];
    const L = (i) => Math.min(14, Math.max(0.4, Math.abs(yTop(i) - ground(i, o + side * 3)) * 2));
    const STEPS = 3;
    for (let k = 0; k < STEPS; k++) {
      const t0 = k / STEPS, t1 = (k + 1) / STEPS;
      const yAt = (i, t) => {
        const l = L(i), g = ground(i, o + side * l * t);
        return yTop(i) + (g - 0.05 - yTop(i)) * (t * t * (3 - 2 * t));
      };
      Wf((i) => [
        at(i, o + side * L(i) * t0, yAt(i, t0), outv(i), dist[i], L(i) * t0),
        at(i, o + side * L(i) * t1, yAt(i, t1), outv(i), dist[i], L(i) * t1),
      ]);
    }
  };

  // asfalt
  const offs = [-half, -half / 3, half / 3, half];
  for (let k = 0; k < 3; k++) {
    const o0 = offs[k], o1 = offs[k + 1];
    rb.strip(n, (j) => { const i = I(j); return [at(i, o0, gy(i, o0), up, dist[i], o0), at(i, o1, gy(i, o1), up, dist[i], o1)]; },
      dir, (j) => [road.n, road.o, road.lw, jd[I(j)]], info2);
  }

  // kavşak dışındaki ardışık gruplar (kaldırım ve bariyer kavşakta kesilir)
  const runs = [];
  let cur = [];
  for (let j = 0; j < n; j++) {
    if (jd[I(j)] > 0 || bridge) cur.push(j);
    else { if (cur.length > 1) runs.push(cur); cur = []; }
  }
  if (cur.length > 1) runs.push(cur);
  const highway = road.c === 'motorway' || road.c === 'trunk';

  // kavşak içinde (kaldırım yok) asfalt kenarı da araziye kadar kapatılır
  if (!bridge) {
    const jruns = [];
    let jc = [];
    for (let j = 0; j < n; j++) {
      if (jd[I(j)] <= 0) jc.push(j);
      else { if (jc.length > 1) jruns.push(jc); jc = []; }
    }
    if (jc.length > 1) jruns.push(jc);
    for (const run of jruns) {
      const rn = run.length, RI = (k) => I(run[k]), rdir = (k) => [dirx[RI(k)], dirz[RI(k)]];
      for (const side of [-1, 1]) {
        const eo = half * side, nrm = (i) => [dirz[i] * side, 0, -dirx[i] * side];
        wb.strip(rn, (k) => { const i = RI(k);
          return [at(i, eo, gy(i, eo) - 0.02, nrm(i), dist[i], 0), at(i, eo, ground(i, eo) - 0.4, nrm(i), dist[i], -0.4)]; },
          rdir, C, info2);
      }
    }
  }

  for (const run of runs) {
    const rn = run.length, RI = (k) => I(run[k]), rdir = (k) => [dirx[RI(k)], dirz[RI(k)]];
    const W = (f, info = C) => wb.strip(rn, (k) => f(RI(k)), rdir, info, info2);
    let outerL = -half, outerR = half;   // tabliye / dolgu dış kenarı
    let topAt = (i) => ys[i];            // dış kenardaki üst yükseklik

    for (const side of [-1, 1]) {
      const out = (i) => [dirz[i] * side, 0, -dirx[i] * side];
      const inw = (i) => [-dirz[i] * side, 0, dirx[i] * side];
      const eo = half * side;
      let o = eo, yTop = (i) => gy(i, eo);
      if (road.s > 0) {
        // bordür yüzü + kaldırım (parke): kaldırım da arazi yüzeyini izler
        const oo = (half + road.s) * side; o = oo;
        yTop = (i) => gy(i, oo) + CURB_H;
        W((i) => [at(i, eo, ground(i, eo) - 0.4, inw(i), dist[i], -0.4), at(i, eo, gy(i, eo) + CURB_H, inw(i), dist[i], CURB_H)]);
        W((i) => [at(i, eo, gy(i, eo) + CURB_H, up, dist[i], eo), at(i, oo, yTop(i), up, dist[i], oo)], P);
      } else if (highway && side < 0 && road.o && !bridge) {
        // otoban orta bariyeri (New Jersey tipi beton, 0.9 m)
        const b1 = eo - 0.6, bh = 0.9, yb = (i) => gy(i, eo);
        W((i) => [at(i, eo, yb(i) - 0.05, inw(i), dist[i], 0), at(i, eo - 0.15, yb(i) + bh, inw(i), dist[i], bh)]);
        W((i) => [at(i, eo - 0.15, yb(i) + bh, up, dist[i], 0), at(i, b1 + 0.15, yb(i) + bh, up, dist[i], 0.3)]);
        W((i) => [at(i, b1 + 0.15, yb(i) + bh, out(i), dist[i], bh), at(i, b1, foot(i, b1, yb(i)), out(i), dist[i], -2)]);
        o = b1;
      } else {
        // beton omuz
        const oo = (half + 0.5) * side; o = oo;
        W((i) => [at(i, eo, gy(i, eo) - 0.02, up, dist[i], eo), at(i, oo, gy(i, oo) - 0.02, up, dist[i], oo)]);
        yTop = (i) => gy(i, oo) - 0.02;
      }
      if (side < 0) outerL = o; else outerR = o;
      topAt = yTop;

      if (bridge) {
        // köprü korkuluğu (beton parapet 1.0 m) + tabliye dış yüzü (kalınlık DECK)
        const ph = 1.0, o2 = o + 0.35 * side;
        W((i) => [at(i, o, yTop(i), inw(i), dist[i], 0), at(i, o, yTop(i) + ph, inw(i), dist[i], ph)]);
        W((i) => [at(i, o, yTop(i) + ph, up, dist[i], 0), at(i, o2, yTop(i) + ph, up, dist[i], 0.35)]);
        W((i) => [at(i, o2, yTop(i) + ph, out(i), dist[i], ph), at(i, o2, ys[i] - DECK, out(i), dist[i], -DECK)]);
        if (side < 0) outerL = o2; else outerR = o2;
      } else if (!(highway && side < 0 && road.o && road.s === 0)) {
        // dış yan duvar: araziye kadar iner (dolguda istinat duvarı olur, altı asla boş kalmaz)
        // dış kenardan araziye eğimli şev (1:2): yol araziye karışır, dik duvar yok
        graded(W, o, side, yTop, i0 => i0);
      }
    }

    if (bridge) {
      // tabliye alt yüzü
      W((i) => [at(i, outerL, ys[i] - DECK, down, dist[i], outerL), at(i, outerR, ys[i] - DECK, down, dist[i], outerR)]);
      // ayaklar: her ~30 m'de, yerden 2 m'den yüksekse; geniş tabliyede iki ayak
      const width = outerR - outerL;
      const cols2 = width > 14 ? [outerL + width * 0.25, outerR - width * 0.25] : [(outerL + outerR) / 2];
      let last = -1e9;
      for (let k = 0; k < rn; k++) {
        const i = RI(k);
        if (dist[i] - last < 30 || jd[i] < 5) continue;
        const yb = ys[i] - DECK;
        for (const oc of cols2) {
          const g = ground(i, oc);
          if (yb - g < 2) continue;
          pillar(wb, px(i, oc), pz(i, oc), dirx[i], dirz[i], yb, g - 1, info2);
        }
        // ayak başlığı (enine kiriş)
        if (yb - ground(i, (outerL + outerR) / 2) >= 2) {
          beam(wb, i, outerL + 0.5, outerR - 0.5, yb, yb - 1.2, px, pz, dirx, dirz, info2);
        }
        last = dist[i];
      }
    }
  }
}

// kare kesitli beton ayak (1.6 m): 4 yan yüz
function pillar(wb, x, z, tx, tz, yTop, yBot, info2) {
  const s = 0.8, rx = tz, rz = -tx;    // sağ vektör
  const c = [[-1, -1], [1, -1], [1, 1], [-1, 1]].map(([u, v]) => [x + (rx * u + tx * v) * s, z + (rz * u + tz * v) * s]);
  for (let f = 0; f < 4; f++) {
    const p0 = c[f], p1 = c[(f + 1) % 4];
    let nx = (p0[0] + p1[0]) / 2 - x, nz = (p0[1] + p1[1]) / 2 - z;
    const l = Math.hypot(nx, nz) || 1; nx /= l; nz /= l;
    const d = [p1[0] - p0[0], p1[1] - p0[1]], dl = Math.hypot(d[0], d[1]);
    wb.strip(2, (j) => {
      const p = j ? p1 : p0;
      return [[p[0], yTop, p[1], nx, 0, nz, j * dl, 0], [p[0], yBot, p[1], nx, 0, nz, j * dl, yBot - yTop]];
    }, () => [d[0] / dl, d[1] / dl], () => [1, 0, 0, 0], info2);
  }
}

// ayak başlığı: tabliye altında enine beton kiriş (ön/arka yüz + alt yüz)
function beam(wb, i, o0, o1, yTop, yBot, px, pz, dirx, dirz, info2) {
  const t = 0.8;
  for (const f of [-1, 1]) {
    const ox = dirx[i] * t * f, oz = dirz[i] * t * f;
    wb.strip(2, (j) => {
      const o = j ? o1 : o0;
      return [[px(i, o) + ox, yTop, pz(i, o) + oz, dirx[i] * f, 0, dirz[i] * f, o, 0],
              [px(i, o) + ox, yBot, pz(i, o) + oz, dirx[i] * f, 0, dirz[i] * f, o, yBot - yTop]];
    }, () => [dirz[i], -dirx[i]], () => [1, 0, 0, 0], info2);
  }
  wb.strip(2, (j) => {
    const o = j ? o1 : o0;
    return [[px(i, o) - dirx[i] * t, yBot, pz(i, o) - dirz[i] * t, 0, -1, 0, o, 0],
            [px(i, o) + dirx[i] * t, yBot, pz(i, o) + dirz[i] * t, 0, -1, 0, o, 2 * t]];
  }, () => [dirz[i], -dirx[i]], () => [1, 0, 0, 0], info2);
}

let roadMat, walkMat;
const builtTiles = new Set();
// part: 'major' (ana yollar) veya 'minor' (ara sokaklar)
function buildTilePart(t, part) {
  roadMat ||= roadMaterial(); walkMat ||= walkMaterial();
  const rb = new StripBuilder(), wb = new StripBuilder();
  for (const pc of t.pieces) if (MINOR.has(pc.R.road.c) === (part === 'minor')) emitPiece(rb, wb, pc);
  const g = new THREE.Group();
  const a = rb.mesh(roadMat), b = wb.mesh(walkMat);
  if (a) g.add(a);
  if (b) g.add(b);
  scene.add(g);
  t.built[part] = g;
  builtTiles.add(t);
}
function disposeTilePart(t, part) {
  const g = t.built[part];
  if (!g) return;
  scene.remove(g);
  g.traverse((o) => o.geometry && o.geometry.dispose());
  delete t.built[part];
  if (!t.built.major && !t.built.minor) builtTiles.delete(t);
}
// Kameranın çevresindeki karoları üret, uzaktakileri sil. budgetMs: kare başına süre sınırı
const STREAM = { major: 4500, minor: 2200 };
function streamRoads(px, pz, budgetMs = 6) {
  const t0 = performance.now();
  const want = [];
  for (const t of roadTileMap.values()) {
    const d = Math.hypot(t.cx - px, t.cz - pz) - TILE * 0.7;
    for (const part of ['major', 'minor']) {
      if (d < STREAM[part] && !t.built[part]) want.push([d, t, part]);
      else if (d > STREAM[part] + 800 && t.built[part]) disposeTilePart(t, part);
    }
  }
  want.sort((a, b) => a[0] - b[0]);
  for (const [, t, part] of want) {
    buildTilePart(t, part);
    if (performance.now() - t0 > budgetMs) break;
  }
}

prepareRoads();

// Arazi mesh'inin gerçek yüzeyi: her hücre (a,d,b) ve (b,d,e) üçgenlerine bölünür (köşegen b-d).
// Yolların arazinin altında kalmaması için çift doğrusal değil, bu üçgen yüzey okunur.
export function heightMesh(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  const ha = hAt(c, r), hb = hAt(c + 1, r), hd = hAt(c, r + 1), he = hAt(c + 1, r + 1);
  return u + v <= 1 ? ha + u * (hb - ha) + v * (hd - ha) : he + (1 - u) * (hd - he) + (1 - v) * (hb - he);
}

// Araziyi 128x128 hücrelik parçalara böl (görüş dışındakiler çizilmez)
const terrainMat = terrainMaterial();
const CH = 128;
const terrain = new THREE.Group();
for (let r0 = 0; r0 < rows - 1; r0 += CH) {
  for (let c0 = 0; c0 < cols - 1; c0 += CH) {
    const nc = Math.min(CH, cols - 1 - c0) + 1, nr = Math.min(CH, rows - 1 - r0) + 1;
    const pos = new Float32Array(nc * nr * 3), nor = new Float32Array(nc * nr * 3);
    let i = 0;
    for (let r = 0; r < nr; r++) for (let c = 0; c < nc; c++, i += 3) {
      const C = c0 + c, R = r0 + r;
      pos[i] = C * dx - Wm / 2; pos[i + 1] = hAt(C, R); pos[i + 2] = R * dz - Dm / 2;
      const nx = (hAt(C - 1, R) - hAt(C + 1, R)) / (2 * dx), nz = (hAt(C, R - 1) - hAt(C, R + 1)) / (2 * dz);
      const l = Math.hypot(nx, 1, nz);
      nor[i] = nx / l; nor[i + 1] = 1 / l; nor[i + 2] = nz / l;
    }
    const idx = new Uint32Array((nc - 1) * (nr - 1) * 6);
    let k = 0;
    for (let r = 0; r < nr - 1; r++) for (let c = 0; c < nc - 1; c++) {
      const a = r * nc + c, b = a + 1, d = a + nc, e = d + 1;
      idx[k++] = a; idx[k++] = d; idx[k++] = b; idx[k++] = b; idx[k++] = d; idx[k++] = e;
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
    g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
    g.setIndex(new THREE.BufferAttribute(idx, 1));
    g.computeBoundingSphere();
    terrain.add(new THREE.Mesh(g, terrainMat));
  }
}
scene.add(terrain);

const roadCount = preparedRoads.length;

// ---------------------------------------------------------------------------
// Başlangıç kamerası: Seul merkezi (Jongno), güneybatıdan bakış
// ---------------------------------------------------------------------------
function toWorld(lat, lon) {
  const [latMin, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [(lon - lonMin) * mlon - Wm / 2, (latMax - lat) * 111320 - Dm / 2];
}
const params = new URLSearchParams(location.search);
let [cx, cz] = toWorld(Number(params.get('lat') || 37.5715), Number(params.get('lon') || 126.9768)); // varsayılan: Gwanghwamun
cx += Number(params.get('ox') || 0); cz += Number(params.get('oz') || 0); // test için kaydırma
const camDist = Number(params.get('d') || 2500);
controls.target.set(cx, heightAt(cx, cz), cz);
camera.position.set(cx - camDist * 0.5, heightAt(cx, cz) + camDist * Number(params.get('hy') || 0.55), cz + camDist * 0.8);
controls.update();

document.getElementById('info').textContent =
  `Alan: ${(Wm / 1000).toFixed(1)} × ${(Dm / 1000).toFixed(1)} km · ${roadCount ? roadCount + ' yol' : 'yol verisi yok'}`;

// ---------------------------------------------------------------------------
// Klavye ile hareket
// ---------------------------------------------------------------------------
const keys = new Set();
addEventListener('keydown', (e) => keys.add(e.code));
addEventListener('keyup', (e) => keys.delete(e.code));
const clock = new THREE.Clock();
const fwd = new THREE.Vector3(), right = new THREE.Vector3();

function tick() {
  const dt = Math.min(clock.getDelta(), 0.1);
  const dist = camera.position.distanceTo(controls.target);
  const speed = dist * 0.8 * dt;
  camera.getWorldDirection(fwd); fwd.y = 0; fwd.normalize();
  right.crossVectors(fwd, camera.up).normalize();
  const move = new THREE.Vector3();
  if (keys.has('KeyW') || keys.has('ArrowUp')) move.add(fwd);
  if (keys.has('KeyS') || keys.has('ArrowDown')) move.sub(fwd);
  if (keys.has('KeyD') || keys.has('ArrowRight')) move.add(right);
  if (keys.has('KeyA') || keys.has('ArrowLeft')) move.sub(right);
  if (move.lengthSq()) {
    move.normalize().multiplyScalar(speed);
    camera.position.add(move); controls.target.add(move);
  }
  // kamera arazinin altına girmesin
  const gy = heightAt(camera.position.x, camera.position.z) + 3;
  if (camera.position.y < gy) camera.position.y = gy;

  // yolları kameranın (yere düşen) hedef noktası çevresinde akış halinde üret
  streamRoads(controls.target.x, controls.target.z, params.has('still') ? 1e9 : 6);

  controls.update();
  renderer.render(scene, camera);
  if (!params.has('still')) requestAnimationFrame(tick); // ?still: tek kare (test için)
}

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

const load = document.getElementById('load');
load.style.opacity = 0;
setTimeout(() => load.remove(), 500);
window.__ready = true;
tick();
