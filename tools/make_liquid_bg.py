#!/usr/bin/env python3
"""Gera o fundo 'Liquid Chrome' (metal líquido) para as skins do SYNTHKEY.
Uso: python3 tools/make_liquid_bg.py saida.jpg [largura altura seed]
Render procedural (domain warping + normais + reflexão em ambiente), sem imagens de terceiros."""
import sys, numpy as np
from scipy.ndimage import zoom, gaussian_filter
from PIL import Image

out = sys.argv[1] if len(sys.argv) > 1 else "liquid.jpg"
W = int(sys.argv[2]) if len(sys.argv) > 2 else 1600
H = int(sys.argv[3]) if len(sys.argv) > 3 else 900
seed = int(sys.argv[4]) if len(sys.argv) > 4 else 7
rng = np.random.default_rng(seed)

def field(cells_x, cells_y, smooth=0.0):
    f = rng.standard_normal((cells_y + 3, cells_x + 3))
    f = zoom(f, (H / cells_y, W / cells_x), order=3)[:H, :W]
    return f

def fbm(base, octs=4, gain=0.5):
    acc = np.zeros((H, W)); amp = 1.0; tot = 0
    for o in range(octs):
        acc += amp * field(max(2, int(base * 1.7 ** o)), max(2, int(base * 0.6 * 1.7 ** o)))
        tot += amp; amp *= gain
    return acc / tot

yy, xx = np.mgrid[0:H, 0:W].astype(np.float64)
u, v = xx / W, yy / H
# domain warp
wx = fbm(3, 3) * 0.22; wy = fbm(3, 3) * 0.22
uu = u + wx + 0.10 * np.sin(v * 5 + wy * 4); vv = v + wy
h = fbm(3, 5, 0.55)
# dobras diagonais (como tecido/metal escorrido)
h += 0.9 * np.sin((uu * 2.2 + vv * 3.1) * 3.1 + fbm(2, 3) * 2.6)
h += 0.5 * np.sin((uu * -1.3 + vv * 2.0) * 4.7 + wx * 9)
h = gaussian_filter(h, 2.0)

gy, gx = np.gradient(h)
s = 420.0 * 1600.0 / W
nx, ny = -gx * s, -gy * s
nz = np.ones_like(nx)
n = np.sqrt(nx * nx + ny * ny + nz * nz); nx /= n; ny /= n; nz /= n

def smoothstep(a, b, x):
    t = np.clip((x - a) / (b - a), 0, 1); return t * t * (3 - 2 * t)

# ambiente: gradiente vertical + 'softboxes' (reflexos brancos alongados)
def env(rx, ry):
    t = 0.5 + 0.5 * ry
    g = 0.04 + 0.16 * smoothstep(0.0, 1.0, t)               # grafite
    box1 = np.exp(-((rx - 0.55) ** 2) / 0.012 - ((ry + 0.25) ** 2) / 0.30)   # faixa clara
    box2 = np.exp(-((rx + 0.50) ** 2) / 0.020 - ((ry - 0.35) ** 2) / 0.18)
    return g, box1, box2

rx = nx * 2 * nz; ry = ny * 2 * nz
g, b1, b2 = env(rx, ry)

# paleta local (ciano em cima à direita, roxo/magenta na diagonal central, cobre à esquerda)
cyan   = np.array([0.30, 0.85, 0.95]); purple = np.array([0.62, 0.22, 0.95]); magenta = np.array([0.95, 0.30, 0.80])
copper = np.array([0.95, 0.60, 0.30]); steel  = np.array([0.55, 0.65, 0.72])
d = (u * 0.8 - v * 0.6)                                     # +1 canto sup. dir.
wc = smoothstep(0.05, 0.55, d + 0.12 * fbm(2, 3))
wp = np.exp(-((u * 0.55 + v * 0.8 - 0.62 + 0.10 * fbm(2, 3)) ** 2) / 0.020)
wk = smoothstep(0.35, 0.0, u + 0.2 * fbm(2, 3) - 0.12) * smoothstep(0.1, 0.5, v)
col = np.zeros((H, W, 3))
base = (g[..., None] * (0.55 * steel + 0.45 * np.array([0.35, 0.55, 0.6])))
col += base
col += (b1 * 1.15 + 0.55 * np.clip(-ry, 0, 1) ** 1.4 * wc)[..., None] * (wc[..., None] * cyan + (1 - wc[..., None]) * steel * 0.6)
col += (0.5 * np.clip(rx, 0, 1) ** 1.3 * wc)[..., None] * cyan * 0.8
col += (b2 * 1.1 + 0.5 * np.clip(ry, 0, 1) ** 1.3 * wk)[..., None] * (copper * wk[..., None] + steel * 0.4 * (1 - wk[..., None]))
col += (wp * (0.25 + 0.7 * b1 + 0.5 * np.clip(ry, 0, 1)))[..., None] * (0.6 * purple + 0.4 * magenta)
# brilho especular fino (cromado)
L = np.array([-0.45, -0.55, 0.70]); L /= np.linalg.norm(L)
spec = np.clip(nx * L[0] + ny * L[1] + nz * L[2], 0, 1) ** 90
L2 = np.array([0.6, -0.3, 0.74]); L2 /= np.linalg.norm(L2)
spec2 = np.clip(nx * L2[0] + ny * L2[1] + nz * L2[2], 0, 1) ** 140
col += (1.1 * spec + 0.7 * spec2)[..., None] * np.array([0.85, 0.98, 1.0])
# fresnel escuro nas cavas
cav = smoothstep(-0.6, 0.6, h / (np.abs(h).max() + 1e-9))
col *= (0.55 + 0.6 * cav)[..., None]
# vinheta + tom
vig = 1 - 0.55 * ((u - 0.5) ** 2 + (v - 0.5) ** 2) * 2
col *= vig[..., None]
col = np.clip(col, 0, None); col = col / (1 + 0.35 * col)          # tone map suave
col = np.clip(col ** (1 / 1.15), 0, 1)
noise = rng.standard_normal((H, W, 1)) * 0.004                      # dither (evita bandas)
img = (np.clip(col + noise, 0, 1) * 255).astype(np.uint8)
Image.fromarray(img).save(out, quality=88, subsampling=0)
print("ok", out, W, H)
