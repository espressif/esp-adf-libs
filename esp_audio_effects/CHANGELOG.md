# Changelog

## v1.2.0

### Features

- Added audio effects module for `DRC`, `MBC`
- Added `esp_ae_xx_reset` function for all effect modules
- Added per-stream enable/disable control for `Mixer`
- Added esp_audio_effects demo
- Optimized effects implementation to reduce binary size
- Added refine code and license for test app

### Bug Fixes

- Fixed an issue where `ALC` failed to operate correctly when the input level was too low
- Fixed `FADE` module reset not taking effect
- Fixed a bit-depth conversion error when converting 8-bit to 24-bit

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
