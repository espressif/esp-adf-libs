# Changelog

## v1.0.2

### Features

- Added support for `esp32c61` target

### Bug Fixes

- Fixed ANNEX-B H264 video in AVI can not playable
- Fixed extractor wrongly close or close hangup due to atomic operation fails
- Fixed OGG and AAC seek to 0, wrong PTS output
- Fixed AVI output frame size wrongly when read frame need waiting for output

## v1.0.1

### Features

- Added support for `esp32s31` target

## v1.0.0

### Features

- Initial version of `esp_extractor`
- Added comprehensive support for MP4, TS, FLV, WAV, OGG, HLS, AVI, Audio ES containers
- Implemented unified API for all extractor types
- Added memory pool management and optimization features
- Introduced advanced control mechanisms for fine-tuning extraction behavior
