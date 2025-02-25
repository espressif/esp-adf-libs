# Changelog

## v2.2.1

### Bug Fixes

- Compile libraries with optimial compilation option

## v2.2.0

### Features

- Support g711a, g711u, pcm, adpcm decoder in audio simple decoder
- Add pcm decoder in esp_audio_codec

### Bug Fixes

- Fix a bug that dtx is not effect in opus encoder
- Complete error log in esp_audio_codec

## v2.1.0

### Features

- Support raw opus decoder in audio simple decoder
- Add a breaking change for the Opus encoder to support VBR
- Add a breaking change to the Opus decoder to support a specific frame duration

### Bug Fixes

- Fix a bug that opus decoder not check out buffer length
- Add error log when decoder failed to decode frame
- Fix a bug that out_buf size is larger than in_buf size in audio encoder

## v2.0.3

### Features

- Fix opus decode error


## v2.0.2

### Features

- Support MP2 and MP1 decode

### Bug Fixes

- Fix FLAC mono channel decode noise


## v2.0.1

### Features

- Support esp32c2 and esp32c5 board

### Bug Fixes

- Decrease memory usage when decode AAC-Plus
- Clear search position when simple decoder find valid frame to avoid accumulate and generate decoder error


## v2.0.0

### Features

- Add audio decoder common APIs to operate all supported decoders
- Add audio decoder implementation for `AAC`, `MP3`, `OPUS`, `ADPCM`, `G711A`, `G711U`, `AMRNB`, `AMRWB`, `VORBIS`, `ALAC`
- Add audio simple decoder common APIs to operate all supported simple decoders
- Add audio simple decoder implementation for `AAC`, `MP3`, `M4A`, `TS`, `AMRNB`, `AMRWB`, `FLAC`, `WAV`
- Add `esp_es_parse` to easily parse and get audio frame
- Add audio encoder registration and customization support through `esp_audio_enc_register`, deprecate `esp_audio_enc_install` and `esp_audio_enc_uninstall` APIs
- Add audio decoder registration and customization support through `esp_audio_dec_register`
- Add audio simple decoder registration and customization support through `esp_audio_simple_dec_register`
- Add audio encoder support for `ALAC`
- Refine memory usage when not enable AAC-Plus support
- Reorganized code layout, separate implementation from common part
- Add default registration for supported audio encoders (controlled by menuconfig) through `esp_audio_enc_register_default`
- Add default registration for supported audio decoders (controlled by menuconfig) through `esp_audio_dec_register_default`
- Add default registration for supported audio simple decoders (controlled by menuconfig) through `esp_audio_simple_dec_register_default`
- Add memory management through `media_lib_sal` and can be traced through [mem_trace](https://github.com/espressif/esp-adf-libs/tree/master/media_lib_sal/mem_trace)

### Bug Fixes

- Fix audio encoder PTS calculation error when run long time


## v1.0.1

### Bug Fixes

- Fix `CMakeLists.txt` hardcode on prebuilt library name
- Refine test code in [README.md](README.md)


## v1.0.0

### Features

- Initial version of `esp-audio-codec`
- Add audio encoder common part
- Add audio encoder implementation for `AAC`, `OPUS`, `ADPCM`, `G711A`, `G711U`, `AMRNB`, `AMRWB`, `PCM`
