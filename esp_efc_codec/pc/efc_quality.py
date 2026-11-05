#!/usr/bin/env python3
"""Score a decoded EFC clip against its source frames.

Every metric uses the pixels a display would show (RGB565 expanded to 8-bit),
not the pre-encode working copy. Transparent color-key pixels are ignored.
"""
from __future__ import annotations

import os
import struct
import subprocess
import sys
import tempfile
import zlib

import numpy as np
from PIL import Image

PC_DIR = os.path.dirname(os.path.abspath(__file__))
EFC_TOOL = os.path.join(PC_DIR, "build", "efc_tool")
EFC_PY = os.path.join(PC_DIR, "efc_py.py")


def codec_path() -> tuple[str, str]:
    if os.path.isfile(EFC_TOOL):
        return "c", EFC_TOOL
    if os.path.isfile(EFC_PY):
        return "py", EFC_PY
    raise SystemExit("no decoder: build/efc_tool is missing and efc_py.py is missing")


def from_565(pix: np.ndarray) -> np.ndarray:
    r = (pix >> 11) & 31
    g = (pix >> 5) & 63
    b = pix & 31
    rgb = np.empty(pix.shape + (3,), np.uint8)
    rgb[..., 0] = (r << 3) | (r >> 2)
    rgb[..., 1] = (g << 2) | (g >> 4)
    rgb[..., 2] = (b << 3) | (b >> 2)
    return rgb


def decode_efc(path: str) -> tuple[np.ndarray, int | None]:
    """Return display RGB888 frames and the RGB565 color key (or None)."""
    kind, tool = codec_path()
    with open(path, "rb") as fh:
        blob = fh.read()
    if len(blob) < 20 or blob[:4] != b"EFC1":
        raise SystemExit(f"not an EFC1 file: {path}")
    if kind == "c":
        fd, rawp = tempfile.mkstemp(prefix="efc_q_", suffix=".rgb565")
        os.close(fd)
        try:
            proc = subprocess.run([tool, "decode", path, rawp], capture_output=True, text=True)
            if proc.returncode != 0:
                raise SystemExit((proc.stdout or "") + (proc.stderr or "") or "decode failed")
            raw = np.fromfile(rawp, dtype="<u2")
        finally:
            os.remove(rawp)
        w, h, _wp, _hp, frames, colors = struct.unpack_from("<HHHHHH", blob, 8)
        if raw.size != w * h * frames:
            raise SystemExit(f"decoded {raw.size} pixels, expected {w * h * frames}")
        pix = raw.reshape(frames, h, w)
        key = _color_key(blob, colors)
        return from_565(pix), key
    sys.path.insert(0, PC_DIR)
    import efc_py
    pix, info = efc_py.decode_frames(blob)
    return from_565(pix), info.get("color_key")


def _inflate_plane(payload: bytes, at: int) -> tuple[bytes, int]:
    if at + 8 > len(payload):
        raise ValueError("truncated EFC plane")
    raw_len, comp_len = struct.unpack_from("<II", payload, at)
    at += 8
    stored_len = comp_len or raw_len
    if stored_len > len(payload) - at:
        raise ValueError("truncated EFC plane")
    data = payload[at:at + stored_len]
    if comp_len:
        data = zlib.decompress(data, -15)
        if len(data) != raw_len:
            raise ValueError("EFC plane length mismatch")
    return data, at + stored_len


def _color_key(blob: bytes, colors: int):
    flags = blob[5]
    if (blob[7] & 1) and (flags & 0x01):
        return 0xF81F
    if flags & 0x01 or colors == 0:
        return None
    frames = struct.unpack_from("<H", blob, 16)[0]
    prefix = 20 + colors * 2 + frames * 9
    payload = blob[prefix:]
    if flags & 0x08:
        try:
            if flags & 0x04:
                payload, _at = _inflate_plane(payload, 0)
            else:
                if len(payload) < 8:
                    return None
                modes_len, lits_len = struct.unpack_from("<II", payload, 0)
                start = 8 + modes_len + lits_len
                if start > len(payload):
                    return None
                payload = payload[start:]
        except (ValueError, zlib.error):
            return None
    elif flags & 0x04:
        if len(payload) < 4:
            return None
        try:
            payload = zlib.decompress(payload[4:], -15)
        except zlib.error:
            return None
    if len(payload) < 2 or payload[0] != 0x13:
        return None
    index = payload[1]
    if index >= colors:
        return None
    pal = struct.unpack_from("<" + "H" * colors, blob, 20)
    return pal[index]


def luma(rgb: np.ndarray) -> np.ndarray:
    a = rgb.astype(np.float32)
    return 0.299 * a[..., 0] + 0.587 * a[..., 1] + 0.114 * a[..., 2]


def _opaque(key: np.ndarray | None, h: int, w: int) -> np.ndarray | None:
    if key is None:
        return None
    if key.ndim == 3:
        return ~key
    return np.broadcast_to(~key, (1, h, w))


def psnr(src: np.ndarray, dec: np.ndarray, opaque: np.ndarray | None,
         src16: np.ndarray | None = None, dec16: np.ndarray | None = None) -> float:
    if src16 is None:
        src16 = src.astype(np.int16)
    if dec16 is None:
        dec16 = dec.astype(np.int16)
    d = src16 - dec16
    squared = np.square(d, dtype=np.int32)
    if opaque is not None:
        mse = squared[opaque].mean() if opaque.any() else 0.0
    else:
        mse = float(squared.mean())
    if mse <= 1e-9:
        return 99.0
    return float(10.0 * np.log10(255.0 * 255.0 / mse))


def ssim(src: np.ndarray, dec: np.ndarray, opaque: np.ndarray | None,
         ys: np.ndarray | None = None, yd: np.ndarray | None = None) -> float:
    """Mean 8x8 SSIM on luma. Windows that are fully transparent are skipped."""
    if ys is None:
        ys = luma(src)
    if yd is None:
        yd = luma(dec)
    n, h, w = ys.shape
    hh, ww = (h // 8) * 8, (w // 8) * 8
    if hh < 8 or ww < 8:
        return 0.0
    x = ys[:, :hh, :ww].reshape(n, hh // 8, 8, ww // 8, 8)
    y = yd[:, :hh, :ww].reshape(n, hh // 8, 8, ww // 8, 8)
    mx = x.mean(axis=(2, 4))
    my = y.mean(axis=(2, 4))
    vx = (x * x).mean(axis=(2, 4)) - mx * mx
    vy = (y * y).mean(axis=(2, 4)) - my * my
    cv = (x * y).mean(axis=(2, 4)) - mx * my
    c1, c2 = 6.5025, 58.5225
    s = ((2 * mx * my + c1) * (2 * cv + c2)) / ((mx * mx + my * my + c1) * (vx + vy + c2) + 1e-12)
    if opaque is None:
        return float(s.mean())
    op = opaque[:, :hh:8, :ww:8]
    if op.shape != s.shape:
        op = opaque[:, :hh, :ww].reshape(n, hh // 8, 8, ww // 8, 8).mean(axis=(2, 4)) > 0.5
    if not op.any():
        return 0.0
    return float(s[op].mean())


def temporal_ripple(src: np.ndarray, dec: np.ndarray, opaque: np.ndarray | None,
                    src16: np.ndarray | None = None,
                    dec16: np.ndarray | None = None) -> float:
    """Mean |Δdecoded − Δsource| per channel. Crawling contours score high."""
    if len(src) < 2:
        return 0.0
    if src16 is None:
        src16 = src.astype(np.int16)
    if dec16 is None:
        dec16 = dec.astype(np.int16)
    ds = np.abs(src16[1:] - src16[:-1])
    dd = np.abs(dec16[1:] - dec16[:-1])
    err = np.abs(dd - ds).mean(axis=-1)
    if opaque is None:
        return float(err.mean())
    op = opaque[1:] if opaque.shape[0] == src.shape[0] else opaque
    if op.shape[0] != err.shape[0]:
        op = np.broadcast_to(op[-1:], err.shape)
    return float(err[op].mean()) if op.any() else float(err.mean())


def false_edges(src: np.ndarray, dec: np.ndarray, opaque: np.ndarray | None,
                ys: np.ndarray | None = None, yd: np.ndarray | None = None) -> float:
    """Fraction of opaque pixels where the decode invented a spatial edge."""
    if ys is None:
        ys = luma(src)
    if yd is None:
        yd = luma(dec)
    gs = np.abs(np.diff(ys.astype(np.int16), axis=2))
    gd = np.abs(np.diff(yd.astype(np.int16), axis=2))
    invented = (gd > 12.0) & (gs < 4.0)
    if opaque is None:
        return float(invented.mean())
    op = opaque[:, :, :-1]
    return float(invented[op].mean()) if op.any() else float(invented.mean())


def hold_stats(dec: np.ndarray, opaque: np.ndarray | None) -> tuple[float, float]:
    """Mean consecutive unchanged age, and 4x4 update-toggle rate."""
    n, h, w, _ = dec.shape
    if n < 2:
        return 0.0, 0.0
    same = np.all(dec[1:] == dec[:-1], axis=-1)
    if opaque is not None:
        op = opaque[1:] if opaque.shape[0] == n else opaque
        same = np.where(op, same, True)
    age = np.zeros(same.shape[1:], np.float64)
    total = 0.0
    for f in range(n - 1):
        age = np.where(same[f], age + 1.0, 0.0)
        total += float(age.mean())
    mean_age = total / (n - 1)

    s = 4
    th, tw = h // s, w // s
    if th == 0 or tw == 0:
        return mean_age, 0.0
    tiles = dec[:, :th * s, :tw * s]
    packed = tiles.reshape(n, th, s, tw, s, 3)
    tile_eq = np.all(packed[1:] == packed[:-1], axis=(2, 4, 5))
    toggles = tile_eq[1:] != tile_eq[:-1]
    return mean_age, float(toggles.mean()) if toggles.size else 0.0


def measure(src: np.ndarray, dec: np.ndarray, key: np.ndarray | None,
            max_side: int | None = None) -> dict:
    """Measure RGB888 clips, optionally sampling large frames on a regular grid."""
    n = min(len(src), len(dec))
    src = src[:n]
    dec = dec[:n]
    h, w = src.shape[1], src.shape[2]
    if max_side and max(h, w) > max_side:
        step = int(np.ceil(max(h, w) / max_side))
        offset = step // 2
        src = src[:, offset::step, offset::step]
        dec = dec[:, offset::step, offset::step]
        if key is not None:
            if key.ndim == 3:
                key = key[:, offset::step, offset::step]
            else:
                key = key[offset::step, offset::step]
        h, w = src.shape[1], src.shape[2]
    opaque = None
    if key is not None:
        opaque = np.broadcast_to(~key, (n, h, w))
    src16 = src.astype(np.int16)
    dec16 = dec.astype(np.int16)
    ys = luma(src)
    yd = luma(dec)
    mean_age, toggle = hold_stats(dec, opaque)
    # Sorting one packed integer per pixel is much faster than np.unique(...,
    # axis=0), which internally sorts a structured array of RGB triplets.
    flat = dec.reshape(-1, 3).astype(np.uint32, copy=False)
    packed = (flat[:, 0] << 16) | (flat[:, 1] << 8) | flat[:, 2]
    unique = int(np.unique(packed).size)
    return {
        "frames": n,
        "psnr": psnr(src, dec, opaque, src16, dec16),
        "ssim": ssim(src, dec, opaque, ys, yd),
        "ripple": temporal_ripple(src, dec, opaque, src16, dec16),
        "false_edges": false_edges(src, dec, opaque, ys, yd),
        "hold_age": mean_age,
        "tile_toggle": toggle,
        "unique": unique,
    }


def format_metrics(m: dict) -> str:
    return (f"PSNR={m['psnr']:.2f} SSIM={m['ssim']:.4f} ripple={m['ripple']:.3f} "
            f"false_edges={m['false_edges']:.4f} hold_age={m['hold_age']:.2f} "
            f"toggle={m['tile_toggle']:.4f} unique={m['unique']}")


def save_preview(src: np.ndarray, dec: np.ndarray, key: np.ndarray | None,
                 path: str, delay_ms: int = 67) -> None:
    """Side-by-side source | decoded EFC animated GIF."""
    n = min(len(src), len(dec))
    h, w = src.shape[1], src.shape[2]
    gap = np.full((h, 4, 3), 255, np.uint8)
    frames = []
    for i in range(n):
        a = src[i].copy()
        b = dec[i].copy()
        if key is not None:
            a[key] = (255, 255, 255)
            b[key] = (255, 255, 255)
        frames.append(Image.fromarray(np.concatenate([a, gap, b], 1)))
    frames[0].save(path, save_all=True, append_images=frames[1:],
                   duration=max(20, int(delay_ms)), loop=0, optimize=True)
