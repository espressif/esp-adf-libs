# Changelog

## v0.6.1

### Features

- Added support for `esp32s31`, `esp32h4` target
- Added test_apps for compilation verification

## v0.6.0

### Features

- Added domain for a single server divided into multiple domains
- Added TCP/TLS keep-alive mechanism
- Added configurable read/write and connect timeouts
- Added response handling for NOTIFY messages

### Bugfix

- Removed authentication headers from unwanted methods
- Fixed missing expire handling in Contact and failure to reset after REGISTER OK
- Removed Expires header from unwanted methods
- Removed retry send logic for TLS/TCP
- Fixed incorrect SIP message parsing when peer flushes input over TCP/TLS
- Improved safety of `strcasecmp` to prevent NULL pointer crashes
- Refined TLS error log output
- Fixed incorrect ACK request sending
- Fixed incorrect handling of `200 OK` responses after a CANCEL

## v0.5.1

### Features

- Update dependency to `espressif/media_lib_sal`

## v0.5.0

### Features

- Initial version of `esp_media_protocols`
- Add media protocols library
- Add a breaking change for the pts added to esp rtsp send stream
