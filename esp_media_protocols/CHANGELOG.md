# Changelog

## v1.0.0

### Features

- Added support for SIP MESSAGE method
- Added support for re-INVITE
- Added support for custom SDP RTP payload type
- Added option to suspend refresh register during a call
- Added support for configuring private header at init for the first message
- Added register refresh keepalive event
- Added support for RFC2435 JPEG over RTP assemble and parse
- Added support for configurable RTP frame size
- Added support for reading raw SIP headers for custom parsing
- Added support for `esp32c61` target

### Bug Fixes

- Fixed refresh register reusing previous `call_id`
- Fixed SDP negotiation failure when only video is present
- Fixed crash caused by race condition when stopping RTP
- Fixed `use_public_addr` always using the default route
- Fixed the abnormal organization of request-URI, branch, and from tag in special cases

## v0.6.2

### Features

- Update dependency version of `espressif/media_lib_sal`

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

### Bug Fixes

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
