# ESP-EFC-CODEC — Efficient PC Encoder and On-Device Decoder for Emote Animations

- [![Component Registry](https://components.espressif.com/components/espressif/esp_efc_codec/badge.svg)](https://components.espressif.com/components/espressif/esp_efc_codec)
- [中文版](./README_CN.md)

## Fast, compact animations for ESP32 and LVGL

**ESP-EFC-CODEC brings EFC animation decoding to ESP32, with both a low-level decoder and a ready-to-use LVGL object.**

EFC is designed for **short looping animations such as eyes, faces, reactions, and UI effects**. It uses block-based decoding and inter-frame reuse to reduce both flash usage and CPU work.

You can use it at two levels:

|                 | Use                                                                                    |
| --------------- | -------------------------------------------------------------------------------------- |
| **EFC decoder** | Decode directly into an RGB565 framebuffer for custom display pipelines                |
| **LVGL object** | Drop an EFC animation directly into an LVGL UI without implementing the playback logic |

### Why EFC?

Compared with GIF and EAF, EFC targets the specific characteristics of embedded animations:

* **Small files** — especially effective when only a small part of each frame changes
* **Fast decoding** — unchanged blocks can be skipped
* **Low RAM overhead** — one framebuffer can be reused for the entire animation
* **Direct RGB565 output** — suitable for embedded displays
* **Lossless** — decoded pixels are identical to the encoded RGB565 pixels
* **LVGL-ready** — use the supplied LVGL object when a custom decoder is not needed

Results for 23 eye-emote clips from the [Emote Pack](https://dl.espressif.com/AE/Emote%20Pack.zip), tested on ESP32-S3:

| Format            | Stored size |       Decode time |      Throughput |
| ----------------- | ----------: | ----------------: | --------------: |
| GIF               |     3.46 MB |                 — |               — |
| EAF RLE           |    12.85 MB |     77.9 ms/frame |      6.6 Mpix/s |
| EAF RLE + Huffman |     8.50 MB |     81.9 ms/frame |      6.2 Mpix/s |
| **EFC**           | **1.94 MB** | **25.7 ms/frame** | **19.9 Mpix/s** |
| **EFC + zlib**    | **0.65 MB** | **25.8 ms/frame** | **19.8 Mpix/s** |

So for this workload, EFC is both **smaller than the source GIFs and substantially faster to decode than EAF**.

---

## Two ways to use EFC

### 1. Direct EFC decoder

For applications with their own display or rendering pipeline, use the decoder API directly.

```mermaid
flowchart TD
    file["EFC file"] --> dec["EFC decoder"]
    dec --> fb["RGB565 framebuffer"]
    fb --> out["Your display / renderer"]
```

The decoder keeps the framebuffer between frames. Unchanged blocks are left untouched, so they do not need to be decoded again. A clip with a GIF-style color key can be decoded with `esp_efc_dec_frame_with_alpha()` into RGB565 plus one alpha byte per pixel (`0x00` on the key color, `0xFF` elsewhere). `esp_efc_dec_frame()` still writes opaque RGB565 and skips the key.

```c
#include "esp_efc_codec.h"

void play_emote(const uint8_t *blob, size_t len, uint16_t *fb)
{
    esp_efc_dec_handle_t dec = NULL;
    if (esp_efc_dec_open(blob, len, &dec) != ESP_EFC_OK) {
        return;
    }

    esp_efc_dec_info_t info = { 0 };
    if (esp_efc_dec_get_info(dec, &info) != ESP_EFC_OK) {
        esp_efc_dec_close(dec);
        return;
    }

    for (;;) {
        for (int i = 0; i < info.frame_count; i++) {
            if (esp_efc_dec_frame(dec, i, fb) != ESP_EFC_OK) {
                break;
            }

            lcd_draw_rgb565(fb, info.width, info.height);
        }
    }

    esp_efc_dec_close(dec);
}
```

`fb` is a `width × height` RGB565 framebuffer.

**Important:** do not clear `fb` between frames. Unchanged blocks intentionally remain from the previous frame.

---

### 2. LVGL object

If your application already uses LVGL, you can skip the playback implementation and embed an EFC animation directly as an LVGL object.

```mermaid
flowchart TD
    codec["ESP-EFC-CODEC"] --> dec["EFC decoder"]
    codec --> obj["LVGL object"]
    dec --> buf["RGB565 buffer"]
    obj --> ui["LVGL UI"]
    buf --> display["Display"]
    ui --> display
```

The LVGL integration handles the animation playback inside the object, making EFC suitable for applications where an animation is simply another UI element. When the file carries a color key, the object draws RGB565A8 and that color is transparent. A file with no key stays opaque RGB565.

#### Scaling quality and performance

For the best image quality and playback performance, resize the source GIF on the PC to the intended display size before encoding it as EFC. If an application uses several display sizes, generate a separate EFC asset for each size.

The LVGL object can fit an oversized animation into its available area, but software scaling uses fast nearest-neighbor downscaling. It preserves the aspect ratio and does not upscale. A large reduction can therefore produce jagged or soft-looking edges, especially around transparency, while decoding still processes the original full-resolution frame. Runtime scaling is convenient when the display size is not known in advance, but pre-scaling the GIF avoids this quality limitation and reduces decoder and scaling work.
```c
#include "lv_efc.h"

lv_obj_t *emote = lv_efc_create(parent);

/* Copies the raw file bytes. A path also works: lv_efc_set_src(emote, "/sdcard/smile.efc") */
lv_efc_set_src_data(emote, efc_data, efc_size);
lv_efc_play(emote);
```

`lv_efc_set_src()` takes one source: a POSIX path, an LVGL filesystem path, or an `lv_image_dsc_t`. It does not take a pointer plus a length. Use `lv_efc_set_src_data()` for a memory blob. `lv_efc_play()` restarts at frame 0. `lv_efc_pause()` holds the current frame, and `lv_efc_resume()` continues.

This makes an EFC animation usable like other LVGL UI objects instead of requiring the application to manage:

* frame timing
* frame iteration
* framebuffer updates
* invalidation / redraw
* animation playback state

### When to use which?

| Application                     | Recommended API |
| ------------------------------- | --------------- |
| Custom LCD/display driver       | **EFC decoder** |
| Custom rendering pipeline       | **EFC decoder** |
| Non-LVGL application            | **EFC decoder** |
| LVGL application                | **LVGL object** |
| Animation as part of an LVGL UI | **LVGL object** |
| Need full control over decoding | **EFC decoder** |

The important distinction is simple:

> **Use the decoder when you control the rendering pipeline. Use the LVGL object when you just want an animation in your LVGL UI.**

---

## How EFC works

EFC is optimized around a common property of animated faces and UI graphics:

**most of the image often stays unchanged between frames.**

Instead of rebuilding the complete image every frame, EFC divides the image into blocks.

```text
Frame N
┌────┬────┬────┬────┐
│    │    │    │    │
├────┼────┼────┼────┤
│    │ ██ │ ██ │    │
├────┼────┼────┼────┤
│    │    │    │    │
└────┴────┴────┴────┘

Frame N+1
┌────┬────┬────┬────┐
│    │    │    │    │
├────┼────┼────┼────┤
│    │ ██ │    │    │
├────┼────┼────┼────┤
│    │    │    │    │
└────┴────┴────┴────┘

Only changed blocks need to be decoded.
```

This provides two benefits at once:

**less data stored + less work performed during playback.**

### Core properties

* **Block decoding** — frames are processed in small blocks
* **Inter-frame reuse** — unchanged blocks reuse the previous framebuffer contents
* **In-place decoding** — no second full-frame buffer is required
* **RGB565 output** — directly suitable for embedded displays
* **Lossless** — no image quality loss
* **Optimized fill/copy paths** — ESP32-S3 and ESP32-P4 use optimized implementations

---

## Performance

Measurements were taken on **ESP32-S3**.

Each file was played once (`iters=1`). Mean frame time is weighted by frame count.

Every retained clip was decoded four times and matched the expected pixels.

### Eye emulation — 23 clips

This is the workload EFC is primarily optimized for: few colors, large unchanged regions, and relatively small changes between frames.

|            |     GIF |    EAF RLE | EAF RLE + Huff |         **EFC** |  **EFC + zlib** |
| ---------- | ------: | ---------: | -------------: | --------------: | --------------: |
| Stored     | 3.46 MB |   12.85 MB |        8.50 MB |     **1.94 MB** |     **0.65 MB** |
| vs GIF     |      1× |       3.7× |           2.5× |       **0.56×** |       **0.19×** |
| Decode     |       — |  77.9 ms/f |      81.9 ms/f |   **25.7 ms/f** |   **25.8 ms/f** |
| Throughput |       — | 6.6 Mpix/s |     6.2 Mpix/s | **19.9 Mpix/s** | **19.8 Mpix/s** |
| vs EAF RLE |       — |         1× |          0.95× |       **3.03×** |       **3.02×** |

GIF decode time was not measured in this benchmark; the GIF column is provided for source-file size comparison.

zlib reduces the EFC set from **1.94 MB to 0.65 MB** while adding almost no decode cost for this workload.

### Real GIFs — 28 clips

Real-world GIFs generally have more colors and more frame-to-frame movement, reducing the benefit of inter-frame reuse.

|            |     GIF |    EAF RLE | EAF RLE + Huff |        **EFC** | **EFC + zlib** |
| ---------- | ------: | ---------: | -------------: | -------------: | -------------: |
| Stored     | 39.8 MB |    58.8 MB |        47.9 MB |    **22.6 MB** |    **17.4 MB** |
| vs GIF     |      1× |       1.5× |           1.2× |      **0.57×** |      **0.44×** |
| Decode     |       — |  29.3 ms/f |      55.8 ms/f |  **18.4 ms/f** |      24.8 ms/f |
| Throughput |       — | 4.6 Mpix/s |     2.4 Mpix/s | **7.3 Mpix/s** |     5.4 Mpix/s |
| vs EAF RLE |       — |         1× |          0.52× |      **1.59×** |          1.18× |

The more an animation changes between frames, the less EFC can exploit unchanged blocks.

---

## When EFC is a good fit

Use EFC when you have:

* Short looping animations
* RGB565 displays
* Limited RAM
* Flash-sensitive applications
* Animations with relatively large unchanged regions
* LVGL-based UI that needs lightweight animated objects

EFC is not intended to replace GIF as a general-purpose interchange format. It is optimized for **embedded playback**, where flash size, decode time, RAM usage, and simple integration all matter.

---

## Typical applications

EFC is particularly suitable for:

* **animated badges**
* Desk pets and companion robots
* Voice-assistant faces
* Smart-speaker animations
* Round watches
* Small LVGL displays
* Animated UI feedback
* Loading / success / error animations
* Eye and facial-expression animations
* Retail and appliance UI effects

---

## Convert GIF to EFC

The encoder is PC-only. This release includes the standalone browser and
Node.js tools under `pc/js/dist`.

Recommended settings for the ESP32 decoder:

* **Tile:** `4`
* **zlib:** enabled
* **Mode:** predictor planes (`PRED`)

PRED is recommended for animated clips because it usually reduces the file and decode work. Legacy opcodes remain available for comparison.

### Browser

Open:

```text
pc/js/dist/encode.html
```

The page is self-contained. No network connection or local server is required.

1. Select a GIF.
2. Leave **Tile** at `4 (device decoder)`.
3. Leave **zlib** enabled.
4. Select **predictor planes**.
5. Click **Encode**.
6. Click **Download**.

The page shows the GIF size, EFC size, frame count, and encoding time.

### Node.js

Node.js 18 or later:

```bash
cd pc/js/dist
node encode_gif.mjs smile.gif smile.efc 4 z pred
```

The released script includes its dependencies; `npm install` is not required.

Disable zlib with:

```bash
node encode_gif.mjs smile.gif smile.efc raw
```
Use legacy opcodes instead:

```bash
node encode_gif.mjs smile.gif smile_legacy.efc 4 z legacy
```

Encode multiple GIFs:

```bash
mkdir -p out

for g in *.gif; do
    node encode_gif.mjs "$g" "out/${g%.gif}.efc"
done
```
---

## Convert a video to EFC

`pc/video_to_efc.py` turns a short MP4, AVI, or GIF into an `.efc` file. It needs Python 3, NumPy, Pillow, and FFmpeg. One-time setup:

```bash
python -m pip install numpy pillow
```

Run the following commands from this directory. Choose a workflow according to the source and the required output:

* Use **lossless RGB565** when preserving the decoded source pixels matters more than size.
* Use **GIF-style palette conversion** for clean graphics that need stable color reduction.
* Use **H.264 smoothing** for photographic, noisy, or heavily textured animation.
* Use a **file-size target** when the output must fit a fixed flash budget. It can be combined with H.264 smoothing.
* Add `--legacy` to any workflow to disable the newer predictor-plane (`PRED`) encoding and emit only legacy tile opcodes. This is useful for older decoders or format comparison, but usually produces a larger EFC file.

### 1. Preserve the source pixels

Use `--lossless` to skip denoising, palette reduction, temporal hold, and internal scaling. EFC stores the decoded source as RGB565 without further lossy processing:

```bash
python pc/video_to_efc.py clip.mp4 clip_lossless.efc --lossless
```

### 2. Reduce colors while keeping stable visual quality

Use `--via-gif` for GIF-style global palette quantization followed by lossless EFC encoding. This is preferred when you want a smaller file without the floating noise that temporal dithering or aggressive hold can introduce:

```bash
python pc/video_to_efc.py clip.mp4 clip_palette.efc --via-gif
```

### 3. Smooth complex sources before EFC encoding

`--h264-crf` performs an H.264 round trip with x264's animation tuning before the normal palette and EFC stages. This is a lossy preprocessing step: it automatically smooths fine noise and texture so that the final EFC is often much smaller while retaining good visual quality at conservative settings.

Start with CRF 18:

```bash
python pc/video_to_efc.py noisy.gif noisy.efc --h264-crf 18 --cmp /tmp/noisy_cmp.png
```

If the EFC is still too large, try CRF 30 and inspect the comparison image or preview. A higher CRF usually makes a smaller EFC, but it also removes more texture and can introduce contouring or temporal pumping:

```bash
python pc/video_to_efc.py noisy.gif noisy_small.efc --h264-crf 30 --cmp /tmp/noisy_small_cmp.png
```

This workflow requires an FFmpeg build with the `libx264` encoder. Set `EFC_FFMPEG` when that build is not the default `ffmpeg` on `PATH`;

```bash
EFC_FFMPEG=$HOME/ffmpeg/ffmpeg/ffmpeg python pc/video_to_efc.py noisy.gif noisy.efc --h264-crf 18
```

Lossy inputs are denoised automatically. Use `--denoise off` when preserving fine texture is more important than the additional size reduction. Quality metrics and `--cmp` still compare the decoded EFC with the original input, not with the H.264 intermediate.

### 4. Fit a file-size target

Use `--size` with bytes or a `K`/`M` suffix:

```bash
python pc/video_to_efc.py clip.mp4 clip_300k.efc --size 300K
```

The search keeps every frame. It first lowers the palette color count. If that is still too large, it gradually increases the 4×4 temporal deadzone. Internal 4/5, 4/6, 1/2, then 1/4 scaling is the last resort. It keeps the highest-quality candidate that fits the requested size and reports when the target cannot be reached.

For a complex source, combine the conservative H.264 starting point with the size search:

```bash
python pc/video_to_efc.py noisy.gif noisy_300k.efc --h264-crf 18 --size 300K
```

An `.efc` file does not store a frame rate. Play it with a delay of `duration_ms / frame_count`.

---

## Play EFC on a PC

`pc/efc_play.py` opens an `.efc` file in a window. It uses Tk when available and otherwise falls back to `ffplay`. The file has no frame rate, so the player uses 15 fps unless you set one.

```bash
python3 pc/efc_play.py clip.efc
python3 pc/efc_play.py clip.efc --fps 15
python3 pc/efc_play.py clip.efc --delay-ms 67
python3 pc/efc_play.py clip.efc --dump /tmp/frame0.png
```

| Key | Action |
| --- | --- |
| Space | Pause or resume |
| Left / Right | Step one frame, and pause |
| Up / Down | Faster or slower |
| C | Cycle the backdrop: checker, black, white |
| Q or Escape | Quit |

The color key is drawn over the backdrop. `--dump` writes frame 0 and exits.

---

## LVGL Examples

Refer [emote_player](examples/emote_player/) for details integration of `esp_efc_codec` to your LVGL project.


---

## Technical Support and Feedback

* Technical questions: [esp32.com forum](https://esp32.com/viewforum.php?f=20)
* Bugs and feature requests: open an issue in the `esp_efc_codec` project
