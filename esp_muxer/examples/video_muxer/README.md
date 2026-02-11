# Video Muxer Example

This example demonstrates how to **mux audio and video** from capture sources (camera and microphone) and **save into storage** (SD card) using the ESP Muxer component.

## Overview

To simplify the process of whole flow, this application uses `esp_capture` with `esp_muxer` support internally:

1. **Captures** video from a camera (V4L2) and audio from the board's audio ADC (codec).
2. **Encodes** video (MJPEG or H264) and audio (AAC).
3. **Muxes** the encoded streams into container formats (TS, FLV, MP4, AVI).
4. **Writes** the muxed file to the SD card (e.g. `/sdcard/muxed_h264.ts`, `/sdcard/muxed_mjpeg.flv`).

Sync is driven by the **audio** source (`ESP_CAPTURE_SYNC_MODE_AUDIO`).

## Typical Use Cases

This example is a ready-made starting point for products that need to **record camera + microphone to a file on storage**:

| Use case | Description |
|----------|-------------|
| **Dashcam / 行车记录仪** | In-vehicle recording: continuous or event-triggered video + audio to SD card. |
| **PVR (Personal Video Recorder)** | Local recording from camera/mic for security, meetings, or streaming capture. |
| **Body cam / wearable recorder** | Portable A/V recording with optional loop/overwrite and file slicing. |
| **Security / NVR-style** | Multi-session or time-sliced recording (e.g. by `slice_cb` and duration). |

> **Simple usage:** Use this example as-is for a quick prototype: connect camera, audio ADC, and SD card, then build and run. You get **muxed A/V files** (TS, FLV, etc.) on the SD card with no extra application logic. Tune `slice_cb`, `MAX_RECORD_DURATION`, and `muxer_lists[]` to match your product (e.g. loop recording, one file per event, or one container format).

## Hardware / Prerequisites

- Board with:
  - **Camera** CSI, DVP (supported by v4l2)
  - **Audio ADC** (codec) for microphone input
  - **SD card** mounted (e.g. as default storage)
- ESP-IDF V5.5.2 or later and this component built for your target (e.g. ESP32, ESP32-S3, ESP32-P4)

## Muxer and Stream Settings

| Item | Default |
|------|--------|
| **Video codec** | MJPEG; H264 also used when available |
| **Video resolution** | 640×480 @ 5 fps (or 1280×720 @ 30 fps on ESP32-P4) |
| **Audio codec** | AAC |
| **Audio** | 16 kHz, 2 channels, 16-bit |
| **Containers** | TS, FLV (MP4/AVI supported in code) |
| **Output path** | `/sdcard/muxed_<codec>.<ext>` (e.g. `muxed_h264.ts`, `muxed_mjpeg.flv`) |

Output path and slice behavior are controlled by the **slice callback** (`slice_cb`), which returns the file path for each slice (here one file per recording).

## Storage (File Output)

- The muxer is configured with a **slice callback** so that each “slice” is written to a file path you provide.
- **Muxer is enabled; streaming is disabled** so that data goes only to the file:
  - `esp_capture_sink_enable_muxer(sink, true)`
  - `esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_VIDEO)`
  - `esp_capture_sink_disable_stream(sink, ESP_CAPTURE_STREAM_TYPE_AUDIO)`
- Recording runs for **`MAX_RECORD_DURATION`** (10 seconds by default) per format/codec.

## Build and Run

- Select target board firstly

```
idf.py gen-bmgr-config -l
idf.py gen-bmgr-config -b esp32_s3_korvo2_v3
```
- Build and flash

```bash
idf.py build
idf.py -p <port> flash monitor
```

> [!NOTE]
> Target is auto guessed after select board no need `idf.py set-target` anymore
> For other supported boards (via `esp_board_manager`), follow the same steps above.
> For customer board refer to the [Custom Board Guide](https://github.com/espressif/esp-gmf/blob/main/packages/esp_board_manager/docs/how_to_customize_board.md) for details.

## Code Flow (Summary)

1. **Board init**: Camera, audio ADC, SD card (required for file output).
2. **Register** default audio encoder, video encoder, and muxer.
3. **Create** video source (camera) and optional audio source (codec).
4. **Open** capture with audio sync, then for each muxer type:
   - **Setup sink** with video and audio format (codec, resolution, sample rate, channels).
   - **Add muxer** with slice callback so output goes to `/sdcard/muxed_<codec>.<ext>`.
   - **Enable muxer**, disable video/audio streaming.
   - **Start** capture, wait for `MAX_RECORD_DURATION`, then **stop**.
5. **Deinit** capture and sources, unregister defaults.

## Notes

- If SD card mount fails, the example skips running and logs "Skip muxer for mount sdcard failed".
- Encoder and scheduler (stack size, core, priority) are tuned for H264/OPUS in the example; you can change them in `capture_scheduler` and the format defines.
- To record only one container or codec, change `muxer_lists[]` or the video codec passed to `record_file()`.
