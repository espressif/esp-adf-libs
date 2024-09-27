# Changelog

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
