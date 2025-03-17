# Changelog

## v0.6.1

### Features

- Optimize error handling logic of the decoder
- Improve example of encoder block mode in test_app

### Bug Fixes

- Fix decoding error for some images containing restart markers

## v0.6.0

### Features

- Support IDF5.4
- Support SD power supply comes from internal LDO IO in test_app
- Add 16-byte alignment check for encoder input buffer when using ESP32-S3
- Add 16-byte alignment check for decoder output buffer when using ESP32-S3

### Bug Fixes

- Fix inaccurate `inbuf_remain` member in `jpeg_dec_io_t` structure after decoding is finished
- Fix spelling errors in test_app

## v0.5.1

### Bug Fixes

- Fix memory leakage on jpeg_enc_open fail
- Fix memory allocation fail when dram is insufficient

## v0.5.0

### Features

- Initial version of `esp_new_jpeg`
- Add JPEG encoder and decoder library
