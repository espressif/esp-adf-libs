#!/usr/bin/env python3
"""Turn a short video into an EFC clip that stays under a byte budget.

The codec is lossless, so size comes from the pixels you feed it. This script
keeps the picture viewable by:

- denoising lossy sources (H.264, MJPEG, VP8/9, AV1) before quantizing
- optionally using an H.264 round trip as a lossy texture simplifier
- freezing pixels that barely move, when a large still region exists
- painting a shared OKLab palette (octree is --palette octree)
- locking palette indices in time so smooth ramps do not crawl
- holding still 4x4 tiles with hysteresis, a max age, and a motion guard
- keeping transparency as an EFC color key
- keeping every frame unless --allow-drop-frames is set

Floyd–Steinberg (--dither) and encoder dither_hold (--hold) stay as A/B
baselines. They add crawling grain and usually grow the file.

--via-gif paints a global palette with a position-locked Bayer dither
(the pixels a GIF would store), then EFC losslessly. Same look as the GIF,
usually a smaller file. The default path does not use it.

A JPEG q75 encode of the same frames is the size and picture reference.
Quality numbers always compare the source with the decoded EFC pixels.

Set EFC_FFMPEG to select the FFmpeg executable. EFC_FFPROBE overrides
ffprobe separately.
"""
from __future__ import annotations

import argparse
import json
import os
import shlex
import shutil
import subprocess
import sys
import tempfile
import time

import numpy as np
from PIL import Image

import efc_quality as qmet

PC_DIR = os.path.dirname(os.path.abspath(__file__))
EFC_TOOL = os.path.join(PC_DIR, "build", "efc_tool")
EFC_PY = os.path.join(PC_DIR, "efc_py.py")

KEY_RGB = np.array([255, 0, 255], np.uint8)
KEY_565 = 0xF81F
LOSSY = {"h264", "hevc", "mpeg4", "mpeg2video", "mjpeg", "msmpeg4v2", "msmpeg4v3",
         "vp8", "vp9", "av1", "h263", "flv1"}
_FFMPEG_FILTER_OPTION_CACHE: dict[tuple[str, str, str], bool] = {}


def ffmpeg_bin() -> str:
    return os.environ.get("EFC_FFMPEG") or os.environ.get("FFMPEG") or "ffmpeg"


def ffmpeg_filter_has_option(filter_name: str, option: str) -> bool:
    """Check an FFmpeg filter option because its API varies by build/version."""
    executable = ffmpeg_bin()
    cache_key = (executable, filter_name, option)
    if cache_key not in _FFMPEG_FILTER_OPTION_CACHE:
        try:
            proc = subprocess.run(
                [executable, "-hide_banner", "-h", f"filter={filter_name}"],
                capture_output=True, text=True, check=False,
            )
            help_text = (proc.stdout or "") + (proc.stderr or "")
            _FFMPEG_FILTER_OPTION_CACHE[cache_key] = any(
                line.split(maxsplit=1)[0] == option
                for line in help_text.splitlines() if line.split()
            )
        except OSError:
            _FFMPEG_FILTER_OPTION_CACHE[cache_key] = False
    return _FFMPEG_FILTER_OPTION_CACHE[cache_key]


def ffprobe_bin() -> str:
    return os.environ.get("EFC_FFPROBE") or os.environ.get("FFPROBE") or "ffprobe"


def probe(path: str) -> dict:
    cmd = [
        ffprobe_bin(), "-v", "error", "-select_streams", "v:0",
        "-show_entries", "stream=codec_name,width,height,pix_fmt,avg_frame_rate,duration,nb_frames",
        "-of", "json", path,
    ]
    st = json.loads(subprocess.check_output(cmd, text=True))["streams"][0]
    num, den = (st.get("avg_frame_rate") or "0/1").split("/")
    fps = float(num) / float(den) if float(den) else 0.0
    dur = float(st["duration"]) if st.get("duration") else 0.0
    pix = st.get("pix_fmt") or ""
    return {
        "codec": st.get("codec_name") or "",
        "w": int(st["width"]),
        "h": int(st["height"]),
        "pix": pix,
        "fps": fps,
        "duration": dur,
        "alpha": ("a" in pix) or pix in ("bgra", "rgba", "ya8", "yuva420p", "yuva444p"),
    }


def extract(path: str, info: dict, denoise: bool, scale_to_tile: bool = True) -> tuple[np.ndarray, np.ndarray | None]:
    w, h = info["w"], info["h"]
    vf = []
    if denoise:
        vf.append("atadenoise=0a=0.04:0b=0.08:1a=0.04:1b=0.08:2a=0.04:2b=0.08:s=9")
    if (w, h) != (info["w"], info["h"]):
        vf.append(f"scale={w}:{h}:flags=lanczos")
    use_alpha = info["alpha"]
    pix = "bgra" if use_alpha else "rgb24"
    vf.append(f"format={pix}")
    cmd = [ffmpeg_bin(), "-y", "-hide_banner", "-loglevel", "error", "-i", path, "-an"]
    if vf:
        cmd += ["-vf", ",".join(vf)]
    fd, raw_path = tempfile.mkstemp(prefix="efc_extract_", suffix=f".{pix}")
    os.close(fd)
    cmd += ["-vsync", "0", "-f", "rawvideo", "-pix_fmt", pix, raw_path]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
        with open(raw_path, "rb") as fh:
            raw = fh.read()
    finally:
        os.remove(raw_path)
    channels = 4 if use_alpha else 3
    frame = w * h * channels
    if len(raw) % frame:
        raise RuntimeError(
            f"FFmpeg extraction returned {len(raw)} bytes, not complete {w}x{h} {pix} frames"
        )
    if use_alpha:
        n = len(raw) // frame
        arr = np.frombuffer(raw[: n * frame], dtype=np.uint8).reshape(n, h, w, 4)
        rgb = np.ascontiguousarray(arr[..., [2, 1, 0]])
        key = arr[..., 3] < 128
        if not key.any():
            key = None
        return rgb, key
    n = len(raw) // frame
    rgb = np.frombuffer(raw[: n * frame], dtype=np.uint8).reshape(n, h, w, 3).copy()
    return rgb, None


def preprocess_h264(path: str, crf: int, width: int, height: int) -> str:
    """Lossily simplify a clip with x264; the caller owns the returned file."""
    fd, out = tempfile.mkstemp(prefix="efc_h264_", suffix=".mp4")
    os.close(fd)
    cmd = [
        ffmpeg_bin(), "-y", "-hide_banner", "-loglevel", "error",
        "-i", path, "-map", "0:v:0", "-an",
    ]
    if width % 2 or height % 2:
        cmd += [
            "-vf", f"pad={width + width % 2}:{height + height % 2}:0:0:color=black",
        ]
    cmd += [
        "-c:v", "libx264", "-preset", "slow", "-crf", str(crf),
        "-tune", "animation", "-pix_fmt", "yuv420p",
        "-vsync", "0", "-movflags", "+faststart", "-threads", "1", out,
    ]
    try:
        subprocess.run(cmd, check=True, stdout=subprocess.DEVNULL)
    except Exception:
        try:
            os.remove(out)
        except OSError:
            pass
        raise
    return out


def freeze_still(frames: np.ndarray, key: np.ndarray | None) -> np.ndarray:
    std = frames.astype(np.float32).std(axis=0).mean(axis=-1)
    still = std < 8.0
    if key is not None:
        transparent = key.any(axis=0) if key.ndim == 3 else key
        still = still & ~transparent
    if still.mean() < 0.15:
        return frames
    med = np.median(frames, axis=0).astype(np.uint8)
    out = frames.copy()
    out[:, still] = med[still]
    return out


def subset(frames: np.ndarray, nkeep: int) -> np.ndarray:
    n = len(frames)
    if nkeep >= n:
        return frames
    ix = np.unique(np.linspace(0, n - 1, nkeep).round().astype(int))
    return frames[ix]


def subset_key(key: np.ndarray | None, nkeep: int) -> np.ndarray | None:
    if key is None or key.ndim == 2:
        return key
    return subset(key, nkeep)


def frame_key(key: np.ndarray | None, index: int) -> np.ndarray | None:
    if key is None:
        return None
    return key[index] if key.ndim == 3 else key


def apply_color_key(frames: np.ndarray, key: np.ndarray | None,
                    copy: bool = True) -> np.ndarray:
    """Paint transparency and keep opaque RGB565 pixels away from the key."""
    if key is None:
        return frames
    out = frames.copy() if copy else frames
    mask = key if key.ndim == 3 else np.broadcast_to(key, out.shape[:3])
    packed = to_565(out)
    collision = (packed == KEY_565) & ~mask
    out[..., 2][collision] = 247
    out[mask] = KEY_RGB
    return out


def resize_clip(frames: np.ndarray, width: int, height: int) -> np.ndarray:
    """Bilinear resize for a coded-size candidate."""
    return np.stack([
        np.asarray(Image.fromarray(frame).resize((width, height), Image.BILINEAR))
        for frame in frames
    ])


def resize_key(key: np.ndarray | None, width: int, height: int) -> np.ndarray | None:
    if key is None:
        return None
    if key.ndim == 2:
        image = Image.fromarray(key.astype(np.uint8) * 255)
        return np.asarray(image.resize((width, height), Image.NEAREST)) >= 128
    return np.stack([
        np.asarray(Image.fromarray(mask.astype(np.uint8) * 255).resize(
            (width, height), Image.NEAREST
        )) >= 128
        for mask in key
    ])


def mark_scaled_efc(path: str, display_width: int, display_height: int,
                    output_tile: int) -> None:
    """Mark one explicit fixed-4 scale stage and its display geometry."""
    stage = {5: 1, 6: 2, 8: 3, 16: 4}.get(output_tile)
    if stage is None:
        raise ValueError(f"unsupported fixed scale output tile {output_tile}")
    with open(path, "rb") as fh:
        blob = bytearray(fh.read())
    if len(blob) < 20 or blob[:4] != b"EFC1" or blob[6] != 4:
        raise SystemExit("internal scaling requires a fixed-4 EFC")
    blob[5] |= 0x10
    blob[7] = (blob[7] & 0x01) | (stage << 1)
    blob[8] = display_width & 255
    blob[9] = (display_width >> 8) & 255
    blob[10] = display_height & 255
    blob[11] = (display_height >> 8) & 255
    with open(path, "wb") as fh:
        fh.write(blob)


def to_565(frames: np.ndarray) -> np.ndarray:
    a = frames.astype(np.uint16)
    r, g, b = a[..., 0], a[..., 1], a[..., 2]
    return (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3)).astype("<u2")


def from_565(pix: np.ndarray) -> np.ndarray:
    """Expand RGB565 the way a display does, so the sheet shows the stored colors."""
    r = (pix >> 11) & 31
    g = (pix >> 5) & 63
    b = pix & 31
    rgb = np.empty(pix.shape + (3,), np.uint8)
    rgb[..., 0] = (r << 3) | (r >> 2)
    rgb[..., 1] = (g << 2) | (g >> 4)
    rgb[..., 2] = (b << 3) | (b >> 2)
    return rgb


def snap_565(rgb: np.ndarray) -> np.ndarray:
    return from_565(to_565(rgb[None])[0]) if rgb.ndim == 2 else from_565(to_565(rgb))


def _srgb_to_linear(c: np.ndarray) -> np.ndarray:
    x = np.clip(c / 255.0, 0.0, 1.0)
    return np.where(x <= 0.04045, x / 12.92, ((x + 0.055) / 1.055) ** 2.4)


def _linear_to_srgb(c: np.ndarray) -> np.ndarray:
    x = np.clip(c, 0.0, 1.0)
    y = np.where(x <= 0.0031308, 12.92 * x, 1.055 * np.power(x, 1.0 / 2.4) - 0.055)
    return np.clip(np.round(y * 255.0), 0, 255).astype(np.uint8)


def rgb_to_oklab(rgb: np.ndarray) -> np.ndarray:
    lin = _srgb_to_linear(rgb.astype(np.float32))
    r, g, b = lin[..., 0], lin[..., 1], lin[..., 2]
    l = 0.4122214708 * r + 0.5363325363 * g + 0.0514459929 * b
    m = 0.2119034982 * r + 0.6806995451 * g + 0.1073969566 * b
    s = 0.0883024619 * r + 0.2817188376 * g + 0.6299787005 * b
    l_ = np.cbrt(np.maximum(l, 0.0))
    m_ = np.cbrt(np.maximum(m, 0.0))
    s_ = np.cbrt(np.maximum(s, 0.0))
    out = np.empty(rgb.shape[:-1] + (3,), np.float32)
    out[..., 0] = 0.2104542553 * l_ + 0.7936177850 * m_ - 0.0040720468 * s_
    out[..., 1] = 1.9779984951 * l_ - 2.4285922050 * m_ + 0.4505937099 * s_
    out[..., 2] = 0.0259040371 * l_ + 0.7827717662 * m_ - 0.8086757660 * s_
    return out


def oklab_to_rgb(lab: np.ndarray) -> np.ndarray:
    L, a, b = lab[..., 0], lab[..., 1], lab[..., 2]
    l_ = L + 0.3963377774 * a + 0.2158037573 * b
    m_ = L - 0.1055613458 * a - 0.0638541728 * b
    s_ = L - 0.0894841775 * a - 1.2914855480 * b
    l, m, s = l_ ** 3, m_ ** 3, s_ ** 3
    lin = np.empty(lab.shape, np.float32)
    lin[..., 0] = +4.0767416621 * l - 3.3077115913 * m + 0.2309699292 * s
    lin[..., 1] = -1.2684380046 * l + 2.6097574011 * m - 0.3413193965 * s
    lin[..., 2] = -0.0041960863 * l - 0.7034186147 * m + 1.7076147010 * s
    return _linear_to_srgb(lin)


def _nearest(lab: np.ndarray, pal: np.ndarray) -> np.ndarray:
    n = lab.shape[0]
    out = np.empty(n, np.int32)
    step = 4096
    for i in range(0, n, step):
        sl = lab[i:i + step]
        d = ((sl[:, None, :] - pal[None, :, :]) ** 2).sum(-1)
        out[i:i + sl.shape[0]] = d.argmin(1)
    return out


def _kmeans(samples: np.ndarray, k: int, iters: int = 12, seed: int = 0,
            weights: np.ndarray | None = None) -> np.ndarray:
    rng = np.random.RandomState(seed)
    n = samples.shape[0]
    k = min(k, n)
    w = np.ones(n, np.float64) if weights is None else weights.astype(np.float64, copy=False)
    pick = min(n, max(k * 8, 2048))
    init_pool_ix = rng.choice(n, pick, replace=False)
    init_pool = samples[init_pool_ix]
    centers = [init_pool[0]]
    for _ in range(1, k):
        c = np.stack(centers, 0)
        d2 = ((init_pool[:, None, :] - c[None, :, :]) ** 2).sum(-1).min(1)
        s = float(d2.sum())
        if s <= 1e-12:
            centers.append(init_pool[rng.randint(0, len(init_pool))])
        else:
            centers.append(init_pool[rng.choice(len(init_pool), p=d2 / s)])
    centers = np.stack(centers, 0).astype(np.float32)
    for _ in range(iters):
        labels = _nearest(samples, centers)
        sw = np.bincount(labels, weights=w, minlength=k)
        sums = np.empty((k, samples.shape[1]), np.float64)
        for c in range(samples.shape[1]):
            sums[:, c] = np.bincount(
                labels, weights=w * samples[:, c], minlength=k
            )
        nonempty = sw > 0
        centers[nonempty] = (sums[nonempty] / sw[nonempty, None]).astype(np.float32)
        for j in np.flatnonzero(~nonempty):
            centers[j] = samples[rng.randint(0, n)]
    return centers


_OKLAB_565 = None


def oklab_565_table() -> np.ndarray:
    """OKLab for every RGB565 value, indexed by the packed 16-bit code."""
    global _OKLAB_565
    if _OKLAB_565 is None:
        r = np.arange(32, dtype=np.uint16)
        g = np.arange(64, dtype=np.uint16)
        b = np.arange(32, dtype=np.uint16)
        rr, gg, bb = np.meshgrid(r, g, b, indexing="ij")
        rgb = np.empty((32, 64, 32, 3), np.uint8)
        rgb[..., 0] = (rr << 3) | (rr >> 2)
        rgb[..., 1] = (gg << 2) | (gg >> 4)
        rgb[..., 2] = (bb << 3) | (bb >> 2)
        _OKLAB_565 = rgb_to_oklab(rgb.reshape(-1, 1, 1, 3)).reshape(65536, 3)
    return _OKLAB_565


def build_palette_oklab(frames: np.ndarray, ncolors: int, key: np.ndarray | None) -> np.ndarray:
    """Weighted k-means over the clip's RGB565 histogram, in OKLab."""
    want = max(2, ncolors - (1 if key is not None else 0))
    pix = to_565(frames).reshape(-1)
    if key is not None:
        mask = np.broadcast_to(~key, frames.shape[:3]).reshape(-1)
        pix = pix[mask]
    counts = np.bincount(pix.astype(np.int32), minlength=65536).astype(np.float64)
    used = np.flatnonzero(counts)
    if used.size == 0:
        used = np.array([0], np.int64)
        counts[0] = 1
    table = oklab_565_table()
    # Up-weight mid-gradient colours: they are where contour bands show.
    rgb = from_565(used.astype("<u2"))
    y = qmet.luma(rgb.reshape(-1, 1, 1, 3)).reshape(-1)
    # Boost mid-luma pinks/ramps a little without starving highlights.
    boost = 1.0 + 0.6 * np.clip(1.0 - np.abs(y - 140.0) / 140.0, 0, 1)
    w = counts[used] * boost
    k = min(want, used.size)
    centers = _kmeans(table[used], k, weights=w)
    rgb = snap_565(oklab_to_rgb(centers.reshape(-1, 1, 1, 3)).reshape(-1, 3))
    uniq = np.unique(rgb, axis=0)
    if uniq.shape[0] < want:
        extra = from_565(used.astype("<u2"))
        order = np.argsort(-counts[used])
        extra = extra[order]
        merged = np.unique(np.concatenate([uniq, extra], 0), axis=0)
        uniq = merged[:want]
    if key is not None:
        uniq = uniq[~np.all(uniq == KEY_RGB, axis=1)]
        if uniq.shape[0] >= ncolors:
            uniq = uniq[: ncolors - 1]
        uniq = np.concatenate([uniq, KEY_RGB.reshape(1, 3)], 0)
    return uniq[:ncolors]


def build_palette_octree(frames: np.ndarray, ncolors: int, key: np.ndarray | None):
    ix = np.linspace(0, len(frames) - 1, min(10, len(frames))).astype(int)
    parts = []
    for i in ix:
        fr = frames[i].copy()
        mask = frame_key(key, i)
        if mask is not None:
            fr[mask] = 0
        parts.append(fr)
    q = Image.fromarray(np.concatenate(parts, 0)).quantize(
        colors=max(2, ncolors - 1), method=Image.FASTOCTREE, dither=Image.NONE
    )
    pal = list(q.getpalette()[: (ncolors - 1) * 3])
    if key is not None:
        pal[-3:] = [255, 0, 255]
    img = Image.new("P", (16, 16))
    img.putpalette(pal + [0] * (768 - len(pal)))
    rgb = np.array(pal, np.uint8).reshape(-1, 3)
    rgb = snap_565(rgb)
    if key is not None:
        rgb[-1] = KEY_RGB
    return rgb, img


def map_clip(frames: np.ndarray, pimg, key: np.ndarray | None, dither: bool = False) -> np.ndarray:
    mode = Image.FLOYDSTEINBERG if dither else Image.NONE
    out = np.empty_like(frames)
    for i, fr in enumerate(frames):
        q = np.array(Image.fromarray(fr).quantize(palette=pimg, dither=mode).convert("RGB"))
        mask = frame_key(key, i)
        if mask is not None:
            q[mask] = KEY_RGB
        out[i] = q
    return apply_color_key(out, key, copy=False)


# Position-locked 8x8 Bayer. Independent Floyd–Steinberg crawls; this pattern does not.
_BAYER8 = (np.array([
    [0, 32, 8, 40, 2, 34, 10, 42],
    [48, 16, 56, 24, 50, 18, 58, 26],
    [12, 44, 4, 36, 14, 46, 6, 38],
    [60, 28, 52, 20, 62, 30, 54, 22],
    [3, 35, 11, 43, 1, 33, 9, 41],
    [51, 19, 59, 27, 49, 17, 57, 25],
    [15, 47, 7, 39, 13, 45, 5, 37],
    [63, 31, 55, 23, 61, 29, 53, 21],
], np.float32) + 0.5) / 64.0 - 0.5


def bayer_bias(h: int, w: int, amp: float) -> np.ndarray:
    if amp <= 0:
        return np.zeros((h, w), np.float32)
    yy, xx = np.ogrid[:h, :w]
    return _BAYER8[yy & 7, xx & 7] * float(amp)


def _spatial_relax(idx: np.ndarray, lab: np.ndarray, pal_lab: np.ndarray,
                   spatial: float, key: np.ndarray | None) -> np.ndarray:
    """Prefer the left/above index when it is almost as close. Makes tiles flatter."""
    if spatial < 0:
        return idx
    h, w = idx.shape

    def dist2(cur_idx: np.ndarray) -> np.ndarray:
        return ((lab - pal_lab[cur_idx]) ** 2).sum(-1)

    def relaxed_limit(d2: np.ndarray) -> np.ndarray:
        # Exact squared form of sqrt(candidate) <= sqrt(current) + spatial.
        return d2 + 2.0 * spatial * np.sqrt(d2) + spatial * spatial

    out = idx
    for _ in range(2):
        limit = relaxed_limit(dist2(out))
        left = np.empty_like(out)
        left[:, 0] = out[:, 0]
        left[:, 1:] = out[:, :-1]
        pick = dist2(left) <= limit
        if key is not None:
            pick = pick & ~key
        out = np.where(pick, left, out)
        limit = relaxed_limit(dist2(out))
        above = np.empty_like(out)
        above[0] = out[0]
        above[1:] = out[:-1]
        pick = dist2(above) <= limit
        if key is not None:
            pick = pick & ~key
        out = np.where(pick, above, out)
    return out


def map_temporal(frames: np.ndarray, pal: np.ndarray, key: np.ndarray | None,
                 hyst: float, max_age: int, spatial: float = 0.012,
                 bayer: float = 0.0) -> np.ndarray:
    """Nearest OKLab index, kept when the previous color is still close enough.

    A position-locked Bayer bias is added before the lookup so smooth ramps
    become a static stipple instead of a moving iso-luma contour. Independent
    Floyd–Steinberg is the opposite: its error path changes every frame.
    """
    pal = np.ascontiguousarray(pal, dtype=np.uint8)
    pal_lab = rgb_to_oklab(pal.reshape(-1, 1, 1, 3)).reshape(-1, 3)
    table = oklab_565_table()
    n, h, w, _ = frames.shape
    out = np.empty_like(frames)
    key_idx = 0
    if key is not None:
        dkey = np.abs(pal.astype(np.int16) - KEY_RGB.reshape(1, 3)).sum(1)
        key_idx = int(dkey.argmin())
        pal = pal.copy()
        pal[key_idx] = KEY_RGB
        pal_lab = rgb_to_oklab(pal.reshape(-1, 1, 1, 3)).reshape(-1, 3)
        opaque_indices = np.delete(np.arange(len(pal), dtype=np.int32), key_idx)
        lut = opaque_indices[_nearest(table, pal_lab[opaque_indices])]
    else:
        lut = _nearest(table, pal_lab).astype(np.int32)
    bias = bayer_bias(h, w, bayer)
    hyst = max(0.0, float(hyst))

    # The common path has no temporal retention. Convert and map the complete
    # clip in one numpy batch instead of repeating float conversion and several
    # full-frame distance calculations in a Python frame loop.
    if hyst == 0.0:
        if bias.any():
            rgb = np.clip(frames.astype(np.float32) + bias[None, ..., None], 0, 255)
            pix = to_565(np.round(rgb).astype(np.uint8))
        else:
            pix = to_565(frames)
        mapped = lut[pix]
        for i in range(n):
            mask = frame_key(key, i)
            idx = _spatial_relax(mapped[i], table[pix[i]], pal_lab, spatial, mask)
            if mask is not None:
                idx[mask] = key_idx
            out[i] = pal[idx]
            if mask is not None:
                out[i][mask] = KEY_RGB
        return apply_color_key(out, key, copy=False)

    def lookup(fr: np.ndarray) -> tuple[np.ndarray, np.ndarray]:
        rgb = fr.astype(np.float32)
        if bias.any():
            rgb = np.clip(rgb + bias[..., None], 0, 255)
        pix = to_565(np.round(rgb).astype(np.uint8)[None])[0]
        lab = table[pix]
        return lut[pix], lab

    new_idx, lab0 = lookup(frames[0])
    mask = frame_key(key, 0)
    idx = _spatial_relax(new_idx, lab0, pal_lab, spatial, mask)
    if mask is not None:
        idx[mask] = key_idx
    out[0] = pal[idx]
    age = np.ones((h, w), np.int16)
    max_age = int(max_age)
    unlimited = max_age <= 0
    for i in range(1, n):
        mask = frame_key(key, i)
        cand, lab = lookup(frames[i])
        new_idx = _spatial_relax(cand, lab, pal_lab, spatial, mask)
        d_prev = np.sqrt(((lab - pal_lab[idx]) ** 2).sum(-1))
        d_new = np.sqrt(((lab - pal_lab[new_idx]) ** 2).sum(-1))
        src_l1 = np.abs(frames[i].astype(np.int16) - frames[i - 1].astype(np.int16)).sum(-1)
        keep = d_prev <= d_new + hyst
        keep |= (src_l1 < 6) & (d_prev <= d_new + hyst + 0.02)
        if not unlimited:
            keep &= age < max_age
        if mask is not None:
            keep = keep & ~mask
            new_idx = new_idx.copy()
            new_idx[mask] = key_idx
        idx = np.where(keep, idx, new_idx)
        age = np.where(keep, age + 1, np.int16(1))
        out[i] = pal[idx]
        if mask is not None:
            out[i][mask] = KEY_RGB
    return apply_color_key(out, key, copy=False)


def map_gif(frames: np.ndarray, pal: np.ndarray, key: np.ndarray | None,
            bayer: float = 16.0) -> np.ndarray:
    """Map like a GIF: one palette, position-locked Bayer, no temporal lock.

    Floyd–Steinberg changes every frame and kills EFC SKIP. Bayer is fixed to
    (x,y), so a still gradient stays a still stipple and EFC stays small.
    """
    pal = np.ascontiguousarray(pal, dtype=np.uint8)
    pal_lab = rgb_to_oklab(pal.reshape(-1, 1, 1, 3)).reshape(-1, 3)
    table = oklab_565_table()
    lut = _nearest(table, pal_lab).astype(np.int32)
    n, h, w, _ = frames.shape
    out = np.empty_like(frames)
    key_idx = 0
    if key is not None:
        dkey = np.abs(pal.astype(np.int16) - KEY_RGB.reshape(1, 3)).sum(1)
        key_idx = int(dkey.argmin())
        pal = pal.copy()
        pal[key_idx] = KEY_RGB
        pal_lab = rgb_to_oklab(pal.reshape(-1, 1, 1, 3)).reshape(-1, 3)
        opaque_indices = np.delete(np.arange(len(pal), dtype=np.int32), key_idx)
        lut = opaque_indices[_nearest(table, pal_lab[opaque_indices])]
    else:
        lut = _nearest(table, pal_lab).astype(np.int32)
    bias = bayer_bias(h, w, bayer)
    for i in range(n):
        rgb = frames[i].astype(np.float32)
        if bias.any():
            rgb = np.clip(rgb + bias[..., None], 0, 255)
        pix = to_565(np.round(rgb).astype(np.uint8)[None])[0]
        idx = lut[pix]
        mask = frame_key(key, i)
        if mask is not None:
            idx = idx.copy()
            idx[mask] = key_idx
        out[i] = pal[idx]
        if mask is not None:
            out[i][mask] = KEY_RGB
    return apply_color_key(out, key, copy=False)


def map_ffmpeg_palette(frames: np.ndarray, colors: int,
                       key: np.ndarray | None) -> np.ndarray:
    """Use FFmpeg's native palettegen/paletteuse instead of Python mapping."""
    n, h, w, _ = frames.shape
    bgra = np.empty((n, h, w, 4), np.uint8)
    bgra[..., 0] = frames[..., 2]
    bgra[..., 1] = frames[..., 1]
    bgra[..., 2] = frames[..., 0]
    bgra[..., 3] = 255
    if key is not None:
        mask = key if key.ndim == 3 else np.broadcast_to(key, (n, h, w))
        bgra[..., 3][mask] = 0

    td = tempfile.mkdtemp(prefix="efc_ffpal_")
    raw_path = os.path.join(td, "input.bgra")
    pal_path = os.path.join(td, "palette.png")
    out_path = os.path.join(td, "output.bgra")
    try:
        bgra.tofile(raw_path)
        common = [
            ffmpeg_bin(), "-y", "-hide_banner", "-loglevel", "error",
            "-f", "rawvideo", "-pixel_format", "bgra",
            "-video_size", f"{w}x{h}", "-framerate", "30",
            "-i", raw_path,
        ]
        reserve = 1 if key is not None else 0
        gen_options = [
            f"max_colors={max(4, min(256, int(colors)))}",
            f"reserve_transparent={reserve}",
            "transparency_color=ff00ff",
        ]
        use_options = ["dither=none", "alpha_threshold=128"]
        if key is not None:
            if ffmpeg_filter_has_option("palettegen", "use_alpha"):
                gen_options.append("use_alpha=1")
            if ffmpeg_filter_has_option("paletteuse", "use_alpha"):
                use_options.append("use_alpha=1")
        gen = common + [
            "-vf", f"palettegen={':'.join(gen_options)}",
            "-frames:v", "1", pal_path,
        ]
        subprocess.run(gen, check=True, stdout=subprocess.DEVNULL)
        use = common + [
            "-i", pal_path,
            "-filter_complex",
            f"[0:v][1:v]paletteuse={':'.join(use_options)},format=bgra",
            "-an", "-f", "rawvideo", "-pix_fmt", "bgra", out_path,
        ]
        subprocess.run(use, check=True, stdout=subprocess.DEVNULL)
        with open(out_path, "rb") as fh:
            raw = fh.read()
    finally:
        shutil.rmtree(td, ignore_errors=True)

    frame_bytes = h * w * 4
    expected = n * frame_bytes
    if len(raw) != expected:
        raise RuntimeError(f"FFmpeg palette mapping returned {len(raw)} bytes, "
                           f"expected {expected}")
    arr = np.frombuffer(raw, np.uint8).reshape(n, h, w, 4)
    rgb = np.ascontiguousarray(arr[..., [2, 1, 0]])
    return apply_color_key(rgb, key, copy=False)


def tile_hold(frames: np.ndarray, dz: float, key: np.ndarray | None) -> np.ndarray:
    if dz <= 0 or len(frames) < 2:
        return frames
    s = 4
    n, h, w, _ = frames.shape
    th, tw = h // s, w // s
    if th == 0 or tw == 0:
        return frames
    hh, ww = th * s, tw * s
    out = frames.copy()
    for i in range(1, n):
        prev, cur = out[i - 1], frames[i]
        d = np.abs(cur.astype(np.int16) - prev.astype(np.int16)).sum(-1)
        score = d[:hh, :ww].reshape(th, s, tw, s).mean(axis=(1, 3))
        mask = np.zeros((h, w), np.bool_)
        mask[:hh, :ww] = np.repeat(np.repeat(score < dz, s, 0), s, 1)
        alpha = frame_key(key, i)
        prev_alpha = frame_key(key, i - 1)
        if alpha is not None:
            mask &= ~alpha
        if prev_alpha is not None:
            mask &= ~prev_alpha
        held = np.where(mask[..., None], prev, cur)
        if alpha is not None:
            held[alpha] = KEY_RGB
        out[i] = held
    return apply_color_key(out, key, copy=False)


def tile_hold_stable(quant: np.ndarray, source: np.ndarray, dz: float,
                     key: np.ndarray | None, max_age: int = 10) -> np.ndarray:
    """Hold a 4x4 when it is cheap, with hysteresis, max age, and a motion guard.

    The enter threshold matches --deadzone (mean |ΔR|+|ΔG|+|ΔB| vs the previous
    output). Once a tile is held it stays held until the score exceeds 1.6× the
    enter value, the source motion around it is large, accumulated drift is
    high, or it has been frozen for max_age frames. That stops the on/off
    pumping of a hard 4x4 threshold without spending extra decoder cycles.
    """
    if dz <= 0 or len(quant) < 2:
        return quant
    s = 4
    n, h, w, _ = quant.shape
    th, tw = h // s, w // s
    if th == 0 or tw == 0:
        return quant
    hh, ww = th * s, tw * s
    out = quant.copy()
    enter = float(dz)
    exit_t = enter * 1.6
    max_age = max(1, int(max_age))
    holding = np.zeros((th, tw), np.bool_)
    age = np.zeros((th, tw), np.int16)
    acc = np.zeros((th, tw), np.float32)

    def tiles(a: np.ndarray) -> np.ndarray:
        return a[:hh, :ww].reshape(th, s, tw, s).mean(axis=(1, 3))

    def neighbor_max(a: np.ndarray) -> np.ndarray:
        m = a.copy()
        m[1:] = np.maximum(m[1:], a[:-1])
        m[:-1] = np.maximum(m[:-1], a[1:])
        m[:, 1:] = np.maximum(m[:, 1:], a[:, :-1])
        m[:, :-1] = np.maximum(m[:, :-1], a[:, 1:])
        return m

    for i in range(1, n):
        prev, cur = out[i - 1], quant[i]
        d = np.abs(cur.astype(np.int16) - prev.astype(np.int16)).sum(-1)
        score = tiles(d)
        mot = tiles(np.abs(source[i].astype(np.int16) - source[i - 1].astype(np.int16)).sum(-1))
        mot_n = neighbor_max(mot)
        start = (~holding) & (score < enter) & (mot < enter) & (mot_n < enter * 2.0)
        stay = holding & (score < exit_t) & (age < max_age) & (acc < enter * max_age) & (mot_n < exit_t * 2.0)
        holding = start | stay
        age = np.where(holding, age + 1, np.int16(0))
        acc = np.where(holding, acc + score.astype(np.float32), np.float32(0))
        mask = np.zeros((h, w), np.bool_)
        mask[:hh, :ww] = np.repeat(np.repeat(holding, s, 0), s, 1)
        alpha = frame_key(key, i)
        prev_alpha = frame_key(key, i - 1)
        if alpha is not None:
            mask &= ~alpha
        if prev_alpha is not None:
            mask &= ~prev_alpha
        held = np.where(mask[..., None], prev, cur)
        if alpha is not None:
            held[alpha] = KEY_RGB
        out[i] = held
    return apply_color_key(out, key, copy=False)


def codec_path() -> tuple[str, str]:
    """Prefer the C tool. Fall back to the shuffled Python codec."""
    if os.path.isfile(EFC_TOOL):
        return "c", EFC_TOOL
    if os.path.isfile(EFC_PY):
        return "py", EFC_PY
    raise SystemExit("no encoder: build/efc_tool is missing and efc_py.py is missing")


def encode(frames: np.ndarray, out_efc: str, key: np.ndarray | None,
           legacy: bool = False, hold: bool = False) -> tuple[int, str]:
    kind, tool = codec_path()
    frames = apply_color_key(frames, key)
    h, w = frames.shape[1], frames.shape[2]
    if kind == "c":
        fd, rawp = tempfile.mkstemp(suffix=".rgb565")
        os.close(fd)
        try:
            with open(rawp, "wb") as raw_file:
                for frame in frames:
                    np.ascontiguousarray(to_565(frame)).tofile(raw_file)
            cmd = [tool, "encode-raw", rawp, str(w), str(h), out_efc, "4", "z"]
            if key is not None:
                cmd.append(hex(KEY_565))
            if legacy:
                cmd.append("legacy")
            if hold:
                cmd.append("hold")
            proc = subprocess.run(cmd, capture_output=True, text=True)
            text = (proc.stdout or "") + (proc.stderr or "")
            if proc.returncode != 0 or "lossless=NO" in text:
                raise SystemExit(text[-500:] or "EFC round-trip failed")
            lines = [ln for ln in text.splitlines() if "lossless" in ln]
            line = lines[-1] if lines else "encoded"
            return os.path.getsize(out_efc), line
        finally:
            os.remove(rawp)
    sys.path.insert(0, PC_DIR)
    import efc_py
    rgb = np.ascontiguousarray(to_565(frames))
    t0 = time.perf_counter()
    key_565 = KEY_565 if key is not None else None
    blob = efc_py.encode_frames(
        rgb, tile=4, color_key=key_565, compress=True,
        legacy=legacy, hold=hold,
    )
    back, info = efc_py.decode_frames(blob)
    expect = efc_py.dither_hold(rgb, key_565) if hold else rgb
    if back.shape != expect.shape or not np.array_equal(back, expect):
        raise SystemExit("EFC round-trip failed")
    with open(out_efc, "wb") as fh:
        fh.write(blob)
    ms = (time.perf_counter() - t0) * 1000.0
    line = (
        f"{info['width']}x{info['height']} f={info['frame_count']} tile={info['tile']} "
        f"T={info['token_width']} pal={info['palette_count']} efc={len(blob)} "
        f"enc={ms:.0f}ms lossless=YES z={'ON' if info['compressed'] else 'off'} "
        f"alpha={'yes' if info['color_key'] is not None else 'no'}"
    )
    return len(blob), line


def jpeg_reference(frames: np.ndarray, key: np.ndarray | None, quality: int = 75) -> int:
    total = 0
    td = tempfile.mkdtemp(prefix="efc_jpg_")
    try:
        for i, fr in enumerate(frames):
            im = fr.copy()
            mask = frame_key(key, i)
            if mask is not None:
                im[mask] = (255, 255, 255)
            path = os.path.join(td, f"f{i:04d}.jpg")
            Image.fromarray(im).save(path, quality=quality, optimize=True)
            total += os.path.getsize(path)
    finally:
        shutil.rmtree(td, ignore_errors=True)
    return total


def comparison(src: np.ndarray, q: np.ndarray, efc: np.ndarray, key, path: str) -> None:
    def on_white(im, index):
        o = im.copy()
        mask = frame_key(key, index)
        if mask is not None:
            o[mask] = (255, 255, 255)
        return o

    n = len(efc)
    picks = [0, n // 3, (2 * n) // 3, n - 1]
    rows = []
    h, w = src.shape[1], src.shape[2]
    src_ix = np.unique(np.linspace(0, len(src) - 1, n).round().astype(int))
    for p in picks:
        si = int(src_ix[min(p, len(src_ix) - 1)])
        a = on_white(src[si], si)
        b = on_white(q[si] if si < len(q) else q[min(si, len(q) - 1)], si)
        c = on_white(efc[p], min(p, len(efc) - 1))
        gap = np.full((h, 4, 3), 255, np.uint8)
        rows.append(np.concatenate([a, gap, b, gap, c], 1))
    gap_y = np.full((4, rows[0].shape[1], 3), 255, np.uint8)
    sheet = rows[0]
    for r in rows[1:]:
        sheet = np.concatenate([sheet, gap_y, r], 0)
    Image.fromarray(sheet).save(path)


def _cmd(video: str, out_efc: str, cmp: str, extra: str) -> str:
    parts = ["python3", sys.argv[0], video, out_efc]
    if cmp:
        parts.extend(["--cmp", cmp])
    return " ".join(shlex.quote(p) for p in parts) + (" " + extra if extra else "")


def finetune_tips(video: str, out_efc: str, cmp: str, max_bytes: int, chosen: dict,
                  allow_drop: bool, tried: set) -> None:
    """Print commands that can shrink a file the search left over --max-bytes."""
    if chosen["size"] <= max_bytes:
        return
    n = int(chosen["n"])
    colors = max(2, int(chosen["colors"]))
    dz = float(chosen["dz"])
    seen = {(int(a), int(b), float(c)) for a, b, c in tried}
    palette = min(colors, 96)
    dz_txt = str(int(dz)) if dz == int(dz) else str(dz)

    def offer(label: str, extra: str, key: tuple) -> None:
        if key in seen:
            return
        seen.add(key)
        print(f"  {label}:")
        print("   ", _cmd(video, out_efc, cmp, extra))

    print(f"over budget: {chosen['size']} bytes > {max_bytes}. The file was still written.")
    print("finetune:")
    if dz > 0:
        offer("drop 4x4 hold (frozen tiles look like a mask)",
              f"--frames {n} --colors {max(colors, 128)} --deadzone 0",
              (n, max(colors, 128), 0.0))
    if colors < 128:
        offer("more colors, no hold", f"--frames {n} --colors 128 --deadzone 0", (n, 128, 0.0))
    if not allow_drop:
        print("  drop frames so the file stays under the cap (motion becomes uneven):")
        print("   ", _cmd(video, out_efc, cmp, f"--max-bytes {max_bytes} --allow-drop-frames"))
    print("  keep this encode and raise the cap:")
    print("   ", _cmd(video, out_efc, cmp,
                       f"--max-bytes {chosen['size']} --frames {n} --colors {colors} --deadzone {dz_txt}"))


def prepare(base: np.ndarray, nkeep: int, colors: int, dz: float, key,
            dither: bool = False, palette: str = "oklab", hyst: float = 0.03,
            max_age: int = 0, plain_hold: bool = False,
            spatial: float = 0.012, bayer: float = 0.0
            ) -> tuple[np.ndarray, np.ndarray, np.ndarray | None]:
    src = subset(base, nkeep)
    src_key = subset_key(key, nkeep)
    if dither:
        _rgb, pimg = build_palette_octree(src, colors, src_key)
        clip = map_clip(src, pimg, src_key, True)
        if plain_hold:
            return tile_hold(clip, dz, src_key), src, src_key
        return tile_hold_stable(clip, src, dz, src_key, max_age), src, src_key
    if palette == "octree":
        pal, _pimg = build_palette_octree(src, colors, src_key)
    else:
        pal = build_palette_oklab(src, colors, src_key)
    clip = map_temporal(src, pal, src_key, hyst, max_age, spatial, bayer)
    if plain_hold:
        clip = tile_hold(clip, dz, src_key)
    else:
        clip = tile_hold_stable(clip, src, dz, src_key, max_age)
    return clip, src, src_key


def quality_rank(m: dict, colors: int, dz: float, size: int, under: bool) -> tuple:
    """Prefer under-budget, then less contouring, then higher PSNR, then a lighter hold."""
    ripple_q = round(m["ripple"], 1)
    false_q = round(m["false_edges"], 3)
    return (int(under), -ripple_q, -false_q, m["psnr"], m["ssim"], -dz, colors, -size)


def parse_size(text: str) -> int:
    """Accept bytes or friendly suffixes such as 300K, 500KiB, and 1M."""
    value = text.strip().lower().replace("ib", "b")
    scale = 1
    if value.endswith("kb"):
        scale, value = 1024, value[:-2]
    elif value.endswith("k"):
        scale, value = 1024, value[:-1]
    elif value.endswith("mb"):
        scale, value = 1024 * 1024, value[:-2]
    elif value.endswith("m"):
        scale, value = 1024 * 1024, value[:-1]
    elif value.endswith("b"):
        value = value[:-1]
    try:
        size = int(float(value) * scale)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid size: {text!r}") from exc
    if size <= 0:
        raise argparse.ArgumentTypeError("size must be greater than zero")
    return size


def parse_scale_ratio(text: str) -> int:
    """Return the output tile size for one supported fixed 4-pixel stage."""
    ratios = {
        "4/5": 5,
        "4/6": 6,
        "1/2": 8,
        "4/8": 8,
        "1/4": 16,
        "4/16": 16,
    }
    try:
        return ratios[text.strip()]
    except KeyError as exc:
        raise argparse.ArgumentTypeError(
            "scale ratio must be 4/5, 4/6, 1/2, or 1/4"
        ) from exc


def human_size(size: int) -> str:
    if size >= 1024 * 1024:
        return f"{size / (1024 * 1024):.2f} MiB"
    return f"{size / 1024:.1f} KiB"


def main() -> int:
    ap = argparse.ArgumentParser(
        usage="%(prog)s INPUT OUTPUT [--size 400K] "
              "[--preset quality|balanced|compact|fast] [--preview FILE]",
        description="Convert a short video to EFC.",
        epilog=(
            "examples:\n"
            "  %(prog)s input.avi output.efc\n"
            "  %(prog)s input.avi output.efc --size 400K\n"
            "  %(prog)s input.avi output.efc --size 500K --preset quality\n"
            "  %(prog)s input.gif output.efc --h264-crf 18\n"
            "  %(prog)s input.avi output.efc --preset fast --preview preview.gif\n\n"
            "environment:\n"
            "  EFC_FFMPEG=/path/to/ffmpeg    override FFmpeg\n"
            "  EFC_FFPROBE=/path/to/ffprobe  override ffprobe"
        ),
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    ap.add_argument("video", help="input video")
    ap.add_argument("out_efc", help="output .efc file")
    ap.add_argument("--max-bytes", "--size", dest="max_bytes", type=parse_size,
                    default=None,
                    help="search for an output within this size; accepts bytes, K, or M. "
                         "Omit for the recommended 128-color encode")
    ap.add_argument("--preset", choices=("quality", "balanced", "compact", "fast"),
                    default="balanced",
                    help="quality: cleanest; balanced: recommended; compact: favor size; "
                         "fast: quicker palette search (default: balanced)")
    ap.add_argument("--preview", default="",
                    help="write an animated source/EFC comparison GIF")
    ap.add_argument("--cmp", default="",
                    help="write a source/JPEG/EFC comparison PNG")
    ap.add_argument("--jpeg-reference", action="store_true",
                    help="calculate and print the JPEG q75 reference size; off by default "
                         "(--cmp enables it automatically)")
    ap.add_argument("--full-metrics", action="store_true",
                    help="measure quality at full resolution; default samples large frames "
                         "for much faster candidate scoring")
    ap.add_argument("--verbose", action="store_true",
                    help="show codec internals and detailed candidate metrics")
    ap.add_argument("--h264-crf", type=int, default=0, metavar="CRF",
                    help="lossily simplify the input through x264 before EFC encoding; "
                         "18 is conservative, 23 is aggressive, 0 disables (default)")

    advanced = ap.add_argument_group("expert options", "Normally leave these unset.")
    advanced.add_argument("--colors", type=int, default=0,
                    help="force a palette and skip the search. With other force flags, default 96")
    advanced.add_argument("--frames", type=int, default=0,
                    help="force a frame count. Omit to keep every extracted frame")
    advanced.add_argument("--deadzone", type=float, default=-1,
                    help="force a 4x4 hold and skip the search. 0 disables the hold")
    advanced.add_argument("--legacy", action="store_true",
                    help="skip predictor side streams, for comparison")
    advanced.add_argument("--scale-ratio", type=parse_scale_ratio, default=0,
                    metavar="4/5|4/6|1/2|1/4",
                    help="force one internal scale stage and skip automatic size/ratio search; "
                         "4 coded pixels map to 5, 6, 8, or 16 output pixels")
    advanced.add_argument("--dither", action="store_true",
                    help="Floyd–Steinberg per frame (A/B). Adds crawling grain and usually grows the file")
    advanced.add_argument("--hold", action="store_true",
                    help="encoder dither_hold (A/B). Turns Floyd–Steinberg on and reuses pixels "
                         "whose packed-RGB565 3x3 median is unchanged")
    advanced.add_argument("--plain-hold", action="store_true",
                    help="old hard-threshold 4x4 hold, for A/B against the stable hold")
    advanced.add_argument("--palette", choices=("oklab", "octree", "ffmpeg"), default=None,
                    help="override the preset's palette builder")
    advanced.add_argument("--hyst", type=float, default=None,
                    help="OKLab slack for keeping the previous palette index. Can reduce size, "
                         "but may look like noise floating over moving gradients; 0 disables")
    advanced.add_argument("--spatial", type=float, default=None,
                    help="override spatial palette relaxation; -1 disables")
    advanced.add_argument("--bayer", type=float, default=0.0,
                    help="position-locked 8x8 Bayer amplitude in 8-bit RGB. 0 disables")
    advanced.add_argument("--via-gif", action="store_true",
                    help="quantize like a GIF (global palette + Bayer), then lossless EFC. "
                         "Same picture as the GIF, usually a smaller file. Default path unchanged")
    advanced.add_argument("--max-hold-age", type=int, default=0,
                    help="max consecutive frames a pixel or 4x4 may stay held. 0 means no cap")
    advanced.add_argument("--denoise", choices=("auto", "on", "off"), default="auto",
                    help="control atadenoise during extraction; auto enables it for lossy codecs")
    advanced.add_argument("--lossless", action="store_true",
                    help="skip denoise, palette, still-freeze, and the 4x4 hold. "
                         "Stores the source as RGB565")
    advanced.add_argument("--allow-drop-frames", action="store_true",
                    help="drop frames to meet --max-bytes; off by default because motion becomes uneven")
    args = ap.parse_args()
    if not 0 <= args.h264_crf <= 51:
        ap.error("--h264-crf must be between 0 (disabled) and 51")
    if args.h264_crf and args.lossless:
        ap.error("--h264-crf is lossy and cannot be combined with --lossless")

    preset = {
        "quality":  {"palette": "oklab",  "spatial": -1.0,  "hyst": 0.0},
        "balanced": {"palette": "ffmpeg", "spatial": -1.0,  "hyst": 0.0},
        "compact":  {"palette": "octree", "spatial": 0.025, "hyst": 0.0},
        "fast":     {"palette": "ffmpeg", "spatial": -1.0, "hyst": 0.0},
    }[args.preset]
    if args.palette is None:
        args.palette = preset["palette"]
    if args.spatial is None:
        args.spatial = preset["spatial"]
    if args.hyst is None:
        args.hyst = preset["hyst"]
    has_budget = args.max_bytes is not None
    if not has_budget:
        # No hidden size target: the normal default is one good, predictable
        # 128-color encode. EFC then stores those mapped RGB565 pixels losslessly.
        args.max_bytes = (1 << 63) - 1
        if (not args.lossless and not args.via_gif and not args.colors
                and not args.frames and args.deadzone < 0):
            args.colors = 128
            args.deadzone = 0.0
    dither = (args.dither or args.hold) and not args.lossless

    info = probe(args.video)
    denoise = (not args.lossless and
               (args.denoise == "on" or
                (args.denoise == "auto" and
                 ("h264" if args.h264_crf else info["codec"]) in LOSSY)))
    if args.verbose:
        print(f"source {info['codec']} {info['w']}x{info['h']} {info['fps']:.2f}fps "
              f"{info['duration']:.2f}s alpha={info['alpha']} denoise={denoise} "
              f"h264_crf={args.h264_crf or 'off'} ffmpeg={ffmpeg_bin()} "
              f"lossless={args.lossless} dither={dither} hold={args.hold} "
              f"palette={args.palette} hyst={args.hyst} spatial={args.spatial} "
              f"bayer={args.bayer} max_age={args.max_hold_age}")
    else:
        print(f"Input:  {args.video} ({info['w']}x{info['h']}, "
              f"{info['fps']:.2f} fps, {info['duration']:.2f} s)")
        if has_budget:
            print(f"Goal:   {human_size(args.max_bytes)} maximum, keep all frames")
        elif args.lossless:
            print("Goal:   source RGB565, lossless, no size limit")
        else:
            print("Goal:   recommended 128-color encode, no size limit")
        print(f"Preset: {args.preset}")
        if args.h264_crf:
            print(f"H.264:  CRF {args.h264_crf}, tune animation, yuv420p")

    if args.h264_crf:
        reference_frames, key = extract(
            args.video, info, False, scale_to_tile=True
        )
        h264_path = preprocess_h264(
            args.video, args.h264_crf, info["w"], info["h"]
        )
        try:
            h264_info = probe(h264_path)
            frames, _unused_key = extract(
                h264_path, h264_info, denoise, scale_to_tile=True
            )
        finally:
            os.remove(h264_path)
        frames = np.ascontiguousarray(frames[:, :info["h"], :info["w"]])
        if frames.shape != reference_frames.shape:
            raise SystemExit(
                "H.264 preprocessing changed the decoded clip shape: "
                f"{reference_frames.shape} -> {frames.shape}"
            )
    else:
        frames, key = extract(
            args.video, info, denoise, scale_to_tile=not args.lossless
        )
        reference_frames = frames
    quality_frames = reference_frames if args.h264_crf else frames
    metric_max_side = None if args.full_metrics else 256

    def measure_quality(src: np.ndarray, dec: np.ndarray,
                        alpha_key: np.ndarray | None) -> dict:
        return qmet.measure(src, dec, alpha_key, max_side=metric_max_side)

    if args.verbose:
        print(f"extracted {len(frames)} frames {frames.shape[2]}x{frames.shape[1]}")
    else:
        print(f"Frames: {len(frames)}\n")
    if args.lossless:
        base = frames
        if key is not None:
            base = apply_color_key(frames, key)
    else:
        base = freeze_still(frames, key)

    work = tempfile.mkdtemp(prefix="efc_search_")
    best_decoded = None
    try:
        if args.scale_ratio:
            output_tile = args.scale_ratio
            src = subset(base, args.frames or len(base))
            ref = subset(quality_frames, args.frames or len(quality_frames))
            src_key = subset_key(key, len(src))
            display_h, display_w = src.shape[1:3]
            coded_w = ((display_w + output_tile - 1) // output_tile) * 4
            coded_h = ((display_h + output_tile - 1) // output_tile) * 4
            low_src = resize_clip(src, coded_w, coded_h)
            low_key = resize_key(src_key, coded_w, coded_h)
            dz = args.deadzone if args.deadzone >= 0 else 0.0
            if args.lossless:
                clip = low_src
                if low_key is not None:
                    clip = apply_color_key(clip, low_key)
                colors = int(np.unique(to_565(clip)).size)
            elif args.via_gif:
                colors = args.colors or 256
                bayer = args.bayer if args.bayer > 0 else 16.0
                if args.palette == "ffmpeg":
                    clip = map_ffmpeg_palette(low_src, colors, low_key)
                elif args.palette == "octree":
                    pal, _pimg = build_palette_octree(low_src, colors, low_key)
                    clip = map_gif(low_src, pal, low_key, bayer)
                else:
                    pal = build_palette_oklab(low_src, colors, low_key)
                    clip = map_gif(low_src, pal, low_key, bayer)
            elif args.palette == "ffmpeg" and not dither:
                colors = args.colors or 128
                clip = map_ffmpeg_palette(low_src, colors, low_key)
                if args.plain_hold:
                    clip = tile_hold(clip, dz, low_key)
                else:
                    clip = tile_hold_stable(
                        clip, low_src, dz, low_key, args.max_hold_age
                    )
            else:
                colors = args.colors or 128
                clip, _prepared_src, low_key = prepare(
                    low_src, len(low_src), colors, dz, low_key, dither,
                    args.palette, args.hyst, args.max_hold_age,
                    args.plain_hold, args.spatial, args.bayer,
                )
            size, line = encode(
                clip, args.out_efc, low_key, args.legacy, args.hold
            )
            mark_scaled_efc(args.out_efc, display_w, display_h, output_tile)
            best_decoded = qmet.decode_efc(args.out_efc)[0]
            metrics = measure_quality(ref, best_decoded, src_key)
            ratio_name = {5: "4/5", 6: "4/6", 8: "1/2", 16: "1/4"}[output_tile]
            print(f"manual internal scale {ratio_name}: {coded_w}x{coded_h} -> "
                  f"{display_w}x{display_h}, {human_size(size)}")
            if args.verbose:
                print(" ", qmet.format_metrics(metrics))
            chosen = {
                "n": len(clip), "colors": colors, "dz": dz, "size": size,
                "line": line + f"  scaled={coded_w}x{coded_h}",
                "metrics": metrics, "coded_width": coded_w,
                "coded_height": coded_h, "key": src_key,
            }
            tried = set()
        elif args.lossless:
            clip = base
            size, line = encode(clip, args.out_efc, key, args.legacy, args.hold)
            n565 = int(np.unique(to_565(clip)).size)
            print(f"lossless RGB565  unique={n565}  no denoise, palette, freeze, or hold")
            chosen = {"n": len(clip), "colors": n565, "dz": 0.0, "size": size, "line": line,
                      "metrics": None, "src": clip, "key": key}
            best_decoded = qmet.decode_efc(args.out_efc)[0]
            tried = set()
        elif args.via_gif:
            colors = args.colors or 256
            bayer = args.bayer if args.bayer > 0 else 16.0
            src = subset(frames, args.frames or len(frames))
            ref = subset(quality_frames, args.frames or len(quality_frames))
            src_key = subset_key(key, len(src))
            if args.palette == "ffmpeg":
                clip = map_ffmpeg_palette(src, colors, src_key)
            elif args.palette == "octree":
                pal, _pimg = build_palette_octree(src, colors, src_key)
                clip = map_gif(src, pal, src_key, bayer)
            else:
                pal = build_palette_oklab(src, colors, src_key)
                clip = map_gif(src, pal, src_key, bayer)
            size, line = encode(clip, args.out_efc, src_key, args.legacy, args.hold)
            dec = qmet.decode_efc(args.out_efc)[0]
            m = measure_quality(ref, dec, src_key)
            print(f"via-gif colors={colors} bayer={bayer} palette={args.palette}  "
                  f"{size} bytes  {qmet.format_metrics(m)}")
            chosen = {"n": len(clip), "colors": colors, "dz": 0.0, "size": size, "line": line,
                      "metrics": m, "key": src_key}
            best_decoded = dec
            tried = set()
        else:
            forced = bool(args.colors or args.frames or args.deadzone >= 0)
            tried = set()
            best = None
            best_frames = None
            best_src = None

            def show_candidate(size, nkeep, colors, dz, under, metrics=None):
                state = "fits" if under else "too large"
                hold_text = "no hold" if dz == 0 else f"hold {dz:g}"
                print(f"  Trying {colors} colors, {hold_text}: "
                      f"{human_size(size)} — {state}")
                if args.verbose and metrics is not None:
                    print("   ", qmet.format_metrics(metrics))

            def consider(nkeep, colors, dz, force_measure=False,
                         defer_quality=False, record_deferred=True):
                nonlocal best, best_frames, best_src, best_decoded
                src = subset(base, nkeep)
                ref = subset(quality_frames, nkeep)
                src_key = subset_key(key, nkeep)
                if args.palette == "ffmpeg" and not dither:
                    clip = map_ffmpeg_palette(src, colors, src_key)
                    if args.plain_hold:
                        clip = tile_hold(clip, dz, src_key)
                    else:
                        clip = tile_hold_stable(
                            clip, src, dz, src_key, args.max_hold_age
                        )
                else:
                    clip, src, src_key = prepare(
                        base, nkeep, colors, dz, key, dither, args.palette,
                        args.hyst, args.max_hold_age, args.plain_hold, args.spatial,
                        args.bayer,
                    )
                path = os.path.join(work, "cand.efc")
                size, line = encode(clip, path, src_key, args.legacy, args.hold)
                under = size <= args.max_bytes
                tried.add((len(clip), colors, float(dz)))
                if defer_quality:
                    show_candidate(size, len(clip), colors, dz, under)
                    keep = (best is None or
                            (under and (not best["under"] or colors > best["colors"])))
                    if record_deferred and keep:
                        best = {"rank": (int(under), colors), "under": under, "n": len(clip),
                                "colors": colors, "dz": dz, "size": size,
                                "line": line, "metrics": None, "key": src_key}
                        best_frames = clip
                        best_src = ref
                        best_decoded = None
                        shutil.copy(path, args.out_efc)
                    return size
                # Rejected candidates only guide the size search. Full quality
                # metrics cost several seconds and cannot make an over-budget
                # candidate beat one that fits, so defer them.
                if not under and not force_measure:
                    show_candidate(size, len(clip), colors, dz, under)
                    return size
                dec = qmet.decode_efc(path)[0]
                m = measure_quality(ref, dec, src_key)
                show_candidate(size, len(clip), colors, dz, under, m)
                # Keep the best-looking candidate even when it is over the
                # byte cap. Previously only an under-budget file (or a hard
                # 96/dz=17 fallback) was stored, so a failed budget search
                # wrote a frozen 4x4 hold mask instead of the no-hold encode
                # that the comparison GIF had already shown as acceptable.
                rank = quality_rank(m, colors, dz, size, under)
                if best is None or rank > best["rank"]:
                    best = {"rank": rank, "under": under, "n": len(clip), "colors": colors,
                            "dz": dz, "size": size, "line": line, "metrics": m,
                            "key": src_key}
                    best_frames = clip
                    best_src = ref
                    best_decoded = dec
                    shutil.copy(path, args.out_efc)
                return size

            def try_scaled_last(nkeep: int) -> None:
                """Last-resort full-frame search: fewer coded pixels, display-size output."""
                nonlocal best, best_frames, best_src, best_decoded
                if best is not None and best["under"]:
                    return
                src = subset(base, nkeep)
                ref = subset(quality_frames, nkeep)
                src_key = subset_key(key, nkeep)
                display_h, display_w = src.shape[1:3]
                seen = set()
                # Four coded pixels expand directly into one output tile.
                # These tile-aligned stages need no final frame-scale pass.
                for output_tile in (5, 6, 8, 16):
                    coded_w = ((display_w + output_tile - 1) // output_tile) * 4
                    coded_h = ((display_h + output_tile - 1) // output_tile) * 4
                    if coded_w < 4 or coded_h < 4:
                        continue
                    if (coded_w, coded_h) in seen or (coded_w == display_w and coded_h == display_h):
                        continue
                    seen.add((coded_w, coded_h))
                    low_src = resize_clip(src, coded_w, coded_h)
                    low_key = resize_key(src_key, coded_w, coded_h)
                    clip = map_ffmpeg_palette(low_src, 128, low_key)
                    path = os.path.join(work, "scaled.efc")
                    size, line = encode(clip, path, low_key, args.legacy, args.hold)
                    mark_scaled_efc(path, display_w, display_h, output_tile)
                    dec = qmet.decode_efc(path)[0]
                    metrics = measure_quality(ref, dec, src_key)
                    under = size <= args.max_bytes
                    print(f"  Trying internal scale {coded_w}x{coded_h} -> "
                          f"{display_w}x{display_h}: {human_size(size)} — "
                          f"{'fits' if under else 'too large'}")
                    if args.verbose:
                        print("   ", qmet.format_metrics(metrics))
                    tried.add((len(clip), 128, float(-coded_w)))
                    if under:
                        best = {
                            "rank": quality_rank(metrics, 128, 0, size, True),
                            "under": True, "n": len(clip), "colors": 128, "dz": 0.0,
                            "size": size, "line": line + f"  scaled={coded_w}x{coded_h}",
                            "metrics": metrics, "coded_width": coded_w,
                            "coded_height": coded_h, "key": src_key,
                        }
                        best_frames = clip
                        best_src = ref
                        best_decoded = dec
                        shutil.copy(path, args.out_efc)
                        return

            if forced:
                nkeep = args.frames or len(base)
                colors = args.colors or 96
                dz = args.deadzone if args.deadzone >= 0 else 0.0
                if args.verbose:
                    print(f"forced frames={nkeep} colors={colors} dz={dz}")
                consider(nkeep, colors, dz, force_measure=True)
                if has_budget and (best is None or not best["under"]):
                    try_scaled_last(nkeep)
                chosen = best
            elif not args.allow_drop_frames:
                full = len(base)
                # Descending quality: stop at the highest palette size that
                # fits the requested ceiling. Starting at 192 made a 500 KB
                # ceiling produce the same file as a 410 KB ceiling.
                palettes = (255, 224, 192, 176, 168, 160, 144, 128, 96)
                size = consider(full, palettes[0], 0, defer_quality=True)
                min_size = None
                impossible = False
                if size > args.max_bytes:
                    # Very low ceilings are often impossible with every frame.
                    # Probe the smallest/strongest candidate before spending
                    # time on a palette search that cannot succeed.
                    if args.max_bytes * 10 < size * 7:
                        min_size = consider(full, 80, 24, defer_quality=True,
                                            record_deferred=False)
                        impossible = min_size > args.max_bytes
                    if not impossible:
                        lo, hi = 1, len(palettes) - 1
                        while lo <= hi:
                            mid = (lo + hi) // 2
                            size = consider(full, palettes[mid], 0, defer_quality=True)
                            if size <= args.max_bytes:
                                hi = mid - 1
                            else:
                                lo = mid + 1
                if best is not None and best["under"]:
                    best_decoded = qmet.decode_efc(args.out_efc)[0]
                    best["metrics"] = measure_quality(
                        best_src, best_decoded, best.get("key")
                    )
                    if args.verbose:
                        print("  selected", qmet.format_metrics(best["metrics"]))
                if best is None or not best["under"]:
                    # First test the smallest/strongest candidate. If even it
                    # is too large, no point searching the hold grid.
                    if min_size is None:
                        min_size = consider(full, 80, 24, defer_quality=True,
                                            record_deferred=False)
                    if min_size > args.max_bytes:
                        print(f"  No full-frame setting can reach "
                              f"{human_size(args.max_bytes)}; trying internal scaling last.")
                    else:
                        # Find the lightest hold that fits each palette with a
                        # binary search instead of scanning every deadzone.
                        deadzones = (8, 12, 14, 15, 17, 24)
                        for colors in (128, 112, 96, 80):
                            lo, hi = 0, len(deadzones) - 1
                            while lo <= hi:
                                mid = (lo + hi) // 2
                                size = consider(full, colors, deadzones[mid])
                                if size <= args.max_bytes:
                                    hi = mid - 1
                                else:
                                    lo = mid + 1
                            if (best and best["under"] and best["metrics"]
                                    and best["metrics"]["psnr"] >= 34.0
                                    and best["metrics"]["false_edges"] <= 0.008):
                                break
                if best is None:
                    consider(full, 128, 0, force_measure=True)
                if best is None or not best["under"]:
                    try_scaled_last(full)
                if not best["under"]:
                    print(f"  Keeping all {full} frames; output will exceed the limit.")
                chosen = best
            else:
                full = len(base)

                def consider_drop(nkeep, colors, dz):
                    return consider(nkeep, colors, dz)

                for colors in (192, 255):
                    lo, hi = 8, full
                    winner = None
                    while lo <= hi:
                        mid = (lo + hi) // 2
                        size = consider_drop(mid, colors, 0)
                        if size <= args.max_bytes:
                            winner = mid
                            lo = mid + 1
                        else:
                            hi = mid - 1
                    if winner is None:
                        continue
                    for extra, dz in ((2, 8), (4, 8), (2, 12)):
                        n = min(full, winner + extra)
                        if n != winner:
                            consider_drop(n, colors, dz)
                if best is None:
                    script = " ".join(shlex.quote(p) for p in ("python3", sys.argv[0], args.video, args.out_efc))
                    raise SystemExit(
                        f"nothing fit under {args.max_bytes} bytes\n"
                        "finetune:\n"
                        f"  shorter palette: {script} --frames 8 --colors 64 --deadzone 0\n"
                        f"  higher cap:      {script} --max-bytes {args.max_bytes * 2}"
                    )
                chosen = best
            if best_decoded is None and os.path.isfile(args.out_efc):
                best_decoded = qmet.decode_efc(args.out_efc)[0]
    finally:
        shutil.rmtree(work, ignore_errors=True)

    chosen_key = chosen.get("key", subset_key(key, chosen["n"]))
    jpg = jpeg_reference(quality_frames, key, 75) if (args.jpeg_reference or args.cmp) else None
    delay = (info["duration"] * 1000.0 / chosen["n"]) if info["duration"] and chosen["n"] else 0
    if args.verbose:
        print(f"EFC {chosen['size']} bytes  frames={chosen['n']} "
              f"colors={chosen['colors']} dz={chosen['dz']}")
        print(f"play delay {delay:.0f} ms  "
              f"({chosen['n'] / info['duration']:.2f} fps)" if info["duration"] else "")
        print(chosen["line"])
    if chosen.get("metrics"):
        if args.verbose:
            print("decoded", qmet.format_metrics(chosen["metrics"]))
    elif best_decoded is not None:
        src_ix = np.unique(
            np.linspace(0, len(quality_frames) - 1, chosen["n"]).round().astype(int)
        )
        m = measure_quality(quality_frames[src_ix], best_decoded, chosen_key)
        if args.verbose:
            print("decoded", qmet.format_metrics(m))
        chosen["metrics"] = m

    fits = not has_budget or chosen["size"] <= args.max_bytes
    print("\nDone")
    print(f"  Output:  {args.out_efc}")
    if has_budget:
        print(f"  Size:    {human_size(chosen['size'])} "
              f"({'within' if fits else 'over'} the {human_size(args.max_bytes)} limit)")
    else:
        print(f"  Size:    {human_size(chosen['size'])} (no size limit)")
    print(f"  Video:   {chosen['n']} frames, "
          f"{chosen['n'] / info['duration']:.2f} fps" if info["duration"]
          else f"  Video:   {chosen['n']} frames")
    print(f"  Palette: {chosen['colors']} colors")
    if jpg is not None:
        print(f"  JPEG q75: {human_size(jpg)}")
    if chosen.get("metrics"):
        quality_name = ("Quality (sampled)"
                        if metric_max_side and max(quality_frames.shape[1:3]) > metric_max_side
                        else "Quality")
        print(f"  {quality_name}: PSNR {chosen['metrics']['psnr']:.2f}, "
              f"SSIM {chosen['metrics']['ssim']:.4f}")
    print(f"  Play:    python3 {os.path.join(PC_DIR, 'efc_play.py')} {args.out_efc}")

    if args.lossless:
        if has_budget and chosen["size"] > args.max_bytes:
            print("\nThe lossless result cannot meet this limit. "
                  "Use the balanced preset to allow palette conversion.")
    else:
        if has_budget and args.verbose:
            finetune_tips(args.video, args.out_efc, args.cmp, args.max_bytes, chosen,
                          args.allow_drop_frames, tried)
        elif has_budget and not fits:
            print("\nThe requested size is not possible while keeping every frame.")
            print(f"  Keep all frames: rerun with --size {human_size(chosen['size']).replace(' ', '')}")
            print("  Meet the limit:  add --allow-drop-frames")
    if args.cmp or args.preview:
        src_n = chosen["n"]
        src_ix = np.unique(
            np.linspace(0, len(quality_frames) - 1, src_n).round().astype(int)
        )
        src_show = quality_frames[src_ix]
        show_key = subset_key(key, src_n)
        if best_decoded is None:
            best_decoded = qmet.decode_efc(args.out_efc)[0]
        if args.cmp:
            jpg_frames = np.empty_like(src_show)
            show = src_show.copy()
            if show_key is not None:
                show = show.copy()
                mask = (show_key if show_key.ndim == 3
                        else np.broadcast_to(show_key, show.shape[:3]))
                show[mask] = (255, 255, 255)
            for i, fr in enumerate(show):
                fd, p = tempfile.mkstemp(suffix=".jpg")
                os.close(fd)
                Image.fromarray(fr).save(p, quality=75, optimize=True)
                jpg_frames[i] = np.array(Image.open(p).convert("RGB"))
                os.remove(p)
            comparison(src_show, jpg_frames, best_decoded, show_key, args.cmp)
            print(f"  Comparison image: {args.cmp}")
        preview = args.preview
        if not preview and args.cmp:
            root, _ext = os.path.splitext(args.cmp)
            preview = root + "_preview.gif"
        if preview:
            qmet.save_preview(
                src_show, best_decoded, show_key, preview,
                delay_ms=int(delay or 67)
            )
            print(f"  Preview: {preview}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
