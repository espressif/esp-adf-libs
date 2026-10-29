# Changelog

## v1.1.0

### Features

- **Added prediction support**
  - Reduces file size and improves decoding speed for complex images with many colors
  - Supports disabling prediction by enabling the legacy option in the EFC converter tools

- **Added tile-based scaling support**
  - Designed to achieve extremely compact file sizes
  - Supports fixed scaling ratios of **4/5, 2/3, 1/2, and 1/4**
  - Reuses EFC's built-in `SKIP` and `REPEAT` mechanisms to encode only changed tiles, reducing redundant data
  - Best suited for emotes and animations with soft edges or details that remain visually acceptable when scaled up

- **Added video-to-EFC conversion tools**
  - Supports direct video-to-EFC conversion
  - Supports lossless conversion via `--lossless`
  - Supports converting video to GIF before EFC encoding via `--via-gif`
  - Supports noise reduction and smoothing through H.264 encoding via `--h264-crf`
  - Supports automatic file-size targeting by searching for suitable encoding parameters via `--size`
  - Requires `ffmpeg`. The `--h264-crf` option additionally requires an `ffmpeg` build with `libx264` enabled

- **Added an EFC player**
  - Plays generated EFC files
  - Supports playback control, including `pause`, `resume`, and frame-by-frame `step`
  - Supports playback speed adjustment through FPS control

## v1.0.0

### Features

- Initial version of `esp_efc_codec`
  - Effective support for emote clips
  - Support for both inter/intra codec
  - Support for PC convert tools
