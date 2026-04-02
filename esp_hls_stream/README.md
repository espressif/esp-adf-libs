## esp_hls_stream

- [中文版](./README_CN.md)

`esp_hls_stream` provides **HTTP Live Streaming (HLS)** support on the Espressif platform: playlist and segment handling, optional encryption, and integration with **ESP Extractor** and **ESP-GMF I/O**.

## Background

HLS is an adaptive bitrate streaming protocol carried over HTTP. Clients use a **playlist** (often with the **`.m3u8`** extension) and fetch **media segments** instead of a single large file, which suits web, mobile, and embedded players behind standard HTTP infrastructure.

Streams may be protected with **`#EXT-X-KEY`** methods such as **AES-128** or **SAMPLE-AES** for **encryption and content protection** (including **DRM** scenarios, depending on the service and key delivery).

## Key features

- HLS **audio and video** demuxing via the extractor path
- **AES-128** encrypted media
- **SAMPLE-AES** and related HLS encryption modes where supported by the implementation

## Architecture

The component exposes **two integration levels**.

### 1. Extractor-compatible API

This path supports the **full** HLS feature set used by the library:

- Audio and video extraction
- AES-128
- SAMPLE-AES

It delivers **compressed** audio and video **access units** (or equivalent frame data) that you can pass to decoders according to your extractor configuration.

**Registration and typical flow**

- Register the HLS extractor with `esp_hls_extractor_register()`, then use it like any other extractor implementation.

Typical sequence:

`esp_extractor_open` → `esp_extractor_parse_stream` → `esp_extractor_read_frame` → `esp_extractor_close`

> **Note:** The HLS extractor relies on other registered extractors (for example **TS** and, where applicable, **audio ES**) to parse container and elementary streams. Ensure those extractors are **registered** before use.

### 2. GMF I/O-compatible API

The HLS module can act as a standard **ESP-GMF reader I/O**: applications read a **continuous byte stream** through the GMF I/O interface. This path is convenient and is tuned especially for **audio** streaming.

**Limitations**

- **Audio only** (no full A/V pipeline on this path as documented for the I/O implementation)
- **SAMPLE-AES** is **not** supported on this path
- **No guaranteed frame boundaries**; use downstream elements (for example **`esp_audio_simple_dec`**) to locate and decode individual audio frames

**Integration**

Call `esp_gmf_io_hls_init()`, register the resulting I/O in your **GMF pool**; when the URL or source is recognized as HLS, this I/O can be selected automatically according to your player or pipeline setup.

### HLS helper APIs

To simplify extractor setup, helper routines fill in **playlist/segment I/O** and related settings:

- **`esp_hls_extractor_file_cfg_init`** — Local files via **libc-style** file I/O (pair with `esp_hls_extractor_file_cfg_deinit`).
- **`esp_hls_extractor_io_cfg_init`** — **Local, HTTP, or mixed** access using an **external GMF pool** handle (pair with `esp_hls_extractor_io_cfg_deinit`).

Use **`esp_hls_extractor_open_with_cfg()`** to open the extractor with the prepared configuration. See **`esp_hls_helper.h`** and **`esp_hls_types.h`** for structure definitions.

## Dependencies

| Component | Purpose |
|-----------|---------|
| `espressif/esp_extractor` | Extractor framework and stream masks |
| `espressif/media_lib_sal` | Media and crypto helpers |
| `espressif/gmf_io` | GMF I/O types and registration |

## Configuration

In **menuconfig**: **Component config** → **Espressif HLS Stream Configuration**

| Option | Description |
|--------|-------------|
| `HLS_STREAM_EXTRACTOR_SUPPORT` | Builds support for the **HLS extractor** API (default: **enabled**) |
| `HLS_STREAM_IO_SUPPORT` | Builds support for the **HLS GMF I/O** API (default: **enabled**) |

For **thread, buffer, and optional callbacks** on the GMF I/O path, see **`esp_hls_io_cfg_t`** and related defaults in **`esp_hls_io.h`**.

## Usage and Example

End-to-end integration is illustrated in the [hls_live_stream](examples/hls_live_stream/README.md) example:

| Path | What the example shows |
|------|-------------------------|
| **GMF I/O** | `esp_gmf_io_hls_init()` with **Audio Simple Player**, Wi-Fi, and board audio output (`hls_live_simple_player.c`). |
| **Extractor** | **`parse_hls()`** — pool-based HTTP/file I/O, and frame reads (`hls_extractor_test.c`). |


## Technical Support

For technical support, use the links below:

- Technical support: [esp32.com](https://esp32.com/viewforum.php?f=20) forum
- Issue reports and feature requests: [GitHub issue](https://github.com/espressif/esp-gmf/issues)

We will reply as soon as possible.
