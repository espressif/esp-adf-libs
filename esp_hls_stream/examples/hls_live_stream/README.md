# HLS live stream Example

- [中文版](./README_CN.md)
- Complex Example: ⭐⭐⭐

## Example Brief

This example shows how to extract and play an **HLS (HTTP Live Streaming)** stream using **`esp_hls_stream`**.

It includes two usage paths:

- **Default**: Play an HLS audio stream directly to the board’s audio output.
- **Optional**: Use the HLS extractor (`parse_hls`) to parse HLS and read A/V frames.

### Typical scenarios

- Internet radio and live broadcasting
- Podcasts and on-demand audio streaming
- DRM-protected audio (premium music services)

### Technical details

### HLS audio player (`hls_live_simple_player.c`)

The HLS I/O acts like a normal reader I/O and feeds data into the audio simple player. The simple player finds frame boundaries, decodes, and outputs PCM through **`esp_codec_dev`**:

`HLS I/O` → `esp_audio_simple_player` → `esp_codec_dev`

To detect the real container/codec format, register **`file_seg_cb`** on the HLS I/O and call **`esp_gmf_audio_dec_reconfig_by_sound_info()`** so the decoder matches the detected format.

### HLS extractor (`hls_extractor_test.c`)

Use the helper API **`esp_hls_extractor_io_cfg_init()`** to build the HLS extractor configuration. Then use the common extractor APIs to drive HLS: obtain audio/video stream information and read each frame.

The HLS extractor uses the TS extractor internally for MPEG-TS segments. **You need to register both** before use:

- **`esp_hls_extractor_register()`**
- **`esp_ts_extractor_register()`**

### Run flow

1. Install the media OS adapter and initialize the audio device via the board manager.
2. Connect to Wi-Fi (`example_connect()`).
3. Create the audio simple player.
4. Initialize HLS I/O (`esp_gmf_io_hls_init()`).
5. Register the I/O with the player.
6. Start playback: `esp_audio_simple_player_run_to_end(player, HLS_STREAM_URL, NULL)`.
7. Clean up on exit.

## Environment Setup

### Hardware required

- An Espressif board supported by **`esp_board_manager`** with:
  - **Audio DAC / playback path**
- **PSRAM** is enabled in `sdkconfig.defaults` (`CONFIG_SPIRAM=y`).

### Default IDF branch

This example supports **IDF `release/v5.4` (≥ v5.4.3)** and **`release/v5.5` (≥ v5.5.2)**.

### Software requirements

- Managed components are resolved from the example and component **`idf_component.yml`** files on the first build (a network connection may be required).
- ESP-IDF **`install.sh`** / **`export.sh`** as usual.

## Build and Flash

### Build Preparation

Before building, ensure the ESP-IDF environment is set up. If it is not, run the following in the ESP-IDF root directory (full steps in the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/index.html)):

```
./install.sh
. ./export.sh
```

Go to **this** example directory (adjust if your clone layout differs):

```
cd <path-to-repo>/esp_hls_stream/esp_hls_stream/examples/hls_live_stream
```

Select the target chip and board configuration with the board manager (list boards, then pick one that matches your hardware):

```bash
idf.py gen-bmgr-config -l
idf.py gen-bmgr-config -b <your_board_id>
```

Replace `<your_board_id>` with your actual board id (boards differ by chip).

### Project configuration

- **Wi-Fi**: Configure **Example Connection Configuration** (or your project’s Wi-Fi menu) so **`example_connect()`** can join your access point.
- **HLS URL**: Edit **`HLS_STREAM_URL`** in `main/hls_live_simple_player.c` if the default stream is unreachable or you need a different source.
- **Audio simple player**: Resample, channel, and bit-depth defaults are in `sdkconfig.defaults` (e.g. **48 kHz**, **2 ch**, **16-bit**).

### Build and Flash Commands

Build, flash, and monitor:

```
idf.py -p PORT flash monitor
```

Exit the monitor: **Ctrl+]** (or your ESP-IDF default).

## How to Use the Example

### Functionality and Usage

- The device connects to Wi-Fi and starts playback automatically.
- Audio is output through the board DAC.
- Logs are tagged with **`HLS_LIVE_STREAM`**.

### Optional: Extractor mode

```c
parse_hls(HLS_STREAM_URL, 30000);
```

- Enable this in `app_main` or call it from your own code path.
- Set the duration argument to how long you want the test to run (milliseconds).

## Technical Support

- Forum: [esp32.com](https://esp32.com/viewforum.php?f=20)
- Issues (GMF / related): [esp-gmf on GitHub](https://github.com/espressif/esp-gmf/issues)

We will reply as soon as possible.
