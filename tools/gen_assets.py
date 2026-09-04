#!/usr/bin/env python3
"""Generate C data for the GBA build into assets/generated (no external dependencies).

- Sprites: PNGs from assets/ downscaled by area averaging, quantized to a shared 15-colour
  palette (index 0 = transparent), emitted as 4bpp 8x8 tiles in OAM tile order.
- Skyline: procedural 480x72 silhouette (same generator family as the Godot version),
  emitted as 8-bit pixels: 0 = sky, 1 = building.
"""
import math
import random
import struct
import zlib
from pathlib import Path

RACINE = Path(__file__).resolve().parent.parent
ASSETS = RACINE / "assets"
SORTIE = ASSETS / "generated"
TOWN_W, TOWN_H = 480, 72


# ---------------------------------------------------------------- PNG reader
def lire_png(chemin):
    d = Path(chemin).read_bytes()
    pos, idat, palette = 8, b"", None
    while pos < len(d):
        ln = struct.unpack(">I", d[pos:pos + 4])[0]
        typ, body = d[pos + 4:pos + 8], d[pos + 8:pos + 8 + ln]
        pos += 12 + ln
        if typ == b"IHDR":
            w, h, bd, ct = struct.unpack(">IIBB", body[:10])
        elif typ == b"PLTE":
            palette = [tuple(body[i:i + 3]) for i in range(0, len(body), 3)]
        elif typ == b"IDAT":
            idat += body
    assert bd == 8, "8-bit PNG expected"
    bpp = {6: 4, 2: 3, 3: 1, 0: 1}[ct]
    raw = zlib.decompress(idat)
    stride = w * bpp
    rows, prev, i = [], bytearray(stride), 0
    for _ in range(h):
        f = raw[i]; i += 1
        line = bytearray(raw[i:i + stride]); i += stride
        for x in range(stride):
            a = line[x - bpp] if x >= bpp else 0
            b = prev[x]
            c = prev[x - bpp] if x >= bpp else 0
            if f == 1: line[x] = (line[x] + a) & 255
            elif f == 2: line[x] = (line[x] + b) & 255
            elif f == 3: line[x] = (line[x] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                line[x] = (line[x] + (a if pa <= pb and pa <= pc else (b if pb <= pc else c))) & 255
        rows.append(bytes(line)); prev = line
    px = []
    for y in range(h):
        ligne = []
        for x in range(w):
            p = rows[y][x * bpp:(x + 1) * bpp]
            if ct == 6: ligne.append((p[0], p[1], p[2], p[3]))
            elif ct == 2: ligne.append((p[0], p[1], p[2], 255))
            elif ct == 3: ligne.append(palette[p[0]] + (255,))
            else: ligne.append((p[0], p[0], p[0], 255))
        px.append(ligne)
    return w, h, px


# ---------------------------------------------------------------- image ops
def reduire(px, w, h, tw, th):
    """Area-average downscale with alpha weighting."""
    out = []
    for ty in range(th):
        ligne = []
        y0, y1 = ty * h // th, max(ty * h // th + 1, (ty + 1) * h // th)
        for tx in range(tw):
            x0, x1 = tx * w // tw, max(tx * w // tw + 1, (tx + 1) * w // tw)
            r = g = b = a = n = 0
            for y in range(y0, y1):
                for x in range(x0, x1):
                    pr, pg, pb, pa = px[y][x]
                    r += pr * pa; g += pg * pa; b += pb * pa; a += pa; n += 1
            if a == 0:
                ligne.append((0, 0, 0, 0))
            else:
                ligne.append((r // a, g // a, b // a, a // n))
        out.append(ligne)
    return out


def rgb15(c):
    return (c[0] >> 3) | ((c[1] >> 3) << 5) | ((c[2] >> 3) << 10)


def quantifier(images, nb=15):
    """Median-cut palette over all opaque pixels of all images; returns palette (RGB) and index maps."""
    pixels = [p[:3] for img in images for row in img for p in row if p[3] >= 128]
    boites = [pixels]
    while len(boites) < nb:
        boites.sort(key=lambda b: -len(b) * max(max(c[k] for c in b) - min(c[k] for c in b) for k in range(3)) if b else 0)
        b = boites.pop(0)
        if len(b) < 2:
            boites.append(b); break
        k = max(range(3), key=lambda k: max(c[k] for c in b) - min(c[k] for c in b))
        b.sort(key=lambda c: c[k])
        m = len(b) // 2
        boites += [b[:m], b[m:]]
    palette = [tuple(sum(c[k] for c in b) // len(b) for k in range(3)) if b else (0, 0, 0) for b in boites]
    while len(palette) < nb:
        palette.append((0, 0, 0))

    def index(c):
        return 1 + min(range(len(palette)), key=lambda i: sum((palette[i][k] - c[k]) ** 2 for k in range(3)))

    cartes = [[[index(p[:3]) if p[3] >= 128 else 0 for p in row] for row in img] for img in images]
    return palette, cartes


def tuiles_4bpp(carte, w, h):
    """Emit 8x8 tiles row-major (matches OBJ 1D mapping), 4bpp: two pixels per byte, low nibble first."""
    octets = bytearray()
    for ty in range(h // 8):
        for tx in range(w // 8):
            for y in range(8):
                for x in range(0, 8, 2):
                    a = carte[ty * 8 + y][tx * 8 + x]
                    b = carte[ty * 8 + y][tx * 8 + x + 1]
                    octets.append(a | (b << 4))
    return bytes(octets)


# ---------------------------------------------------------------- skylines
class Toile:
    def __init__(self):
        self.g = [bytearray(TOWN_W) for _ in range(TOWN_H)]

    def rect(self, x0, y0, x1, y1):
        for y in range(max(0, y0), min(TOWN_H, y1)):
            for x in range(max(0, x0), min(TOWN_W, x1)):
                self.g[y][x] = 1

    def triangle(self, xg, xd, y_base, y_sommet):
        xc = (xg + xd) / 2
        for y in range(max(0, y_sommet), min(TOWN_H, y_base)):
            t = (y - y_sommet) / max(1, y_base - y_sommet)
            demi = (xd - xg) / 2 * t
            self.rect(int(xc - demi), y, int(xc + demi) + 1, y + 1)


def skyline_city(seed, hmin, hmax, wmin, wmax, tall_chance):
    random.seed(seed)
    t = Toile()
    t.rect(0, TOWN_H - 6, TOWN_W, TOWN_H)
    x = -4
    while x < TOWN_W:
        w = random.randint(wmin, wmax)
        h = random.randint(hmin, hmax)
        if random.random() < tall_chance:
            h = random.randint(TOWN_H - 14, TOWN_H - 3)
        y0 = TOWN_H - h
        t.rect(x, y0, x + w, TOWN_H)
        if random.random() < 0.5:
            r = random.randint(1, max(2, w // 4))
            t.rect(x + r, y0 - random.randint(3, 10), x + w - r, y0)
        if random.random() < 0.35:
            ax = x + random.randint(2, max(3, w - 2))
            t.rect(ax, max(0, y0 - random.randint(5, 14)), ax + 1, y0)
        x += w + random.randint(-1, 4)
    return t.g


def skyline_village():
    random.seed(77)
    t = Toile()
    t.rect(0, TOWN_H - 5, TOWN_W, TOWN_H)
    x = 4
    church = False
    while x < TOWN_W - 12:
        if not church and x > TOWN_W * 0.45:
            t.rect(x, TOWN_H - 24, x + 30, TOWN_H)
            t.triangle(x - 2, x + 32, TOWN_H - 24, TOWN_H - 34)
            t.rect(x + 30, TOWN_H - 40, x + 38, TOWN_H)
            t.triangle(x + 28, x + 40, TOWN_H - 40, TOWN_H - 58)
            x += 46
            church = True
            continue
        if random.random() < 0.2:  # tree
            cx = x + 4
            t.rect(cx - 1, TOWN_H - 14, cx + 1, TOWN_H)
            for dy in range(-5, 6):
                for dx in range(-5, 6):
                    if dx * dx + dy * dy <= 25:
                        t.rect(cx + dx, TOWN_H - 18 + dy, cx + dx + 1, TOWN_H - 17 + dy)
            x += 12
            continue
        w = random.randint(12, 22)
        h = random.randint(10, 20)
        y0 = TOWN_H - h
        t.rect(x, y0, x + w, TOWN_H)
        t.triangle(x - 1, x + w + 1, y0, y0 - random.randint(5, 9))
        if random.random() < 0.6:
            cx = x + random.randint(2, w - 3)
            t.rect(cx, y0 - random.randint(6, 10), cx + 2, y0)
        x += w + random.randint(2, 7)
    return t.g


SKYLINES = [
    ("skyline", lambda: skyline_city(2026, 16, TOWN_H - 10, 10, 26, 0.15)),
    ("metropolis", lambda: skyline_city(2024, 24, TOWN_H - 6, 8, 22, 0.3)),
    ("village", skyline_village),
]


# ---------------------------------------------------------------- C emit
def c_array(nom, octets, type_c="const unsigned char", par_ligne=24):
    lignes = [f"{type_c} {nom}[{len(octets)}] __attribute__((aligned(4))) = {{"]
    for i in range(0, len(octets), par_ligne):
        lignes.append("    " + ", ".join(f"0x{b:02X}" for b in octets[i:i + par_ligne]) + ",")
    lignes.append("};")
    return "\n".join(lignes)


def main():
    SORTIE.mkdir(parents=True, exist_ok=True)
    sprites = {
        "lion_idle": ("LionHead.png", 32, 32),
        "lion_puke": ("LionHeadVomit.png", 32, 32),
        "saucer": ("soucoupe.png", 32, 16),
        "ladybug": ("ladybug.png", 16, 16),
    }
    images, cles = [], []
    for cle, (fichier, tw, th) in sprites.items():
        w, h, px = lire_png(ASSETS / fichier)
        images.append(reduire(px, w, h, tw, th))
        cles.append((cle, tw, th))
    palette, cartes = quantifier(images, 15)

    c = ['#include "sprites.h"', ""]
    hdr = ["#pragma once", "// Generated by tools/gen_assets.py — do not edit.", ""]
    pal16 = [0] + [rgb15(p) for p in palette]
    c.append("const unsigned short sprites_pal[16] __attribute__((aligned(4))) = {" + ", ".join(f"0x{v:04X}" for v in pal16) + "};")
    hdr.append("extern const unsigned short sprites_pal[16];")
    for (cle, tw, th), carte in zip(cles, cartes):
        data = tuiles_4bpp(carte, tw, th)
        c.append(c_array(f"{cle}_tiles", data))
        hdr.append(f"extern const unsigned char {cle}_tiles[{len(data)}];  // {tw}x{th}, 4bpp, {tw // 8 * th // 8} tiles")
        hdr.append(f"#define {cle.upper()}_W {tw}")
        hdr.append(f"#define {cle.upper()}_H {th}")
    (SORTIE / "sprites.c").write_text("\n".join(c) + "\n")
    (SORTIE / "sprites.h").write_text("\n".join(hdr) + "\n")

    parts = ['#include "skyline.h"', ""]
    hdr2 = ["#pragma once", "// Generated by tools/gen_assets.py — do not edit.",
            f"#define TOWN_W {TOWN_W}", f"#define TOWN_H {TOWN_H}", f"#define NB_LEVELS {len(SKYLINES)}"]
    for i, (nom, gen) in enumerate(SKYLINES):
        plat = bytes(b for row in gen() for b in row)
        parts.append(c_array(f"skyline_{nom}", plat, par_ligne=48))
        hdr2.append(f"extern const unsigned char skyline_{nom}[{TOWN_W * TOWN_H}];  // 0 = sky, 1 = building")
        print(f"skyline {nom}: {sum(plat)} building pixels / {len(plat)}")
    parts.append("const unsigned char *const skylines[" + str(len(SKYLINES)) + "] = {" + ", ".join(f"skyline_{n}" for n, _ in SKYLINES) + "};")
    parts.append("const char *const level_names[" + str(len(SKYLINES)) + "] = {" + ", ".join(f'"{n.upper()}"' for n, _ in SKYLINES) + "};")
    hdr2 += [f"extern const unsigned char *const skylines[{len(SKYLINES)}];", f"extern const char *const level_names[{len(SKYLINES)}];", ""]
    (SORTIE / "skyline.c").write_text("\n".join(parts) + "\n")
    (SORTIE / "skyline.h").write_text("\n".join(hdr2))
    print("palette:", ["#%02x%02x%02x" % p for p in palette])


if __name__ == "__main__":
    main()
