# Changelog

## v0.5.4

### Features

- Added `esp32s31` target support
- Added more target `esp32h4`, `esp32h4`, `esp32c61` support for `esp_new_jpeg`
- Extend input formats of hardware H.264 encoder for IC rev300+ `esp32p4`
  - Formats `RGB565LE`, `UYVY422`, `BGR888`

## v0.5.3

### Features

- Added target dependency for `esp_new_jpeg`

## v0.5.2

### Features

- Align video codec and pixel format with `esp-gmf`

## v0.5.1

### Features

- Fix build issue for board not support `esp_h264`

## v0.5.0

### Features

- Initial version of `esp-video-codec`
- Add video encoder and decoder common part
- Add hardware MJPEG encoder and decoder implement based on `esp_driver_jpeg`
- Add hardware H264 encoder implement based on `esp_h264`
- Add software MJPEG encoder and decoder implement based on `esp_new_jpeg`
- Add software H264 encoder and decoder implement based on `esp_h264`
