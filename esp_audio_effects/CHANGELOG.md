# Changelog

## v1.1.0

### Features

- Supported `esp-audio-effects` for `esp32-h4`
- Supported mixer outbuf is one of inbuf

## v1.0.2

### Bug Fixes

- Compile libraries using the -o2 compilation option

## v1.0.1

### Bug Fixes

- Fix `test_sonic` test code error
- Remove `sdkconfig.default` have some configuration strings that cannot be detected by the latest IDF

## v1.0.0

### Features

- Initial version of `esp-audio-effects`
- Add audio effects module for `ALC`, `BIT_CVT`, `CH_CVT`, `RATE_CVT`, `EQ`, `FADE`, `DATA_WEAVER`, `MIXER`, `SONIC`
