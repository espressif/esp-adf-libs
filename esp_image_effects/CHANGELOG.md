# Changelog

## v1.2.0

### Features

- Added C and PIE assembly-optimized library variants for **ESP32-S31**.
- Added prebuilt library selection for different **ESP32-P4** chip revisions.
- Enabled PIE acceleration for **ESP32-S31** and **ESP32-P4**.

## v1.1.0

### Features

- Added support for the **ESP32-C61** and **ESP32-S31** chips
- Added **YUYV** and **UYVY** format support for the `scale` module
- Added **YUYV** and **UYVY** format support for the `crop` module

## v1.0.1

### Features

- Added support for the **ESP32-H4** chip

### Bug Fixes

- Fixed compatibility issues when using **ESP-IDF versions earlier than v5.5**  
- Fixed a **memory leak** in the `scale` module

## v1.0.0

### Features

- Initial version of `esp-image-effects`
- Add image effects module for `scale`, `crop`, `color convert`, `rotate`

