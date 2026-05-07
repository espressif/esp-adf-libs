# Changelog

## v1.2.1

### Features

- Added support for `esp32s31`, `esp32h4` target
- Fixed document error for `AVI`

## Bug Fixes

- Fixed test app build error on IDFv6.x

## v1.2.0

### Features

- Added AVI muxer with support for:
  - Video: H.264, MJPEG
  - Audio: AAC, MP3, PCM
  - Index table (idx1) placement: none, file start, or file end
- Added `video_muxer` example that uses `esp_capture` to simplify muxer usage for realtime media

## Bug Fixes

- Fixed build failed on path with whitespace

## v1.1.3

### Features

- Added FourCC codec support
- Added file pattern with context support
- Added dynamic register to list instead of fixed array
- Added `esp_muxer_default.h` for default registration and un-registration
- Added basic unit test for audio muxer support

## Bug Fixes

- Fixed crash when open file to write failed
- Fixed OGG OPUS PTS generate not correct

## v1.1.2

### Features

- Support esp32c2 and esp32c5 board


## v1.1.1

### Features

- Add key frame parser for H264 (in case that `key_frame` flag not set correctly)
  User can disable it by set `no_key_frame_verify`
- Always write file slice on key frame boundary (for H264) to avoid lost data during decoding


## v1.0.1

### Features

- Add CAF, OGG container support
- Add ALAC and OPUS new audio codec support
- Support write to storage use aligned RAM cache to increase write speed through `ram_cache_size`


## v1.0.0

### Features

- Initial version of `esp-muxer`
- Support MP4, TS, FLV, WAV container
