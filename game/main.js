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
// Dokular (Poly Haven, CC0)
// ---------------------------------------------------------------------------
const texLoader = new THREE.TextureLoader();
const maxAniso = renderer.capabilities.getMaxAnisotropy();
const texPending = [];
function tex(path, srgb) {
  let done;
  texPending.push(new Promise((r) => (done = r)));
  const t = texLoader.load(path, done, undefined, done);
  t.wrapS = t.wrapT = THREE.RepeatWrapping;
  t.anisotropy = maxAniso;
  if (srgb) t.colorSpace = THREE.SRGBColorSpace;
  return t;
}
function pbr(name) {
  const base = `./textures/${name}/`;
  return { d: tex(base + 'diff.jpg', true), n: tex(base + 'nor_gl.jpg'), a: tex(base + 'arm.jpg') };
}
const T = {
  grass: pbr('forrest_ground_01'),
  forest: pbr('aerial_grass_rock'),
  rock: pbr('aerial_rocks_02'),
  asphalt: pbr('asphalt_02'),
};

// ---------------------------------------------------------------------------
// Arazi malzemesi: eğim ve yüksekliğe göre çim / orman / kaya karışımı
// ---------------------------------------------------------------------------
function terrainMaterial() {
  const mat = new THREE.MeshStandardMaterial({ roughness: 1, metalness: 0 });
  mat.onBeforeCompile = (sh) => {
    Object.assign(sh.uniforms, {
      gD: { value: T.grass.d }, gN: { value: T.grass.n }, gA: { value: T.grass.a },
      fD: { value: T.forest.d }, fN: { value: T.forest.n }, fA: { value: T.forest.a },
      rD: { value: T.rock.d }, rN: { value: T.rock.n }, rA: { value: T.rock.a },
    });
    sh.vertexShader = sh.vertexShader
      .replace('#include <common>', '#include <common>\nvarying vec3 vWP;\nvarying vec3 vWN;')
      .replace('#include <project_vertex>', `#include <project_vertex>
        vWP = (modelMatrix * vec4(transformed, 1.0)).xyz;
        vWN = normalize(mat3(modelMatrix) * objectNormal);`);

    sh.fragmentShader = sh.fragmentShader
      .replace('#include <common>', `#include <common>
        varying vec3 vWP;
        varying vec3 vWN;
        uniform sampler2D gD, gN, gA, fD, fN, fA, rD, rN, rA;
        vec3 W;            // karışım ağırlıkları: çim, orman, kaya
        vec3 blendN;       // dünya uzayı normal
        float blendR, blendAO;

        float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453); }
        float vnoise(vec2 p){
          vec2 i = floor(p), f = fract(p); f = f*f*(3.0-2.0*f);
          return mix(mix(hash(i), hash(i+vec2(1,0)), f.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y);
        }
        // Tekrar desenini kırmak için iki ölçekte örnekle
        vec4 tri(sampler2D t, vec2 uv){
          return mix(texture2D(t, uv), texture2D(t, uv * 0.137 + 0.31), 0.35);
        }`)
      .replace('#include <map_fragment>', `
        vec3 N0 = normalize(vWN);
        float slope = 1.0 - N0.y;
        float n1 = vnoise(vWP.xz * 0.004) * 0.6 + vnoise(vWP.xz * 0.02) * 0.4;
        float rock = smoothstep(0.30, 0.50, slope + (n1 - 0.5) * 0.15);
        float forest = smoothstep(55.0, 110.0, vWP.y + (n1 - 0.5) * 70.0) * (1.0 - rock);
        forest = max(forest, smoothstep(0.10, 0.22, slope) * (1.0 - rock) * 0.9);
        float grass = max(0.0, 1.0 - rock - forest);
        W = vec3(grass, forest, rock) / (grass + forest + rock + 1e-4);

        vec2 uvG = vWP.xz / 5.0, uvF = vWP.xz / 18.0, uvR = vWP.xz / 14.0;
        vec3 gC = tri(gD, uvG).rgb * vec3(0.85, 1.0, 0.7);
        // dağlar: yukarıdan bakınca ağaç örtüsü -> koyu yeşile çek
        vec3 fC = mix(tri(fD, uvF).rgb, vec3(0.10, 0.16, 0.06), 0.45);
        vec3 col = gC * W.x + fC * W.y + tri(rD, uvR).rgb * W.z;
        // uzakta renk değişimi (makro varyasyon)
        col *= 0.85 + 0.3 * vnoise(vWP.xz * 0.0015);
        diffuseColor.rgb *= col;

        vec3 arm = tri(gA, uvG).rgb * W.x + tri(fA, uvF).rgb * W.y + tri(rA, uvR).rgb * W.z;
        blendAO = arm.r; blendR = arm.g;

        vec3 tn = (tri(gN, uvG).xyz * W.x + tri(fN, uvF).xyz * W.y + tri(rN, uvR).xyz * W.z) * 2.0 - 1.0;
        vec3 Tg = normalize(vec3(1.0, 0.0, 0.0) - N0 * N0.x);
        vec3 Bt = normalize(cross(Tg, N0));
        blendN = normalize(Tg * tn.x + Bt * tn.y + N0 * tn.z);
      `)
      .replace('#include <roughnessmap_fragment>', 'float roughnessFactor = roughness * blendR;')
      .replace('#include <normal_fragment_maps>', `
        normal = normalize((viewMatrix * vec4(blendN, 0.0)).xyz);
      `)
      .replace('#include <aomap_fragment>', `
        reflectedLight.indirectDiffuse *= mix(1.0, blendAO, 0.8);
      `);
  };
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

// ---------------------------------------------------------------------------
// Yollar (OpenStreetMap verisi varsa)
// ---------------------------------------------------------------------------
async function buildRoads() {
  let data;
  try {
    const res = await fetch('./data/roads.json');
    if (!res.ok) return 0;
    data = await res.json();
  } catch { return 0; }

  const STEP = 10; // metre: araziye oturması için yolu bu aralıkla böl
  const pos = [], nor = [], uv = [], idx = [];
  const ORDER = { tertiary: 0, secondary: 1, primary: 2, trunk: 3, motorway: 4 };

  for (const road of data.roads) {
    const p = road.p, half = road.w / 2;
    // yeniden örnekle
    const pts = [];
    for (let i = 0; i < p.length - 2; i += 2) {
      const ax = p[i], az = p[i + 1], bx = p[i + 2], bz = p[i + 3];
      const n = Math.max(1, Math.ceil(Math.hypot(bx - ax, bz - az) / STEP));
      for (let s = 0; s < n; s++) pts.push([ax + (bx - ax) * s / n, az + (bz - az) * s / n]);
    }
    pts.push([p[p.length - 2], p[p.length - 1]]);
    if (pts.length < 2) continue;

    // yükseklik: köprüde uç noktalar arasında doğrusal, değilse arazi + pay
    const lift = 0.6 + ORDER[road.c] * 0.15 + Math.max(0, road.l) * 6;
    const ys = pts.map(([x, z]) => heightAt(x, z) + lift);
    if (road.b) {
      const y0 = ys[0], y1 = ys[ys.length - 1];
      for (let i = 0; i < ys.length; i++) ys[i] = Math.max(ys[i], y0 + (y1 - y0) * i / (ys.length - 1) + 4);
    }

    const base = pos.length / 3;
    let dist = 0;
    for (let i = 0; i < pts.length; i++) {
      const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
      let tx = b[0] - a[0], tz = b[1] - a[1];
      const tl = Math.hypot(tx, tz) || 1; tx /= tl; tz /= tl;
      const [x, z] = pts[i], y = ys[i];
      if (i > 0) dist += Math.hypot(x - pts[i - 1][0], z - pts[i - 1][1]);
      pos.push(x - tz * half, y, z + tx * half, x + tz * half, y, z - tx * half);
      nor.push(0, 1, 0, 0, 1, 0);
      const v = dist / 6;
      uv.push(0, v, road.w / 6, v);
      if (i > 0) {
        const q = base + (i - 1) * 2;
        idx.push(q, q + 2, q + 1, q + 1, q + 2, q + 3);
      }
    }
  }

  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('normal', new THREE.Float32BufferAttribute(nor, 3));
  g.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
  g.setIndex(idx);
  g.computeBoundingSphere();
  const mat = new THREE.MeshStandardMaterial({
    map: T.asphalt.d, normalMap: T.asphalt.n, roughnessMap: T.asphalt.a, aoMap: T.asphalt.a,
    roughness: 1, metalness: 0, side: THREE.DoubleSide,
    polygonOffset: true, polygonOffsetFactor: -2, polygonOffsetUnits: -4,
  });
  g.setAttribute('uv1', g.getAttribute('uv'));
  const mesh = new THREE.Mesh(g, mat);
  mesh.renderOrder = 1;
  scene.add(mesh);
  return data.roads.length;
}
const roadCount = await buildRoads();

// ---------------------------------------------------------------------------
// Başlangıç kamerası: Seul merkezi (Jongno), güneybatıdan bakış
// ---------------------------------------------------------------------------
function toWorld(lat, lon) {
  const [latMin, lonMin, latMax] = meta.bbox;
  const mlon = 111320 * Math.cos(((meta.bbox[0] + meta.bbox[2]) / 2) * Math.PI / 180);
  return [(lon - lonMin) * mlon - Wm / 2, (latMax - lat) * 111320 - Dm / 2];
}
const [cx, cz] = toWorld(37.556, 126.985);
const params = new URLSearchParams(location.search);
const camDist = Number(params.get('d') || 2500);
controls.target.set(cx, heightAt(cx, cz), cz);
camera.position.set(cx - camDist * 0.5, heightAt(cx, cz) + camDist * 0.55, cz + camDist * 0.8);
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

  controls.update();
  renderer.render(scene, camera);
  if (!params.has('still')) requestAnimationFrame(tick); // ?still: tek kare (test için)
}

addEventListener('resize', () => {
  camera.aspect = innerWidth / innerHeight;
  camera.updateProjectionMatrix();
  renderer.setSize(innerWidth, innerHeight);
});

// tüm dokular yüklenene kadar bekle
await Promise.race([Promise.all(texPending), new Promise((r) => setTimeout(r, 20000))]);
const load = document.getElementById('load');
load.style.opacity = 0;
setTimeout(() => load.remove(), 500);
window.__ready = true;
tick();
