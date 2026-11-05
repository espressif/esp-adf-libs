#!/usr/bin/env python3
"""Play an .efc clip in a window.

EFC does not store a frame rate or a background. This player punches out
the color key and holds a steady delay so a short emote loops instead of
racing through. It decodes with build/efc_tool when that binary is present,
otherwise with efc_py.py.

    python3 efc_play.py clip.efc
    python3 efc_play.py clip.efc --fps 15

Keys:
    Space       pause / resume
    Left Right  step one frame (pauses)
    Up Down     faster / slower
    C           cycle the backdrop (checker, black, white)
    Q Escape    quit
"""
from __future__ import annotations

import argparse
import os
import shutil
import struct
import subprocess
import sys
import tempfile
import time
import zlib

import numpy as np
from PIL import Image

try:
    import tkinter as tk
except ImportError:
    tk = None

PC_DIR = os.path.dirname(os.path.abspath(__file__))
EFC_TOOL = os.path.join(PC_DIR, "build", "efc_tool")
EFC_PY = os.path.join(PC_DIR, "efc_py.py")
ALPHA_INDEX = 0x13
FLAG_COMPRESSED = 0x04
FLAG_PRED = 0x08
FLAG_T2 = 0x01


def codec_path() -> tuple[str, str]:
    """Prefer the C tool. Fall back to the shuffled Python codec."""
    if os.path.isfile(EFC_TOOL):
        return "c", EFC_TOOL
    if os.path.isfile(EFC_PY):
        return "py", EFC_PY
    raise SystemExit("no decoder: build/efc_tool is missing and efc_py.py is missing")


def _inflate_plane(payload: bytes, at: int) -> tuple[bytes, int]:
    """One PRED plane: u32 raw length, u32 compressed length, then the bytes."""
    if at + 8 > len(payload):
        raise SystemExit("truncated EFC plane")
    raw_len, comp_len = struct.unpack_from("<II", payload, at)
    at += 8
    if comp_len:
        if at + comp_len > len(payload):
            raise SystemExit("truncated EFC plane")
        data = zlib.decompress(payload[at:at + comp_len], -15)
        if len(data) != raw_len:
            raise SystemExit("EFC plane length mismatch")
        return data, at + comp_len
    if at + raw_len > len(payload):
        raise SystemExit("truncated EFC plane")
    return payload[at:at + raw_len], at + raw_len


def color_key_565(blob: bytes):
    if len(blob) < 20 or blob[:4] != b"EFC1":
        raise SystemExit("not an EFC1 file")
    flags = blob[5]
    _w, _h, _wp, _hp, frames, colors = struct.unpack_from("<HHHHHH", blob, 8)
    prefix = 20 + colors * 2 + frames * 9
    if prefix > len(blob):
        raise SystemExit("truncated EFC header")
    if (blob[7] & 1) and (flags & FLAG_T2):
        return 0xF81F
    if flags & FLAG_T2 or colors == 0:
        return None
    palette = struct.unpack_from("<" + "H" * colors, blob, 20)
    payload = blob[prefix:]
    if flags & FLAG_PRED:
        if flags & FLAG_COMPRESSED:
            payload, _at = _inflate_plane(payload, 0)
        else:
            if len(payload) < 8:
                raise SystemExit("truncated EFC plane")
            modes_len, lits_len = struct.unpack_from("<II", payload, 0)
            start = 8 + modes_len + lits_len
            if start > len(payload):
                raise SystemExit("truncated EFC plane")
            payload = payload[start:]
    elif flags & FLAG_COMPRESSED:
        if len(payload) < 4:
            raise SystemExit("truncated compressed EFC")
        payload = zlib.decompress(payload[4:], -15)
    if len(payload) < 2 or payload[0] != ALPHA_INDEX:
        return None
    index = payload[1]
    if index >= colors:
        raise SystemExit(f"color key index {index} outside palette of {colors}")
    return palette[index]


def expand_565(frame: np.ndarray) -> np.ndarray:
    """RGB565 little-endian plane to RGB888, replicating the top bits."""
    r = (frame >> 11) & 0x1F
    g = (frame >> 5) & 0x3F
    b = frame & 0x1F
    rgb = np.empty(frame.shape + (3,), np.uint8)
    rgb[..., 0] = ((r << 3) | (r >> 2)).astype(np.uint8)
    rgb[..., 1] = ((g << 2) | (g >> 4)).astype(np.uint8)
    rgb[..., 2] = ((b << 3) | (b >> 2)).astype(np.uint8)
    return rgb


def backdrop(height: int, width: int, kind: str) -> np.ndarray:
    if kind == "black":
        return np.zeros((height, width, 3), np.uint8)
    if kind == "white":
        return np.full((height, width, 3), 255, np.uint8)
    yy, xx = np.indices((height, width))
    dark = ((xx // 8 + yy // 8) & 1) == 0
    bg = np.empty((height, width, 3), np.uint8)
    bg[dark] = (48, 48, 48)
    bg[~dark] = (196, 196, 196)
    return bg


def composite(frame: np.ndarray, key, bg: np.ndarray) -> np.ndarray:
    rgb = expand_565(frame)
    if key is None:
        return rgb
    return np.where((frame == key)[..., None], bg, rgb)


def display_scale(width: int, height: int, screen_w: int, screen_h: int) -> int:
    """Integer scale that keeps the clip crisp and inside the screen."""
    room_w = max(1, screen_w - 48)
    room_h = max(1, screen_h - 96)
    fit = max(1, min(room_w // width, room_h // height))
    # Small emotes are hard to judge at 1x. Prefer about 640px on the long side.
    want = max(1, 640 // max(width, height))
    return max(1, min(fit, want, 8))


def to_photo(rgb: np.ndarray, scale: int) -> tk.PhotoImage:
    im = Image.fromarray(rgb, "RGB")
    if scale != 1:
        im = im.resize((im.width * scale, im.height * scale), Image.NEAREST)
    # PPM avoids Pillow's ImageTk, which is not always installed.
    ppm = im.tobytes()
    header = f"P6 {im.width} {im.height} 255\n".encode("ascii")
    return tk.PhotoImage(data=header + ppm)


class Player:
    BACKGROUNDS = ("white", "checker", "black")

    def __init__(self, path: str, frames: np.ndarray, key, fps: float):
        self.path = path
        self.frames = frames
        self.n, self.h, self.w = frames.shape
        self.key = key
        self.fps = fps
        self.index = 0
        self.paused = False
        self.bg_i = 0
        self.bgs = [backdrop(self.h, self.w, name) for name in self.BACKGROUNDS]
        self.next_t = time.perf_counter()

        self.root = tk.Tk()
        self.root.title(os.path.basename(path))
        sw = self.root.winfo_screenwidth()
        sh = self.root.winfo_screenheight()
        self.scale = display_scale(self.w, self.h, sw, sh)
        self.photo = None
        self.label = tk.Label(self.root, bd=0)
        self.label.pack()
        self.status = tk.Label(self.root, anchor="w", font=("TkFixedFont", 11))
        self.status.pack(fill="x", padx=6, pady=4)

        self.root.bind("<space>", lambda e: self.toggle())
        self.root.bind("<Left>", lambda e: self.step(-1))
        self.root.bind("<Right>", lambda e: self.step(1))
        self.root.bind("<Up>", lambda e: self.nudge(1.25))
        self.root.bind("<Down>", lambda e: self.nudge(1 / 1.25))
        self.root.bind("c", lambda e: self.cycle_bg())
        self.root.bind("C", lambda e: self.cycle_bg())
        self.root.bind("q", lambda e: self.root.destroy())
        self.root.bind("Q", lambda e: self.root.destroy())
        self.root.bind("<Escape>", lambda e: self.root.destroy())
        self.root.protocol("WM_DELETE_WINDOW", self.root.destroy)
        self.draw()
        self.root.after(15, self.tick)

    def delay(self) -> float:
        return 1.0 / self.fps if self.fps > 0 else 0.066

    def draw(self) -> None:
        rgb = composite(self.frames[self.index], self.key, self.bgs[self.bg_i])
        self.photo = to_photo(rgb, self.scale)
        self.label.configure(image=self.photo)
        name = self.BACKGROUNDS[self.bg_i]
        key = "none" if self.key is None else f"#{self.key:04x}"
        state = "paused" if self.paused else "play"
        self.status.configure(
            text=(f" {self.index + 1}/{self.n}   {self.w}x{self.h}  "
                  f"{self.fps:.2f} fps  {self.delay() * 1000:.0f} ms   "
                  f"{state}   key {key}   bg {name}   x{self.scale}")
        )
        self.root.title(f"{os.path.basename(self.path)}  {self.index + 1}/{self.n}")

    def tick(self) -> None:
        if not self.root.winfo_exists():
            return
        if not self.paused and self.n > 1:
            now = time.perf_counter()
            step = self.delay()
            if now >= self.next_t:
                self.index = (self.index + 1) % self.n
                self.draw()
                self.next_t += step
                if self.next_t < now:
                    self.next_t = now + step
        self.root.after(15, self.tick)

    def toggle(self) -> None:
        self.paused = not self.paused
        self.next_t = time.perf_counter() + self.delay()
        self.draw()

    def step(self, delta: int) -> None:
        self.paused = True
        self.index = (self.index + delta) % self.n
        self.draw()

    def nudge(self, factor: float) -> None:
        self.fps = min(60.0, max(1.0, self.fps * factor))
        self.next_t = time.perf_counter() + self.delay()
        self.draw()

    def cycle_bg(self) -> None:
        self.bg_i = (self.bg_i + 1) % len(self.BACKGROUNDS)
        self.draw()

    def run(self) -> None:
        self.root.mainloop()


def play_ffplay(path: str, frames: np.ndarray, key, fps: float) -> None:
    """Stream a looping RGB24 clip to ffplay when Tk is unavailable."""
    ffplay = shutil.which("ffplay")
    if not ffplay:
        raise SystemExit(
            "no GUI backend: install python3-tk, or install ffplay from the "
            "FFmpeg package; --dump preview.png works without either"
        )
    n, h, w = frames.shape
    scale = max(1, min(8, 640 // max(w, h)))
    vf = f"scale=iw*{scale}:ih*{scale}:flags=neighbor"
    command = [
        ffplay,
        "-hide_banner",
        "-loglevel", "error",
        "-window_title", os.path.basename(path),
        "-f", "rawvideo",
        "-pixel_format", "rgb24",
        "-video_size", f"{w}x{h}",
        "-framerate", f"{fps:g}",
        "-i", "pipe:0",
        "-vf", vf,
    ]
    bg = backdrop(h, w, "checker")
    print("viewer: ffplay fallback (Space pause/resume, Q/Escape quit)")
    try:
        proc = subprocess.Popen(command, stdin=subprocess.PIPE)
    except OSError as exc:
        raise SystemExit(f"failed to start ffplay: {exc}") from exc
    try:
        assert proc.stdin is not None
        while proc.poll() is None:
            for frame in frames:
                rgb = np.ascontiguousarray(composite(frame, key, bg))
                proc.stdin.write(rgb.tobytes())
                if proc.poll() is not None:
                    break
    except (BrokenPipeError, KeyboardInterrupt):
        pass
    finally:
        if proc.stdin:
            try:
                proc.stdin.close()
            except BrokenPipeError:
                pass
        if proc.poll() is None:
            proc.terminate()
        proc.wait()


def load(path: str):
    kind, tool = codec_path()
    with open(path, "rb") as fh:
        blob = fh.read()
    if kind == "c":
        fd, raw_path = tempfile.mkstemp(prefix="efc_play_", suffix=".rgb565")
        os.close(fd)
        try:
            proc = subprocess.run([tool, "decode", path, raw_path], capture_output=True, text=True)
            if proc.returncode != 0:
                raise SystemExit((proc.stdout or "") + (proc.stderr or "") or "decode failed")
            raw = np.fromfile(raw_path, dtype="<u2")
        finally:
            os.remove(raw_path)
        w, h, _wp, _hp, frames, _colors = struct.unpack_from("<HHHHHH", blob, 8)
        if raw.size != w * h * frames:
            raise SystemExit(f"decoded {raw.size} pixels, expected {w * h * frames}")
        return raw.reshape(frames, h, w), color_key_565(blob)
    sys.path.insert(0, PC_DIR)
    import efc_py
    frames, info = efc_py.decode_frames(blob)
    return frames, info["color_key"]


def main() -> int:
    ap = argparse.ArgumentParser(description="Play an EFC clip")
    ap.add_argument("efc")
    ap.add_argument("--fps", type=float, default=15.0,
                    help="playback rate (EFC files do not store one; default 15)")
    ap.add_argument("--delay-ms", type=float, default=0,
                    help="frame delay in milliseconds; overrides --fps")
    ap.add_argument("--dump", metavar="PNG",
                    help="write frame 0 composited on the checkerboard and exit")
    ap.add_argument("--backend", choices=("auto", "tk", "ffplay"), default="auto",
                    help="window backend; auto uses Tk then ffplay (default auto)")
    args = ap.parse_args()
    if not os.path.isfile(args.efc):
        raise SystemExit(f"missing file: {args.efc}")
    fps = 1000.0 / args.delay_ms if args.delay_ms > 0 else args.fps
    frames, key = load(args.efc)
    if args.dump:
        rgb = composite(frames[0], key, backdrop(frames.shape[1], frames.shape[2], "checker"))
        Image.fromarray(rgb, "RGB").save(args.dump)
        print(f"wrote {args.dump}  {frames.shape[2]}x{frames.shape[1]} "
              f"frames={frames.shape[0]} key={'none' if key is None else hex(key)}")
        return 0
    print(f"{args.efc}: {frames.shape[2]}x{frames.shape[1]} frames={frames.shape[0]} "
          f"key={'none' if key is None else hex(key)}  {fps:.2f} fps")
    if args.backend == "ffplay" or (args.backend == "auto" and tk is None):
        play_ffplay(args.efc, frames, key, fps)
        return 0
    if tk is None:
        raise SystemExit(
            "Tk backend unavailable: install python3-tk or use --backend ffplay"
        )
    try:
        Player(args.efc, frames, key, fps).run()
    except tk.TclError as exc:
        if args.backend == "tk":
            raise SystemExit(f"Tk backend failed: {exc}") from exc
        print(f"Tk unavailable ({exc}); using ffplay.", file=sys.stderr)
        play_ffplay(args.efc, frames, key, fps)
    return 0


if __name__ == "__main__":
    sys.exit(main())
