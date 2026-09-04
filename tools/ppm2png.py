#!/usr/bin/env python3
"""Convert a binary PPM (P6) to PNG, optionally scaling up by an integer factor (nearest)."""
import struct
import sys
import zlib


def lire_ppm(chemin):
    data = open(chemin, "rb").read()
    parts = data.split(maxsplit=4)
    assert parts[0] == b"P6"
    w, h = int(parts[1]), int(parts[2])
    pixels = parts[4][: w * h * 3]
    return w, h, pixels


def ecrire_png(chemin, w, h, pixels, echelle=1):
    lignes = []
    for y in range(h):
        ligne = pixels[y * w * 3:(y + 1) * w * 3]
        if echelle > 1:
            ligne = b"".join(ligne[x * 3:x * 3 + 3] * echelle for x in range(w))
        for _ in range(echelle):
            lignes.append(b"\x00" + ligne)
    raw = b"".join(lignes)

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    png = b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w * echelle, h * echelle, 8, 2, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")
    open(chemin, "wb").write(png)


if __name__ == "__main__":
    src, dst = sys.argv[1], sys.argv[2]
    echelle = int(sys.argv[3]) if len(sys.argv) > 3 else 1
    w, h, px = lire_ppm(src)
    ecrire_png(dst, w, h, px, echelle)
    print(f"{dst} {w * echelle}x{h * echelle}")
