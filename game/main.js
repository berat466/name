import * as THREE from 'three';
import { MapControls } from 'three/addons/controls/MapControls.js';

// ---------------------------------------------------------------------------
// Sahne
// ---------------------------------------------------------------------------
const params = new URLSearchParams(location.search);
// dokunmatik cihaz (telefon/tablet): uçuş kamerası, düşük bellek ve hafif çizim
const isTouch = params.get('touch') === '1' || (params.get('touch') !== '0' &&
  (matchMedia('(pointer: coarse)').matches || 'ontouchstart' in window));
const STILL = params.has('still');   // tek kare (test için): her şey beklemeden üretilir
const renderer = new THREE.WebGLRenderer({ antialias: !isTouch });
renderer.setPixelRatio(Math.min(devicePixelRatio, isTouch ? 1.5 : 2));
renderer.setSize(innerWidth, innerHeight);
renderer.toneMapping = THREE.ACESFilmicToneMapping;
renderer.toneMappingExposure = 1.0;
document.body.appendChild(renderer.domElement);

const scene = new THREE.Scene();
const HORIZON = new THREE.Color(0xb4cbdd), ZENITH = new THREE.Color(0x4b83c4);
scene.background = HORIZON;
scene.fog = new THREE.FogExp2(HORIZON, 0.000032);

const camera = new THREE.PerspectiveCamera(55, innerWidth / innerHeight, 1.5, 160000);

scene.add(new THREE.HemisphereLight(0xcfe3ff, 0x4a4030, 0.9));
const sun = new THREE.DirectionalLight(0xfff1dc, 2.6);
sun.position.set(-0.5, 0.8, 0.35).multiplyScalar(10000);
scene.add(sun);

// gökyüzü: ufukta açık, tepede koyu mavi; kamerayı izler
const sky = new THREE.Mesh(new THREE.SphereGeometry(120000, 24, 12), new THREE.ShaderMaterial({
  side: THREE.BackSide, depthWrite: false, fog: false,
  uniforms: { cH: { value: HORIZON }, cZ: { value: ZENITH } },
  vertexShader: 'varying vec3 vP; void main(){ vP = position; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }',
  fragmentShader: `varying vec3 vP; uniform vec3 cH, cZ;
    void main(){ float h = clamp(normalize(vP).y, 0.0, 1.0); gl_FragColor = vec4(mix(cH, cZ, pow(h, 0.55)), 1.0);
      #include <tonemapping_fragment>
      #include <colorspace_fragment>
    }`,
}));
sky.renderOrder = -10;
scene.add(sky);

const controls = new MapControls(camera, renderer.domElement);
controls.enableDamping = true;
controls.dampingFactor = 0.08;
controls.maxPolarAngle = Math.PI * 0.49;
controls.minDistance = 15;
controls.maxDistance = 70000;
controls.screenSpacePanning = false;
controls.zoomToCursor = true;

// ---------------------------------------------------------------------------
// Dokular (Poly Haven, CC0, 2K) — birden çok PBR seti doku dizisi (texture array) olarak
// ---------------------------------------------------------------------------
const maxAniso = renderer.capabilities.getMaxAnisotropy();
// Telefonda 1K (4 kat az bellek); ?q=2k ile 2K zorlanabilir
const TEX = (isTouch && params.get('q') !== '2k') ? 1024 : 2048;

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
  'dirt_floor',        // 2 toprak (yerleşim zemini, şevler)
  'forest_leaves_02',  // 3 orman zemini
  'aerial_rocks_02',   // 4 kaya
  'mossy_rock',        // 5 yosunlu kaya
];
// Yol katmanları: 0-2 asfalt, 3 beton (kaldırım, istinat duvarı, köprü)
const ROAD_SETS = ['asphalt_02', 'aerial_asphalt_01', 'asphalt_04', 'concrete_pavement_02'];

const [terrainTex, roadTex] = await Promise.all([TERRAIN_SETS, ROAD_SETS].map(loadPbrArray));

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
function patchStandard(mat, uniforms, vtxExtra, fragDecl, fragBody, fixedKey) {
  // her malzeme türü kendi shader'ını derlesin (aksi halde three.js aynı programı paylaştırır);
  // fixedKey verilirse aynı türden malzemeler (ör. her yol karosu) tek programı paylaşır
  const key = fixedKey || 'patched-' + (shaderId++);
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
// Köprü tabliyesi malzemesi (vektör geometri): 3 asfalt PBR seti + lastik izi + şerit çizgileri
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
  patchStandard(mat, { tA: { value: roadTex.alb }, tN: { value: roadTex.nra } }, ROAD_VTX, ROAD_FRAG_DECL, `
    float layer = vInfo.x;  // 0 = parke, 1 = beton
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addLayer(tA, tN, vRuv / 2.5, 3.0, 1.0);
    ${GLSL_RESOLVE}
    diffuseColor.rgb *= accA * (0.85 + 0.25 * vnoise(vWP.xz * 0.05));
    ${GLSL_ROAD_TBN}
  `);
  return mat;
}

// ---------------------------------------------------------------------------
// Harita verisi
// ---------------------------------------------------------------------------
const meta = await (await fetch('./data/terrain.json')).json();
const { cols, rows, width: Wm, depth: Dm } = meta;
const HN = await fetch('./data/heights.u16').then((r) => r.arrayBuffer()).then((b) => {   // doğal arazi: hiçbir zaman değiştirilmez
  const u = new Uint16Array(b), f = new Float32Array(u.length);
  for (let i = 0; i < u.length; i++) f[i] = u[i] * meta.hscale;
  return f;
});
const dx = Wm / (cols - 1), dz = Dm / (rows - 1);
const hAt = (c, r) => HN[(r < 0 ? 0 : r >= rows ? rows - 1 : r) * cols + (c < 0 ? 0 : c >= cols ? cols - 1 : c)];
// doğal arazi, çift doğrusal
function heightAt(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  return (hAt(c, r) * (1 - u) + hAt(c + 1, r) * u) * (1 - v) + (hAt(c, r + 1) * (1 - u) + hAt(c + 1, r + 1) * u) * v;
}
// doğal arazi, yumuşak (Catmull-Rom): yakın arazide 30 m'lik veri köşeli görünmesin
const cr = (a, b, c, d, t) => b + 0.5 * t * (c - a + t * (2 * a - 5 * b + 4 * c - d + t * (3 * (b - c) + d - a)));
function heightSmooth(x, z) {
  const fc = (x + Wm / 2) / dx, fr = (z + Dm / 2) / dz;
  const c = Math.floor(fc), r = Math.floor(fr), u = fc - c, v = fr - r;
  const row = (rr) => cr(hAt(c - 1, rr), hAt(c, rr), hAt(c + 1, rr), hAt(c + 2, rr), u);
  return Math.max(0, cr(row(r - 1), row(r), row(r + 1), row(r + 2), v));
}
const toWorld = (lat, lon) => [meta.ll2w[0][0] + meta.ll2w[0][1] * lon + meta.ll2w[0][2] * lat,
                               meta.ll2w[1][0] + meta.ll2w[1][1] * lon + meta.ll2w[1][2] * lat];
const toLatLon = (x, z) => [meta.w2ll[1][0] + meta.w2ll[1][1] * x + meta.w2ll[1][2] * z,
                            meta.w2ll[0][0] + meta.w2ll[0][1] * x + meta.w2ll[0][2] * z];

// ---- yol ağı: her noktada konum + çözülmüş yol kotu ----
const rd = await (await fetch('./data/roads.json')).json();
const rbin = await (await fetch('./data/roads.bin')).arrayBuffer();
const RX = new Float32Array(rbin, 0, rd.points), RZ = new Float32Array(rbin, 4 * rd.points, rd.points),
      RY = new Float32Array(rbin, 8 * rd.points, rd.points);
// sınıf sırası: motorway, trunk, primary, secondary, tertiary, unclassified, residential, living_street, service
const ORDER = [6, 5, 4, 3, 2, 1, 1, 0, 0];
const roads = [];
{
  let off = 0;
  for (const [c, n, o, lw, sl, sr, w, s, flags, len] of rd.roads) {
    roads.push({ c, n, o, lw, sl, sr, w, s, off, len, order: ORDER[c],
      bridge: !!(flags & 1), tunnel: !!(flags & 2), cutA: !!(flags & 4), cutB: !!(flags & 8),
      R: w / 2 + s + 0.6 });                      // düz platform yarı genişliği (asfalt + kaldırım + banket)
    off += len;
  }
}

// ---- harita dokuları: uzak yollar (R) + su (G), yerleşim yoğunluğu ----
const loadImage = async (url) => { const im = new Image(); im.src = url; await im.decode(); return im; };
async function buildMapTex() {
  const S = isTouch ? 2048 : 4096, sx = S / Wm, sz = S / Dm;
  const c = document.createElement('canvas'); c.width = c.height = S;
  const g = c.getContext('2d', { willReadFrequently: true });
  const out = new Uint8Array(S * S * 2);
  // su
  g.drawImage(await loadImage('./data/water.png'), 0, 0, S, S);
  let d = g.getImageData(0, 0, S, S).data;
  for (let i = 0, k = 1; i < d.length; i += 4, k += 2) out[k] = d[i];
  // uzak yollar: ana yollar gerçek genişlikte (en az 1 piksel), ara sokaklar soluk ince çizgi
  g.fillStyle = '#000'; g.fillRect(0, 0, S, S);
  g.lineJoin = 'round'; g.lineCap = 'round'; g.strokeStyle = '#fff';
  const draw = (pred, width, alpha) => {
    g.globalAlpha = alpha; g.lineWidth = width; g.beginPath();
    for (const r of roads) {
      if (r.tunnel || !pred(r)) continue;
      for (let p = r.off; p < r.off + r.len; p++) {
        const x = (RX[p] + Wm / 2) * sx, z = (RZ[p] + Dm / 2) * sz;
        if (p === r.off) g.moveTo(x, z); else g.lineTo(x, z);
      }
    }
    g.stroke();
  };
  draw((r) => r.order === 1, 0.7, 0.55);
  for (const ord of [2, 3, 4, 5, 6]) draw((r) => r.order === ord, Math.max(1.0, (8 + ord * 3) * sx), 1);
  d = g.getImageData(0, 0, S, S).data;
  for (let i = 0, k = 0; i < d.length; i += 4, k += 2) out[k] = d[i];
  c.width = c.height = 0;
  const t = new THREE.DataTexture(out, S, S, THREE.RGFormat);
  t.minFilter = THREE.LinearMipmapLinearFilter; t.magFilter = THREE.LinearFilter; t.generateMipmaps = true;
  t.anisotropy = maxAniso; t.needsUpdate = true;
  return t;
}
async function buildUrbanTex() {
  const im = await loadImage('./data/urban.png');
  const c = document.createElement('canvas'); c.width = im.width; c.height = im.height;
  const g = c.getContext('2d'); g.drawImage(im, 0, 0);
  const d = g.getImageData(0, 0, c.width, c.height).data, out = new Uint8Array(c.width * c.height);
  for (let i = 0, k = 0; i < d.length; i += 4, k++) out[k] = d[i];
  const t = new THREE.DataTexture(out, c.width, c.height, THREE.RedFormat);
  t.minFilter = t.magFilter = THREE.LinearFilter; t.needsUpdate = true;
  return t;
}
const [mapTex, urbanTex] = await Promise.all([buildMapTex(), buildUrbanTex()]);

// yakın arazi karoları (aşağıda) hangi 256 m'lik hücreleri kaplıyor: uzak arazi orada çizilmez
const BT = 256, NT = 256;                          // karo boyu (m), kaplama dokusu boyutu
const coverData = new Uint8Array(NT * NT);
const coverTex = new THREE.DataTexture(coverData, NT, NT, THREE.RedFormat);
coverTex.minFilter = coverTex.magFilter = THREE.NearestFilter; coverTex.needsUpdate = true;

// ---------------------------------------------------------------------------
// Arazi malzemesi (uzak arazi ve yakın karolar aynı shader'ı paylaşır)
//   - eğim, yükseklik, gürültü ve yerleşim yoğunluğuna göre 6 PBR setinin karışımı
//   - su: hareketli dalga + gökyüzü yansıması
//   - PATCH (yakın karo): şev/istinat duvarı + asfalt, kaldırım, boya maskesi
// ---------------------------------------------------------------------------
const uTime = { value: 0 };
const TERRAIN_DECL = `
  uniform sampler2D tMap, tUrban;
  uniform vec2 uWorld; uniform float uTime; uniform vec3 uSky;
  #ifdef PATCH
    uniform highp sampler2DArray rA, rN;
    uniform sampler2D tM1, tM2; uniform vec3 uTile;
    varying vec2 vMod;
  #else
    uniform sampler2D tCover; uniform float uCoverInv;
  #endif
  // Doku türevleri dallanmadan önce bir kez hesaplanır: koşullu örneklemede mip seçimi bozulmaz
  // (aksi halde karışım sınırlarında siyah noktalar oluşur).
  vec2 gDx, gDy;
  vec4 smpG(highp sampler2DArray t, vec2 uv, float sc, float l){
    return mix(textureGrad(t, vec3(uv, l), gDx * sc, gDy * sc),
               textureGrad(t, vec3(uv * 0.137 + 0.31, l), gDx * sc * 0.137, gDy * sc * 0.137), 0.35);
  }
  void addG(highp sampler2DArray A, highp sampler2DArray N, float sc, float l, float w){
    if (w < 0.004) return;
    vec2 uv = vWP.xz * sc;
    accA += smpG(A, uv, sc, l).rgb * w; accN += smpG(N, uv, sc, l) * w; accW += w;
  }
  void resolveAcc(out vec3 alb, out vec3 tn, out float r, out float ao){
    alb = accA / accW; vec4 n4 = accN / accW;
    vec2 nxy = n4.xy * 2.0 - 1.0; nxy.y = -nxy.y;   // doku dizisi Y çevrilmeden yüklendi
    tn = vec3(nxy, sqrt(max(0.0, 1.0 - dot(nxy, nxy)))); r = n4.z; ao = n4.w;
  }
`;
const TERRAIN_BODY = `
  vec3 N0 = normalize(vWN);
  gDx = dFdx(vWP.xz); gDy = dFdy(vWP.xz);
  vec2 wuv = (vWP.xz + uWorld * 0.5) / uWorld;
  #ifndef PATCH
    if (texture2D(tCover, (vWP.xz + uWorld * 0.5) * uCoverInv).r > 0.5) discard;   // burada yakın karo var
  #endif
  vec4 mp = texture2D(tMap, wuv);
  float urban = texture2D(tUrban, wuv).r;
  float wm = smoothstep(0.42, 0.58, mp.g);
  #ifdef PATCH
    float bank = vMod.x;
  #else
    float bank = 0.0;
  #endif

  float slope = 1.0 - N0.y;
  float n1 = vnoise(vWP.xz * 0.004) * 0.6 + vnoise(vWP.xz * 0.02) * 0.4;
  float n2 = vnoise(vWP.xz * 0.011 + 17.0);
  float n3 = vnoise(vWP.xz * 0.012 + 91.0) * 0.7 + vnoise(vWP.xz * 0.06 + 5.0) * 0.3;
  // bölgeler: kaya (dik), orman (yamaç ve yüksek), çim/toprak (düz)
  float rock = smoothstep(0.34, 0.52, slope + (n1 - 0.5) * 0.15) * (1.0 - bank);
  float forest = smoothstep(35.0, 100.0, vWP.y + (n1 - 0.5) * 60.0) * (1.0 - rock);
  forest = max(forest, smoothstep(0.07, 0.18, slope) * (1.0 - rock) * 0.9);
  forest *= (1.0 - urban * 0.9) * (1.0 - bank);
  float grass = max(0.0, 1.0 - rock - forest);
  float gB = smoothstep(0.35, 0.65, n2);
  float dirt = max(smoothstep(0.58, 0.85, n3) * 0.5, max(urban * (0.45 + 0.4 * smoothstep(0.3, 0.7, n2)), bank));
  float rB = smoothstep(0.4, 0.6, n2);
  float fB = smoothstep(0.45, 0.75, n3) * 0.5;
  float w0 = grass * (1.0 - gB) * (1.0 - dirt);
  float w1 = grass * gB * (1.0 - dirt) + forest * fB;
  float w2 = grass * dirt;
  float w3 = forest * (1.0 - fB);
  float w4 = rock * (1.0 - rB);
  float w5 = rock * rB;

  vec3 col = vec3(0.0), tnv = vec3(0.0, 0.0, 1.0); float R_ = 1.0, AO_ = 1.0;
  if (wm < 0.995) {
    accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
    addG(tA, tN, 1.0 / 5.0, 0.0, w0);
    addG(tA, tN, 1.0 / 16.0, 1.0, w1);
    addG(tA, tN, 1.0 / 4.0, 2.0, w2);
    addG(tA, tN, 1.0 / 3.0, 3.0, w3);
    addG(tA, tN, 1.0 / 14.0, 4.0, w4);
    addG(tA, tN, 1.0 / 6.0, 5.0, w5);
    resolveAcc(col, tnv, R_, AO_);
    col *= mix(vec3(1.0), vec3(0.85, 1.0, 0.7), grass * (1.0 - dirt));     // çimi canlandır
    col = mix(col, vec3(0.09, 0.15, 0.06), forest * 0.5);                  // yukarıdan ağaç örtüsü
    // toprak: turuncu yerine gri-kahve (doygunluğu düşür); yerleşimde daha da gri
    float lum = dot(col, vec3(0.3, 0.55, 0.15));
    col = mix(col, lum * vec3(0.80, 0.74, 0.66), clamp((grass * dirt + bank) * (0.65 + 0.3 * urban), 0.0, 1.0));
    col *= 0.85 + 0.3 * vnoise(vWP.xz * 0.0015);                           // makro varyasyon
    #ifndef PATCH
      col = mix(col, vec3(0.19, 0.19, 0.21), mp.r * 0.9);                  // uzaktaki yollar
    #endif
  }

  #ifdef PATCH
  {
    vec2 muv = (vWP.xz - uTile.xy) * uTile.z;
    vec4 m1 = texture2D(tM1, muv); float my = texture2D(tM2, muv).r;
    float wa = m1.r, ws = min(m1.g, 1.0 - m1.r);
    float wl = vMod.y * max(0.0, 1.0 - wa - ws);                          // istinat duvarı
    float cov = clamp(wa + ws + wl, 0.0, 1.0);
    if (cov > 0.004) {
      float q1 = vnoise(vWP.xz * 0.02), q2 = vnoise(vWP.xz * 0.07 + 33.0);
      float wB = smoothstep(0.4, 0.8, q1) * 0.6, wC = smoothstep(0.55, 0.85, q2) * 0.5;
      accA = vec3(0.0); accN = vec4(0.0); accW = 0.0;
      addG(rA, rN, 1.0 / 5.0, 0.0, wa * (1.0 - wB) * (1.0 - wC));
      addG(rA, rN, 1.0 / 8.0, 1.0, wa * wB * (1.0 - wC));
      addG(rA, rN, 1.0 / 4.0, 2.0, wa * wC);
      addG(rA, rN, 1.0 / 2.5, 3.0, ws);
      if (wl > 0.004) {                                                    // duvar: dikey yüzeyde yandan izdüşüm
        vec3 wu = vec3((vWP.x + vWP.z) / 3.0, vWP.y / 3.0, 3.0);
        accA += textureGrad(rA, wu, gDx / 3.0, gDy / 3.0).rgb * wl; accN += textureGrad(rN, wu, gDx / 3.0, gDy / 3.0) * wl; accW += wl;
      }
      vec3 ra, rtn; float rr, rao;
      resolveAcc(ra, rtn, rr, rao);
      ra *= (wa * vec3(0.60, 0.61, 0.64) + ws * vec3(0.74, 0.73, 0.71) + wl * vec3(0.78, 0.78, 0.76)) / max(wa + ws + wl, 1e-3);
      ra = mix(ra, vec3(0.6, 0.6, 0.58), clamp(4.0 * wa * ws, 0.0, 1.0) * 0.8);   // bordür taşı
      float wear = mix(0.6, 1.0, vnoise(vWP.xz * 0.35));
      float white = smoothstep(0.3, 0.6, m1.b) * wear * wa, yellow = smoothstep(0.3, 0.6, my) * wear * wa;
      ra = mix(ra, vec3(0.82), white);
      ra = mix(ra, vec3(0.85, 0.6, 0.07), yellow);
      rr = mix(rr, 0.5, max(white, yellow));
      col = mix(col, ra, cov); tnv = normalize(mix(tnv, rtn, cov)); R_ = mix(R_, rr, cov); AO_ = mix(AO_, rao, cov);
    }
  }
  #endif

  vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
  vec3 Bt = normalize(cross(Tg, N0));
  blendN = normalize(Tg * tnv.x + Bt * tnv.y + N0 * tnv.z);
  if (wm > 0.001) {
    // su: iki ölçekli hareketli dalga, bakış açısına göre gökyüzü yansıması
    vec2 p = vWP.xz; float t = uTime;
    vec2 rip = vec2(vnoise(p * 0.08 + vec2(t * 0.35, t * 0.1)) - 0.5, vnoise(p * 0.08 + 31.0 - vec2(t * 0.2, t * 0.3)) - 0.5)
             + 0.5 * vec2(vnoise(p * 0.45 + t * 0.6) - 0.5, vnoise(p * 0.45 + 11.0 - t * 0.5) - 0.5);
    vec3 wn = normalize(vec3(rip.x * 0.45, 1.0, rip.y * 0.45));
    vec3 V = normalize(cameraPosition - vWP);
    float fres = pow(1.0 - max(dot(V, wn), 0.0), 4.0);
    vec3 wc = mix(vec3(0.10, 0.25, 0.24), vec3(0.03, 0.12, 0.2), smoothstep(0.55, 1.0, mp.g));
    wc = mix(wc, uSky * 0.8, fres * 0.75);
    col = mix(col, wc, wm); R_ = mix(R_, 0.08, wm); AO_ = mix(AO_, 1.0, wm);
    blendN = normalize(mix(blendN, wn, wm));
  }
  blendR = R_; blendAO = AO_;
  diffuseColor.rgb *= col;
`;
const terrainUniforms = () => ({
  tA: { value: terrainTex.alb }, tN: { value: terrainTex.nra }, tMap: { value: mapTex }, tUrban: { value: urbanTex },
  uWorld: { value: new THREE.Vector2(Wm, Dm) }, uTime, uSky: { value: HORIZON },
});
const terrainMat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0 });
patchStandard(terrainMat, { ...terrainUniforms(), tCover: { value: coverTex }, uCoverInv: { value: 1 / (NT * BT) } },
  {}, TERRAIN_DECL, TERRAIN_BODY, 'terrain-far');
const blankTex = new THREE.DataTexture(new Uint8Array(4), 1, 1); blankTex.needsUpdate = true;
function patchMaterial(t1, t2, x0, z0, inv) {
  const mat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0 });
  mat.defines = { PATCH: '' };
  patchStandard(mat, { ...terrainUniforms(), rA: { value: roadTex.alb }, rN: { value: roadTex.nra },
    tM1: { value: t1 }, tM2: { value: t2 }, uTile: { value: new THREE.Vector3(x0, z0, inv) } },
    { decl: 'attribute vec2 aMod; varying vec2 vMod;', body: 'vMod = aMod;' }, TERRAIN_DECL, TERRAIN_BODY, 'terrain-near');
  return mat;
}

// ---------------------------------------------------------------------------
// Uzak arazi: dörtlü ağaç (quadtree) LOD. Kök tüm haritayı kaplar; kameraya yakın düğümler bölünür.
// Her düğüm QN x QN hücrelik ağ + etek (farklı ayrıntı düzeyleri arasındaki çatlakları kapatır).
// ---------------------------------------------------------------------------
const QN = isTouch ? 32 : 64, CELLS = cols - 1, MAXL = Math.round(Math.log2(CELLS / QN));
const SPLIT = isTouch ? 1.25 : 1.7;
const nodeCache = new Map();
const farGroup = new THREE.Group();
scene.add(farGroup);
function nodeMesh(level, nx, nz) {
  const key = (level << 20) | (nx << 10) | nz;
  let e = nodeCache.get(key);
  if (e) return e;
  const span = CELLS >> level, st = span / QN, c0 = nx * span, r0 = nz * span, n = QN + 1;
  const nv = n * n + 4 * n, pos = new Float32Array(nv * 3), nor = new Float32Array(nv * 3);
  let v = 0, ymin = 1e9, ymax = -1e9;
  for (let j = 0; j < n; j++) for (let i = 0; i < n; i++, v++) {
    const C = c0 + i * st, Rr = r0 + j * st, y = hAt(C, Rr);
    pos[3 * v] = C * dx - Wm / 2; pos[3 * v + 1] = y; pos[3 * v + 2] = Rr * dz - Dm / 2;
    const gx = (hAt(C - st, Rr) - hAt(C + st, Rr)) / (2 * st * dx), gz = (hAt(C, Rr - st) - hAt(C, Rr + st)) / (2 * st * dz);
    const l = Math.hypot(gx, 1, gz);
    nor[3 * v] = gx / l; nor[3 * v + 1] = 1 / l; nor[3 * v + 2] = gz / l;
    if (y < ymin) ymin = y; if (y > ymax) ymax = y;
  }
  const idx = [];
  for (let j = 0; j < QN; j++) for (let i = 0; i < QN; i++) {
    const a = j * n + i, b = a + 1, d = a + n, f = d + 1;
    idx.push(a, d, b, b, d, f);
  }
  // etek: dört kenar boyunca aşağı sarkan şerit
  const drop = 6 * st * dx / 30 + 4;
  const edge = (get) => {
    const base = v;
    for (let k = 0; k < n; k++, v++) {
      const s = get(k);
      pos[3 * v] = pos[3 * s]; pos[3 * v + 1] = pos[3 * s + 1] - drop; pos[3 * v + 2] = pos[3 * s + 2];
      nor[3 * v] = nor[3 * s]; nor[3 * v + 1] = nor[3 * s + 1]; nor[3 * v + 2] = nor[3 * s + 2];
      if (k) { const a = get(k - 1), b = s, c = base + k - 1, d = base + k; idx.push(a, c, b, b, c, d, a, b, c, b, d, c); }
    }
  };
  edge((k) => k); edge((k) => (n - 1) * n + k); edge((k) => k * n); edge((k) => k * n + n - 1);
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
  g.setIndex(new THREE.BufferAttribute(new Uint16Array(idx), 1));
  g.computeBoundingSphere();
  const mesh = new THREE.Mesh(g, terrainMat);
  mesh.visible = false;
  farGroup.add(mesh);
  const size = span * dx;
  e = { mesh, used: 0, cx: (c0 + span / 2) * dx - Wm / 2, cz: (r0 + span / 2) * dz - Dm / 2, cy: (ymin + ymax) / 2, size };
  nodeCache.set(key, e);
  return e;
}
let farFrame = 0;
function updateFar(cam) {
  farFrame++;
  const visit = (level, nx, nz) => {
    const span = CELLS >> level, size = span * dx, h = size / 2;
    const ncx = (nx * span + span / 2) * dx - Wm / 2, ncz = (nz * span + span / 2) * dz - Dm / 2;
    const ddx = Math.max(0, Math.abs(cam.x - ncx) - h), ddz = Math.max(0, Math.abs(cam.z - ncz) - h);
    const d = Math.hypot(ddx, ddz, cam.y - heightAt(THREE.MathUtils.clamp(cam.x, ncx - h, ncx + h), THREE.MathUtils.clamp(cam.z, ncz - h, ncz + h)));
    if (level < MAXL && d < size * SPLIT) {
      for (let k = 0; k < 4; k++) visit(level + 1, nx * 2 + (k & 1), nz * 2 + (k >> 1));
    } else nodeMesh(level, nx, nz).used = farFrame;
  };
  visit(0, 0, 0);
  for (const [key, e] of nodeCache) {
    e.mesh.visible = e.used === farFrame;
    if (farFrame - e.used > 400) { farGroup.remove(e.mesh); e.mesh.geometry.dispose(); nodeCache.delete(key); }
  }
}

// ---------------------------------------------------------------------------
// Yakın arazi karoları: kesim-dolgu burada yapılır (doğal veri değişmez, katman olarak uygulanır)
//   Her 256 m'lik karo, kameraya uzaklığına göre 2 / 4 / 8 m ızgarayla yeniden üretilir:
//     - yol platformu (asfalt + kaldırım + banket) çözülmüş yol kotunda dümdüz olur
//     - platformun dışında arazi şevle doğal zemine bağlanır (yarma 1:1,5, dolgu 1:2)
//     - şev 24 m'ye sığmayacak kadar derinse platform kenarına istinat duvarı gelir
//     - köprü altında zemin yalnızca tabliyeye değmeyecek kadar kazılır
//   Yol, kaldırım ve boya aynı yüzeye maske dokusuyla basılır: ayrı yol katmanı yoktur.
// ---------------------------------------------------------------------------
const BPAD = 2, BEXT = BT + 2 * BPAD;
const BANK = 24, S_CUT = 1.5, S_FILL = 2.0;
const LODS = isTouch
  ? [{ r: 80, cell: 2, res: 1024 }, { r: 180, cell: 2, res: 512 }, { r: 420, cell: 4, res: 256 }, { r: 800, cell: 8, res: 128 }]
  : [{ r: 120, cell: 2, res: 2048 }, { r: 300, cell: 2, res: 1024 }, { r: 650, cell: 4, res: 512 }, { r: 1250, cell: 8, res: 256 }];
const NTX = Math.ceil(Wm / BT), NTZ = Math.ceil(Dm / BT);
const tiles = new Array(NTX * NTZ);
const tileOf = (tx, tz) => (tiles[tz * NTX + tx] ||= { tx, tz, roads: [], junc: [], patch: null });
{
  // her yolun (şev payıyla) değdiği karolar
  const seen = new Set();
  roads.forEach((r, ri) => {
    if (r.tunnel) return;
    seen.clear();
    const reach = r.R + BANK + 8;
    for (let p = r.off; p < r.off + r.len - 1; p++) {
      const x0 = Math.floor((Math.min(RX[p], RX[p + 1]) - reach + Wm / 2) / BT), x1 = Math.floor((Math.max(RX[p], RX[p + 1]) + reach + Wm / 2) / BT);
      const z0 = Math.floor((Math.min(RZ[p], RZ[p + 1]) - reach + Dm / 2) / BT), z1 = Math.floor((Math.max(RZ[p], RZ[p + 1]) + reach + Dm / 2) / BT);
      for (let a = Math.max(0, x0); a <= Math.min(NTX - 1, x1); a++) for (let b = Math.max(0, z0); b <= Math.min(NTZ - 1, z1); b++) seen.add(b * NTX + a);
    }
    for (const k of seen) tileOf(k % NTX, (k / NTX) | 0).roads.push(ri);
  });
  for (let i = 0; i < rd.junc.length; i += 3) {
    const x = rd.junc[i], z = rd.junc[i + 1], m = rd.junc[i + 2] + 4;
    for (let a = Math.floor((x - m + Wm / 2) / BT); a <= Math.floor((x + m + Wm / 2) / BT); a++)
      for (let b = Math.floor((z - m + Dm / 2) / BT); b <= Math.floor((z + m + Dm / 2) / BT); b++)
        if (a >= 0 && b >= 0 && a < NTX && b < NTZ) tileOf(a, b).junc.push(i);
  }
}

// Karo yüzeyi: (n+2)² yükseklik (normal için bir halka fazlası) + şev / duvar ağırlıkları
function tileSurface(t, cell) {
  const n = BT / cell + 1, m = n + 2, mm = m * m;
  const x0 = t.tx * BT - Wm / 2 - cell, z0 = t.tz * BT - Dm / 2 - cell;
  const xmax = x0 + (m - 1) * cell, zmax = z0 + (m - 1) * cell;
  const h = new Float32Array(mm), bank = new Float32Array(mm), wall = new Float32Array(mm);
  for (let j = 0, k = 0; j < m; j++) for (let i = 0; i < m; i++, k++) h[k] = heightSmooth(x0 + i * cell, z0 + j * cell);
  if (!t.roads.length) return { n, m, h, bank, wall };
  const sw = new Float32Array(mm), swe = new Float32Array(mm), bdev = new Float32Array(mm), bh = new Float32Array(mm),
        bw = new Float32Array(mm), bcut = new Float32Array(mm).fill(1e9);
  for (const ri of t.roads) {
    const r = roads[ri], R = r.R, reach = R + BANK, reach2 = reach * reach, last = r.off + r.len - 1, pri = 1 + r.order;
    for (let p = r.off; p < last; p++) {
      const ax = RX[p], az = RZ[p], bx = RX[p + 1], bz = RZ[p + 1];
      const lox = Math.min(ax, bx) - reach, hix = Math.max(ax, bx) + reach, loz = Math.min(az, bz) - reach, hiz = Math.max(az, bz) + reach;
      if (hix < x0 || lox > xmax || hiz < z0 || loz > zmax) continue;
      const ay = RY[p], by = RY[p + 1], sx = bx - ax, sz = bz - az, len2 = sx * sx + sz * sz || 1e-6;
      const i0 = Math.max(0, Math.floor((lox - x0) / cell)), i1 = Math.min(m - 1, Math.ceil((hix - x0) / cell));
      const j0 = Math.max(0, Math.floor((loz - z0) / cell)), j1 = Math.min(m - 1, Math.ceil((hiz - z0) / cell));
      // uç düzlemi: köprüye/tünele bağlanan uçta ve köprünün kendi uçlarında düzeltme düz kesilir
      const cutA = p === r.off && (r.cutA || r.bridge), cutB = p === last - 1 && (r.cutB || r.bridge);
      for (let j = j0; j <= j1; j++) {
        const pz = z0 + j * cell;
        for (let i = i0; i <= i1; i++) {
          const px = x0 + i * cell;
          let tt = ((px - ax) * sx + (pz - az) * sz) / len2;
          if (tt < 0) { if (cutA) continue; tt = 0; } else if (tt > 1) { if (cutB) continue; tt = 1; }
          const qx = px - (ax + sx * tt), qz = pz - (az + sz * tt), d2 = qx * qx + qz * qz;
          if (d2 > reach2) continue;
          const d = Math.sqrt(d2), e = ay + (by - ay) * tt, k = j * m + i;
          if (r.bridge) {                               // tabliye altı boşluğu: yalnızca kazı
            const lim = e - 2.4 + (d > R ? (d - R) / S_CUT : 0);
            if (lim < bcut[k]) bcut[k] = lim;
          } else if (d <= R) {                          // platform: ağırlıklı ortalama (kavşakta yumuşak birleşme)
            const q = 1 - d / R, w = (q * q + 1e-3) * pri;
            sw[k] += w; swe[k] += w * e;
          } else {                                      // şev
            const wd = d - R, dh = h[k] - e, adh = Math.abs(dh), S = dh > 0 ? S_CUT : S_FILL;
            const wallH = Math.max(0, adh - BANK / S), allowed = wallH + wd / S;
            if (adh > allowed) {
              const hb = e + (dh > 0 ? allowed : -allowed), dev = adh - allowed;
              if (dev > bdev[k]) { bdev[k] = dev; bh[k] = hb; bw[k] = wallH > 0.6 && wd < cell * 1.5 + 1 ? Math.min(1, wallH / 2) : 0; }
            }
          }
        }
      }
    }
  }
  for (let k = 0; k < mm; k++) {
    if (sw[k] > 0) { h[k] = swe[k] / sw[k]; bank[k] = 1; }
    else if (bdev[k] > 0) { h[k] = bh[k]; bank[k] = Math.min(1, bdev[k] / 0.8); wall[k] = bw[k]; }
    if (bcut[k] < h[k]) { h[k] = bcut[k]; bank[k] = 1; }
  }
  return { n, m, h, bank, wall };
}

// Maske dokuları: 1) R asfalt, G kaldırım, B beyaz boya   2) sarı boya
function bakeMasks(t, res) {
  const BK = res / BEXT, x0 = t.tx * BT - Wm / 2 - BPAD, z0 = t.tz * BT - Dm / 2 - BPAD;
  const mk = () => {
    const c = document.createElement('canvas'); c.width = c.height = res;
    const g = c.getContext('2d', { alpha: false });
    g.fillStyle = '#000'; g.fillRect(0, 0, res, res); g.lineJoin = 'round';
    return [c, g];
  };
  const [c1, g1] = mk(), [c2, g2] = mk();
  const X = (x) => (x - x0) * BK, Z = (z) => (z - z0) * BK;
  const list = t.roads.map((i) => roads[i]).filter((r) => !r.bridge && !r.tunnel).sort((a, b) => a.order - b.order);
  const trace = (g, r, off) => {
    g.beginPath();
    const a0 = r.off, a1 = r.off + r.len - 1;
    for (let p = a0; p <= a1; p++) {
      let x = RX[p], z = RZ[p];
      if (off) {
        const a = Math.max(a0, p - 1), b = Math.min(a1, p + 1);
        let tx = RX[b] - RX[a], tz = RZ[b] - RZ[a];
        const l = Math.hypot(tx, tz) || 1;
        x += (tz / l) * off; z -= (tx / l) * off;
      }
      if (p === a0) g.moveTo(X(x), Z(z)); else g.lineTo(X(x), Z(z));
    }
  };
  const line = (g, r, off, color, widthM, dash) => {
    trace(g, r, off);
    const px = widthM * BK;
    // çizgi en az 1,8 piksel çizilir, shader eşikleyerek keskinleştirir; çok uzakta soluklaşır
    g.globalAlpha = Math.min(1, 0.45 + px / 1.2);
    g.strokeStyle = color; g.lineWidth = Math.max(px, 1.8); g.lineCap = 'butt';
    g.setLineDash(dash ? [3 * BK, 5 * BK] : []);
    g.stroke(); g.globalAlpha = 1; g.setLineDash([]);
  };
  const body = (r, color, widthM) => {
    trace(g1, r, 0);
    g1.strokeStyle = color; g1.lineWidth = widthM * BK;
    g1.lineCap = r.cutA || r.cutB ? 'butt' : 'round';   // köprü/tünel ucunda düz biter
    g1.stroke();
  };
  for (const r of list) if (r.s > 0) body(r, '#0f0', r.w + 2 * r.s);
  for (const r of list) body(r, '#f00', r.w);
  // boya: ana yollarda orta sarı çift çizgi, şerit çizgileri, kenar çizgisi
  for (const r of list) {
    if (r.order < 2) continue;
    const half = r.w / 2, cL = -half + r.sl, cR = half - r.sr, W = '#f0f';
    if (!r.o) {
      const mid = (cL + cR) / 2, side = Math.max(1, Math.floor(r.n / 2)), slw = (cR - cL) / 2 / side;
      if (r.n >= 2) line(g2, r, mid, '#fff', 0.3);     // orta çizgi (sarı)
      for (let k = 1; k < side; k++) { line(g1, r, mid - slw * k, W, 0.13, true); line(g1, r, mid + slw * k, W, 0.13, true); }
    } else {
      for (let k = 1; k < r.n; k++) line(g1, r, cL + r.lw * k, W, 0.13, true);
    }
    line(g1, r, cL, W, 0.14); line(g1, r, cR, W, 0.14);
  }
  // kavşak içinde boya yok (asfalt ve kaldırım korunur)
  for (const ji of t.junc) {
    const x = X(rd.junc[ji]), z = Z(rd.junc[ji + 1]), rad = (rd.junc[ji + 2] + 1.5) * BK;
    g1.globalCompositeOperation = 'multiply'; g1.fillStyle = 'rgb(255,255,0)';
    g1.beginPath(); g1.arc(x, z, rad, 0, Math.PI * 2); g1.fill(); g1.globalCompositeOperation = 'source-over';
    g2.fillStyle = '#000'; g2.beginPath(); g2.arc(x, z, rad, 0, Math.PI * 2); g2.fill();
  }
  const tex = (c, red) => {
    const tx = new THREE.CanvasTexture(c);
    if (red) tx.format = THREE.RedFormat;
    tx.flipY = false; tx.wrapS = tx.wrapT = THREE.ClampToEdgeWrapping;
    tx.minFilter = THREE.LinearMipmapLinearFilter; tx.magFilter = THREE.LinearFilter; tx.anisotropy = maxAniso;
    tx.onUpdate = () => { c.width = c.height = 0; };   // GPU'ya yüklendikten sonra tuvali bırak
    return tx;
  };
  return [tex(c1, false), tex(c2, true)];
}

function buildPatch(t, lod) {
  const { cell, res } = LODS[lod];
  const { n, m, h, bank, wall } = tileSurface(t, cell);
  const x0 = t.tx * BT - Wm / 2, z0 = t.tz * BT - Dm / 2;
  const nv = n * n + 4 * n, pos = new Float32Array(nv * 3), nor = new Float32Array(nv * 3), mod = new Float32Array(nv * 2);
  const hh = new Float32Array(n * n);
  let v = 0;
  for (let j = 0; j < n; j++) for (let i = 0; i < n; i++, v++) {
    const k = (j + 1) * m + i + 1, y = h[k];
    hh[v] = y;
    pos[3 * v] = x0 + i * cell; pos[3 * v + 1] = y; pos[3 * v + 2] = z0 + j * cell;
    const gx = (h[k - 1] - h[k + 1]) / (2 * cell), gz = (h[k - m] - h[k + m]) / (2 * cell), l = Math.hypot(gx, 1, gz);
    nor[3 * v] = gx / l; nor[3 * v + 1] = 1 / l; nor[3 * v + 2] = gz / l;
    mod[2 * v] = bank[k]; mod[2 * v + 1] = wall[k];
  }
  const q = n - 1, idx = new Uint16Array(q * q * 6 + 4 * q * 12);
  let c = 0;
  for (let j = 0; j < q; j++) for (let i = 0; i < q; i++) {
    const a = j * n + i, b = a + 1, d = a + n, f = d + 1;
    idx[c++] = a; idx[c++] = d; idx[c++] = b; idx[c++] = b; idx[c++] = d; idx[c++] = f;
  }
  const drop = cell * 2 + 3;
  const edge = (get) => {
    const base = v;
    for (let k = 0; k < n; k++, v++) {
      const s = get(k);
      pos[3 * v] = pos[3 * s]; pos[3 * v + 1] = pos[3 * s + 1] - drop; pos[3 * v + 2] = pos[3 * s + 2];
      nor[3 * v] = nor[3 * s]; nor[3 * v + 1] = nor[3 * s + 1]; nor[3 * v + 2] = nor[3 * s + 2];
      mod[2 * v] = mod[2 * s]; mod[2 * v + 1] = mod[2 * s + 1];
      if (k) {
        const a = get(k - 1), d = base + k - 1, f = base + k;
        idx[c++] = a; idx[c++] = d; idx[c++] = s; idx[c++] = s; idx[c++] = d; idx[c++] = f;
        idx[c++] = a; idx[c++] = s; idx[c++] = d; idx[c++] = s; idx[c++] = f; idx[c++] = d;
      }
    }
  };
  edge((k) => k); edge((k) => (n - 1) * n + k); edge((k) => k * n); edge((k) => k * n + n - 1);
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.BufferAttribute(nor, 3));
  g.setAttribute('aMod', new THREE.BufferAttribute(mod, 2));
  g.setIndex(new THREE.BufferAttribute(idx, 1));
  g.computeBoundingSphere();
  const hasRoad = t.roads.some((i) => !roads[i].bridge && !roads[i].tunnel);
  const tex = hasRoad ? bakeMasks(t, res) : null;
  const mesh = new THREE.Mesh(g, patchMaterial(tex ? tex[0] : blankTex, tex ? tex[1] : blankTex, x0 - BPAD, z0 - BPAD, 1 / BEXT));
  scene.add(mesh);
  if (t.patch) disposePatch(t);
  t.patch = { mesh, lod, cell, n, h: hh, tex };
  coverData[t.tz * NT + t.tx] = 255; coverTex.needsUpdate = true;
}
function disposePatch(t) {
  const p = t.patch;
  if (!p) return;
  scene.remove(p.mesh);
  p.mesh.geometry.dispose(); p.mesh.material.dispose();
  if (p.tex) { p.tex[0].dispose(); p.tex[1].dispose(); }
  t.patch = null;
  coverData[t.tz * NT + t.tx] = 0; coverTex.needsUpdate = true;
}
const livePatches = new Set();
const RFAR = LODS[LODS.length - 1].r;
// kameranın çevresindeki karoları üret / ayrıntı düzeyini değiştir / uzaktakileri sil
function streamPatches(px, pz, budgetMs) {
  const t0 = performance.now();
  const cx = Math.floor((px + Wm / 2) / BT), cz = Math.floor((pz + Dm / 2) / BT), rng = Math.ceil(RFAR / BT) + 1;
  const want = [];
  for (let a = cx - rng; a <= cx + rng; a++) for (let b = cz - rng; b <= cz + rng; b++) {
    if (a < 0 || b < 0 || a >= NTX || b >= NTZ) continue;
    const d = Math.hypot((a + 0.5) * BT - Wm / 2 - px, (b + 0.5) * BT - Dm / 2 - pz);
    if (d > RFAR) continue;
    const t = tileOf(a, b), lod = d < LODS[0].r ? 0 : d < LODS[1].r ? 1 : d < LODS[2].r ? 2 : 3;
    if (!t.patch) want.push([d, t, lod]);
    else if (t.patch.lod > lod && d < LODS[lod].r * 0.94) want.push([d, t, lod]);        // yaklaştı: incelt
    else if (t.patch.lod < lod && d > LODS[t.patch.lod].r * 1.15) want.push([d + 1e5, t, lod]);   // uzaklaştı: kabalaştır (acil değil)
  }
  want.sort((p, q) => p[0] - q[0]);
  for (const [, t, lod] of want) {
    buildPatch(t, lod); livePatches.add(t);
    if (performance.now() - t0 > budgetMs) break;
  }
  for (const t of livePatches) {
    if (Math.hypot((t.tx + 0.5) * BT - Wm / 2 - px, (t.tz + 0.5) * BT - Dm / 2 - pz) > RFAR + 350) { disposePatch(t); livePatches.delete(t); }
  }
}
// gerçek zemin (kesim-dolgu dahil): yakın karo varsa onun yüzeyi, yoksa doğal arazi
function groundAt(x, z) {
  const tx = Math.floor((x + Wm / 2) / BT), tz = Math.floor((z + Dm / 2) / BT);
  const t = tx >= 0 && tz >= 0 && tx < NTX && tz < NTZ ? tiles[tz * NTX + tx] : null;
  if (!t || !t.patch) return heightAt(x, z);
  const p = t.patch, fx = (x + Wm / 2 - tx * BT) / p.cell, fz = (z + Dm / 2 - tz * BT) / p.cell;
  const i = Math.min(p.n - 2, Math.floor(fx)), j = Math.min(p.n - 2, Math.floor(fz)), u = fx - i, v = fz - j, k = j * p.n + i;
  return (p.h[k] * (1 - u) + p.h[k + 1] * u) * (1 - v) + (p.h[k + p.n] * (1 - u) + p.h[k + p.n + 1] * u) * v;
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

// ---------------------------------------------------------------------------
// Köprüler ve tünel ağızları: ayrı 3B model (oyunlardaki ayrım: zemindeki yol arazinin parçası,
// köprü ayrı yapı). Kameranın çevresinde akış halinde üretilir.
// ---------------------------------------------------------------------------
const TILE = 1500, PIECE = 48, DECK = 1.4, CURB_H = 0.15;
const structTiles = new Map();
function structTile(x, z) {
  const tx = Math.floor(x / TILE), tz = Math.floor(z / TILE), key = `${tx},${tz}`;
  let t = structTiles.get(key);
  if (!t) structTiles.set(key, (t = { pieces: [], portals: [], group: null, cx: (tx + 0.5) * TILE, cz: (tz + 0.5) * TILE }));
  return t;
}
{
  const tunnelEnds = new Map();   // tünel uçları: başka tünel parçasına bağlanmayan uç = ağız
  const ek = (p) => `${Math.round(RX[p] * 4)},${Math.round(RZ[p] * 4)}`;
  for (const r of roads) if (r.tunnel) for (const p of [r.off, r.off + r.len - 1]) tunnelEnds.set(ek(p), (tunnelEnds.get(ek(p)) || 0) + 1);
  for (const r of roads) {
    if (r.tunnel) {
      for (const [p, q] of [[r.off, r.off + 1], [r.off + r.len - 1, r.off + r.len - 2]]) {
        if (tunnelEnds.get(ek(p)) !== 1) continue;
        const l = Math.hypot(RX[q] - RX[p], RZ[q] - RZ[p]) || 1;
        structTile(RX[p], RZ[p]).portals.push({ x: RX[p], z: RZ[p], y: RY[p], dx: (RX[q] - RX[p]) / l, dz: (RZ[q] - RZ[p]) / l, w: r.w });
      }
      continue;
    }
    if (!r.bridge) continue;
    // köprüyü 8 m aralıkla örnekle
    const xs = [], zs = [], ys = [];
    for (let p = r.off; p < r.off + r.len - 1; p++) {
      const n = Math.max(1, Math.ceil(Math.hypot(RX[p + 1] - RX[p], RZ[p + 1] - RZ[p]) / 8));
      for (let s = 0; s < n; s++) { const f = s / n; xs.push(RX[p] + (RX[p + 1] - RX[p]) * f); zs.push(RZ[p] + (RZ[p + 1] - RZ[p]) * f); ys.push(RY[p] + (RY[p + 1] - RY[p]) * f); }
    }
    const e = r.off + r.len - 1; xs.push(RX[e]); zs.push(RZ[e]); ys.push(RY[e]);
    const N = xs.length, dist = new Float32Array(N), dirx = new Float32Array(N), dirz = new Float32Array(N);
    for (let i = 1; i < N; i++) dist[i] = dist[i - 1] + Math.hypot(xs[i] - xs[i - 1], zs[i] - zs[i - 1]);
    for (let i = 0; i < N; i++) {
      const a = Math.max(0, i - 1), c = Math.min(N - 1, i + 1), l = Math.hypot(xs[c] - xs[a], zs[c] - zs[a]) || 1;
      dirx[i] = (xs[c] - xs[a]) / l; dirz[i] = (zs[c] - zs[a]) / l;
    }
    const R = { road: r, N, xs, zs, ys, dist, dirx, dirz };
    for (let a = 0; a < N - 1; a += PIECE - 1) {
      const b = Math.min(N, a + PIECE), m = (a + b) >> 1;
      structTile(xs[m], zs[m]).pieces.push({ R, a, e: b });
    }
  }
}

function emitBridge(rb, wb, { R, a, e }) {
  const { road, xs, zs, ys, dist, dirx, dirz } = R;
  const n = e - a, half = road.w / 2, up = [0, 1, 0], down = [0, -1, 0];
  const I = (j) => a + j;
  const px = (i, o) => xs[i] + dirz[i] * o, pz = (i, o) => zs[i] - dirx[i] * o;
  const at = (i, o, y, nrm, u, v) => [px(i, o), y, pz(i, o), nrm[0], nrm[1], nrm[2], u, v];
  const dir = (j) => [dirx[I(j)], dirz[I(j)]];
  const info2 = [road.sl, road.sr, half, 0], C = () => [1, 0, 0, 0];
  const top = (i) => ys[i] + 0.04;                 // zemindeki yolla birleşimde titreme olmasın
  const W = (f) => wb.strip(n, (j) => f(I(j)), dir, C, info2);
  // asfalt (şerit çizgileri shader'da)
  rb.strip(n, (j) => { const i = I(j); return [at(i, -half, top(i), up, dist[i], -half), at(i, half, top(i), up, dist[i], half)]; },
    dir, () => [road.n, road.o, road.lw, 1e6], info2);
  let outerL = -half, outerR = half;
  for (const side of [-1, 1]) {
    const out = (i) => [dirz[i] * side, 0, -dirx[i] * side], inw = (i) => [-dirz[i] * side, 0, dirx[i] * side];
    const eo = half * side;
    let o = eo, yT = top;
    if (road.s > 0) {                              // bordür + kaldırım
      const oo = (half + road.s) * side; o = oo; yT = (i) => top(i) + CURB_H;
      W((i) => [at(i, eo, top(i), inw(i), dist[i], 0), at(i, eo, yT(i), inw(i), dist[i], CURB_H)]);
      W((i) => [at(i, eo, yT(i), up, dist[i], eo), at(i, oo, yT(i), up, dist[i], oo)]);
    }
    // korkuluk (beton parapet 1 m) + tabliye yan yüzü
    const ph = 1.0, o2 = o + 0.35 * side, yTop = yT;
    W((i) => [at(i, o, yTop(i), inw(i), dist[i], 0), at(i, o, yTop(i) + ph, inw(i), dist[i], ph)]);
    W((i) => [at(i, o, yTop(i) + ph, up, dist[i], 0), at(i, o2, yTop(i) + ph, up, dist[i], 0.35)]);
    W((i) => [at(i, o2, yTop(i) + ph, out(i), dist[i], ph), at(i, o2, ys[i] - DECK, out(i), dist[i], -DECK)]);
    if (side < 0) outerL = o2; else outerR = o2;
  }
  // tabliye alt yüzü
  W((i) => [at(i, outerL, ys[i] - DECK, down, dist[i], outerL), at(i, outerR, ys[i] - DECK, down, dist[i], outerR)]);
  // ayaklar: ~40 m'de bir, yerden 3 m'den yüksekse; geniş tabliyede iki ayak + başlık kirişi
  const width = outerR - outerL;
  const offs = width > 14 ? [outerL + width * 0.25, outerR - width * 0.25] : [(outerL + outerR) / 2];
  for (let j = 0; j < n; j++) {
    const i = I(j);
    if (Math.floor(dist[i] / 40) === Math.floor((i ? dist[i - 1] : -1) / 40)) continue;
    const yb = ys[i] - DECK;
    let any = false;
    for (const oc of offs) {
      const g = heightAt(px(i, oc), pz(i, oc));
      if (yb - g < 3) continue;
      pillar(wb, px(i, oc), pz(i, oc), dirx[i], dirz[i], yb, g - 1.5, info2); any = true;
    }
    if (any) beam(wb, i, outerL + 0.5, outerR - 0.5, yb, yb - 1.2, px, pz, dirx, dirz, info2);
  }
}

// tünel ağzı: beton alın duvarı + karanlık açıklık
function emitPortal(wb, dark, p) {
  const ow = p.w + 1.2, oh = 5.4, side = 4.5, topH = oh + 2.6, base = p.y - 1.0;
  const nx = -p.dx, nz = -p.dz, rx = p.dz, rz = -p.dx;          // n: dışarı bakan yüz, r: sağ
  const cx = p.x + p.dx * 0.4, cz = p.z + p.dz * 0.4;
  const quad = (o0, o1, y0, y1) => wb.strip(2, (j) => {
    const o = j ? o1 : o0;
    return [[cx + rx * o, y0, cz + rz * o, nx, 0, nz, o, y0 - p.y], [cx + rx * o, y1, cz + rz * o, nx, 0, nz, o, y1 - p.y]];
  }, () => [rx, rz], () => [1, 0, 0, 0], [0, 0, 0, 0]);
  quad(-ow / 2 - side, -ow / 2, base, p.y + topH);               // sol ayak
  quad(ow / 2, ow / 2 + side, base, p.y + topH);                 // sağ ayak
  quad(-ow / 2, ow / 2, p.y + oh, p.y + topH);                   // üst kiriş
  const bx = cx + p.dx * 0.15, bz = cz + p.dz * 0.15, b = dark.length / 3;
  for (const [o, y] of [[-ow / 2, p.y - 0.2], [ow / 2, p.y - 0.2], [ow / 2, p.y + oh], [-ow / 2, p.y + oh]]) dark.push(bx + rx * o, y, bz + rz * o);
  dark.idx.push(b, b + 1, b + 2, b, b + 2, b + 3);
}

let roadMat, walkMat;
const darkMat = new THREE.MeshBasicMaterial({ color: 0x050607, side: THREE.DoubleSide, fog: true });
const STRUCT_R = isTouch ? 2600 : 4500;
function streamStructs(px, pz, budgetMs) {
  const t0 = performance.now();
  roadMat ||= roadMaterial(); walkMat ||= walkMaterial();
  for (const t of structTiles.values()) {
    const d = Math.hypot(t.cx - px, t.cz - pz) - TILE * 0.7;
    if (d < STRUCT_R && !t.group) {
      const rb = new StripBuilder(), wb = new StripBuilder(), dark = []; dark.idx = [];
      for (const pc of t.pieces) emitBridge(rb, wb, pc);
      for (const p of t.portals) emitPortal(wb, dark, p);
      const g = new THREE.Group(), a = rb.mesh(roadMat), b = wb.mesh(walkMat);
      if (a) g.add(a);
      if (b) g.add(b);
      if (dark.length) {
        const dg = new THREE.BufferGeometry();
        dg.setAttribute('position', new THREE.Float32BufferAttribute(dark, 3)); dg.setIndex(dark.idx);
        g.add(new THREE.Mesh(dg, darkMat));
      }
      scene.add(g); t.group = g;
      if (performance.now() - t0 > budgetMs) break;
    } else if (d > STRUCT_R + 900 && t.group) {
      scene.remove(t.group); t.group.traverse((o) => o.geometry && o.geometry.dispose()); t.group = null;
    }
  }
}

// ---------------------------------------------------------------------------
// Başlangıç kamerası ve kamera modları
//   uçuş (fly): telefonda varsayılan; sol baş parmak hareket, sağ baş parmak bakış
//   harita (orbit): masaüstünde varsayılan; fare ile kaydır / döndür / yakınlaş
// ---------------------------------------------------------------------------
let [cx, cz] = toWorld(Number(params.get('lat') || meta.start[0]), Number(params.get('lon') || meta.start[1]));
const camDist = Number(params.get('d') || (isTouch ? 500 : 1800));
const az = Number(params.get('az') || 215) * Math.PI / 180;       // kameranın hedefe göre yönü (derece, kuzeyden saat yönü)
controls.target.set(cx, heightAt(cx, cz), cz);
camera.position.set(cx + Math.sin(az) * camDist * 0.85, heightAt(cx, cz) + camDist * Number(params.get('hy') || 0.5), cz - Math.cos(az) * camDist * 0.85);
controls.update();

camera.rotation.order = 'YXZ';
let mode = params.get('mode') || (isTouch ? 'fly' : 'orbit');
let yaw = 0, pitch = 0;
const tmpV = new THREE.Vector3();
function anglesFromCamera() {
  camera.getWorldDirection(tmpV);
  yaw = Math.atan2(-tmpV.x, -tmpV.z);
  pitch = Math.asin(THREE.MathUtils.clamp(tmpV.y, -1, 1));
}
// ---- arayüz ----
const ui = document.createElement('div');
ui.id = 'ui';
ui.innerHTML = `
  <div id="joy"><div id="knob"></div></div>
  <div class="btn" id="bMode" title="Kamera modu">🗺</div>
  <div class="btn" id="bUp">▲</div>
  <div class="btn" id="bDown">▼</div>
  <div class="btn" id="bSpeed"></div>
  <div id="hint">Sol baş parmak: ekrana bas ve sürükle → ilerle<br>Sağ baş parmak: sürükle → etrafına bak<br>▲ ▼: yüksel / alçal</div>`;
document.body.appendChild(ui);
const $ = (id) => document.getElementById(id);
const SPEEDS = [15, 60, 200, 600];            // m/s
let speedIdx = params.has('speed') ? Number(params.get('speed')) : 1;
const joy = { x: 0, y: 0 }, hold = { up: false, down: false };
const refreshUi = () => {
  const fly = mode === 'fly';
  for (const id of ['bUp', 'bDown', 'bSpeed']) $(id).style.display = fly ? 'grid' : 'none';
  $('bSpeed').textContent = `${SPEEDS[speedIdx]}\nm/s`;
  $('bMode').textContent = fly ? '🗺' : '✈';
  $('hint').style.display = fly && isTouch ? 'block' : 'none';
};
function setMode(m) {
  mode = m;
  controls.enabled = m === 'orbit';
  if (m === 'fly') anglesFromCamera();
  else {
    // bakılan yeri hedef yap (en çok 3 km ileri)
    camera.getWorldDirection(tmpV);
    const agl = Math.max(30, camera.position.y - heightAt(camera.position.x, camera.position.z));
    const dist = Math.min(3000, agl / Math.max(0.15, -tmpV.y));
    controls.target.copy(camera.position).addScaledVector(tmpV, dist);
    controls.target.y = heightAt(controls.target.x, controls.target.z);
  }
  refreshUi();
}
controls.enabled = mode === 'orbit';
if (mode === 'fly') anglesFromCamera();
refreshUi();
setTimeout(() => { $('hint').style.opacity = 0; }, 9000);

$('bMode').addEventListener('click', () => setMode(mode === 'fly' ? 'orbit' : 'fly'));
$('bSpeed').addEventListener('click', () => { speedIdx = (speedIdx + 1) % SPEEDS.length; refreshUi(); });
for (const [id, key] of [['bUp', 'up'], ['bDown', 'down']]) {
  const el = $(id);
  el.addEventListener('pointerdown', (e) => { hold[key] = true; try { el.setPointerCapture(e.pointerId); } catch {} e.preventDefault(); });
  const off = () => { hold[key] = false; };
  el.addEventListener('pointerup', off); el.addEventListener('pointercancel', off); el.addEventListener('lostpointercapture', off);
}

// ---- dokunma / fare: sol bölge = joystick, diğer her yer = bakış ----
const cv = renderer.domElement;
cv.style.touchAction = 'none';
const ptrs = new Map();
const JR = 56; // joystick yarıçapı (px)
cv.addEventListener('pointerdown', (e) => {
  if (mode !== 'fly') return;
  try { cv.setPointerCapture(e.pointerId); } catch {}
  const joyFree = ![...ptrs.values()].some((p) => p.role === 'joy');
  const role = e.pointerType === 'touch' && joyFree && e.clientX < innerWidth * 0.5 && e.clientY > innerHeight * 0.3 ? 'joy' : 'look';
  ptrs.set(e.pointerId, { role, sx: e.clientX, sy: e.clientY, lx: e.clientX, ly: e.clientY });
  if (role === 'joy') {
    $('joy').style.display = 'block'; $('joy').style.left = e.clientX + 'px'; $('joy').style.top = e.clientY + 'px';
    $('knob').style.transform = 'translate(0,0)';
  }
});
cv.addEventListener('pointermove', (e) => {
  const p = ptrs.get(e.pointerId);
  if (!p || mode !== 'fly') return;
  if (p.role === 'joy') {
    let dx = e.clientX - p.sx, dy = e.clientY - p.sy;
    const l = Math.hypot(dx, dy);
    if (l > JR) { dx *= JR / l; dy *= JR / l; }
    joy.x = dx / JR; joy.y = -dy / JR;
    $('knob').style.transform = `translate(${dx}px,${dy}px)`;
  } else {
    yaw -= (e.clientX - p.lx) * 0.0045;
    pitch = THREE.MathUtils.clamp(pitch - (e.clientY - p.ly) * 0.0045, -1.5, 1.5);
    p.lx = e.clientX; p.ly = e.clientY;
  }
});
const endPtr = (e) => {
  const p = ptrs.get(e.pointerId);
  if (!p) return;
  if (p.role === 'joy') { joy.x = joy.y = 0; $('joy').style.display = 'none'; }
  ptrs.delete(e.pointerId);
};
cv.addEventListener('pointerup', endPtr); cv.addEventListener('pointercancel', endPtr);


document.getElementById('info').textContent = `${(Wm / 1000).toFixed(1)} × ${(Dm / 1000).toFixed(1)} km · ${roads.length} yol`;
document.getElementById('name').textContent = meta.name;
document.getElementById('attr').textContent = meta.attribution + ' Dokular: Poly Haven (CC0).';
document.title = meta.name;
const hudHelp = document.getElementById('help');
if (hudHelp) hudHelp.style.display = isTouch ? 'none' : 'block';

// ---------------------------------------------------------------------------
// Hareket döngüsü
// ---------------------------------------------------------------------------
const keys = new Set();
addEventListener('keydown', (e) => keys.add(e.code));
addEventListener('keyup', (e) => keys.delete(e.code));
const clock = new THREE.Clock();
const fwd = new THREE.Vector3(), right = new THREE.Vector3(), move = new THREE.Vector3();
let frame = 0;

function tick() {
  const dt = Math.min(clock.getDelta(), 0.1);
  uTime.value += dt;
  let sx, sz;
  if (mode === 'fly') {
    const boost = keys.has('ShiftLeft') || keys.has('ShiftRight') ? 3 : 1;
    const f = THREE.MathUtils.clamp(joy.y + (keys.has('KeyW') || keys.has('ArrowUp') ? 1 : 0) - (keys.has('KeyS') || keys.has('ArrowDown') ? 1 : 0), -1, 1);
    const r = THREE.MathUtils.clamp(joy.x + (keys.has('KeyD') || keys.has('ArrowRight') ? 1 : 0) - (keys.has('KeyA') || keys.has('ArrowLeft') ? 1 : 0), -1, 1);
    const v = (hold.up || keys.has('Space') || keys.has('KeyE') ? 1 : 0) - (hold.down || keys.has('KeyQ') ? 1 : 0);
    camera.rotation.set(pitch, yaw, 0);
    camera.getWorldDirection(fwd);
    right.crossVectors(fwd, camera.up).normalize();
    const sp = SPEEDS[speedIdx] * boost;
    move.set(0, 0, 0).addScaledVector(fwd, f).addScaledVector(right, r);
    if (move.lengthSq() > 1) move.normalize();
    camera.position.addScaledVector(move, sp * dt);
    camera.position.y += v * sp * 0.6 * dt;
    sx = camera.position.x; sz = camera.position.z;
  } else {
    const dist = camera.position.distanceTo(controls.target);
    const speed = dist * 0.8 * dt;
    camera.getWorldDirection(fwd); fwd.y = 0; fwd.normalize();
    right.crossVectors(fwd, camera.up).normalize();
    move.set(0, 0, 0);
    if (keys.has('KeyW') || keys.has('ArrowUp')) move.add(fwd);
    if (keys.has('KeyS') || keys.has('ArrowDown')) move.sub(fwd);
    if (keys.has('KeyD') || keys.has('ArrowRight')) move.add(right);
    if (keys.has('KeyA') || keys.has('ArrowLeft')) move.sub(right);
    if (move.lengthSq()) {
      move.normalize().multiplyScalar(speed);
      camera.position.add(move); controls.target.add(move);
    }
    sx = controls.target.x; sz = controls.target.z;
  }
  // harita sınırları içinde kal
  camera.position.x = THREE.MathUtils.clamp(camera.position.x, -Wm / 2, Wm / 2);
  camera.position.z = THREE.MathUtils.clamp(camera.position.z, -Dm / 2, Dm / 2);

  // akış: yakın arazi karoları (kesim-dolgu), köprüler, uzak arazi
  streamPatches(sx, sz, STILL ? 1e9 : 7);
  streamStructs(sx, sz, STILL ? 1e9 : 4);
  // kamera gerçek zeminin (kesim-dolgu dahil) altına girmesin
  const gy = groundAt(camera.position.x, camera.position.z) + 1.7;
  if (camera.position.y < gy) camera.position.y = gy;
  updateFar(camera.position);
  sky.position.copy(camera.position);

  if (mode === 'orbit') controls.update();
  renderer.render(scene, camera);
  if (++frame % 20 === 1) {
    const [la, lo] = toLatLon(camera.position.x, camera.position.z);
    $('pos').textContent = `${la.toFixed(4)}, ${lo.toFixed(4)} · yerden ${Math.round(camera.position.y - groundAt(camera.position.x, camera.position.z))} m`;
  }
  if (!STILL) requestAnimationFrame(tick);
}

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

const load = document.getElementById('load');
load.style.opacity = 0;
setTimeout(() => load.remove(), 500);
window.__cam = camera; window.__info = () => ({ ...renderer.info.render, geo: renderer.info.memory.geometries, tex: renderer.info.memory.textures }); // test için
window.__ready = true;
tick();
