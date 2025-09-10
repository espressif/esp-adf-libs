# ESP_MEDIA_PROTOCOLS
[中文版](./README_CN.md)

## Overview

The ESP Media Protocols library provides comprehensive implementations of popular media streaming and communication protocols optimized for Espressif Socs. This library enables developers to build sophisticated multimedia applications including VoIP systems, streaming servers, smart home devices, and synchronized audio systems.

# Features

## Supported Protocols

| Protocol   |Description                                                       |
|------------|------------------------------------------------------------------|
| SIP        | Session Initiation Protocol, suitable for establishing, modifying and terminating calls, commonly used in VoIP communications |
| RTSP       | Real Time Streaming Protocol, supports media streaming play, pause, seek and etc. |
| RTMP       | Real-Time Messaging Protocol, suitable for pushing and pulling live streaming |
| MRM        | Multi-Room Music, supports audio distribution and sync across devices |
| UPnP       | Universal Plug and Play, allows devices to discovery and interoperate, commonly used for home network devices |

### SIP Protocol

Session Initiation Protocol implementation for audio and video communication over IP networks.
- **Readme:** [ESP_SIP](docs/en/SIP_README.md)
- **Headers:** [`esp_rtc.h`](include/esp_rtc.h), [`esp_sip.h`](include/esp_sip.h)(deprecated)
- **Key Features:**
  - Multiple audio/video codecs
  - DTMF support (RFC2833)
  - SSL/TLS encryption
  - Custom SIP headers
- **Use Cases:**
  - IP phones and VoIP applications
  - Video conferencing systems
  - Intercom systems
  - Emergency communication devices

### RTSP Protocol

Real-Time Streaming Protocol for audio and video streaming applications.
- **Readme:** [ESP_RTSP](docs/en/RTSP_README.md)
- **Headers:** [`esp_rtsp.h`](include/esp_rtsp.h)
- **Key Features:**
  - Multiple audio/video codecs
  - UDP/TCP transport
  - Server and client modes
  - Push and pull operations
  - Stream management (DESCRIBE, SETUP, PLAY, TEARDOWN)
- **Use Cases:**
  - IP cameras and surveillance systems
  - Video streaming servers
  - Media players and recorders
  - Remote monitoring applications

### RTMP Protocol

Real-Time Messaging Protocol for live streaming to platforms and custom servers.
- **Readme:** [ESP_RTMP](docs/en/RTMP_README.md)
- **Headers:** [`esp_rtmp_push.h`](include/esp_rtmp_push.h), [`esp_rtmp_server.h`](include/esp_rtmp_server.h), [`esp_rtmp_src.h`](include/esp_rtmp_src.h), [`esp_rtmp_types.h`](include/esp_rtmp_types.h)
- **Key Features:**
  - Multiple audio/video codecs
  - SSL/TLS encryption(RTMPS)
  - RTMP client (push streams)
  - RTMP server (receive and distribute streams)
  - RTMP source (pull streams)
- **Use Cases:**
  - Live streaming to YouTube/Twitch
  - Custom streaming platforms
  - IoT camera streaming
  - Content distribution networks

### MRM Protocol

Multi-Room Music Protocol for whole-home multi-device synchronized audio playback system.
- **Readme:** [ESP_MRM_Client](docs/en/MRM_README.md)
- **Headers:** [`esp_mrm_client.h`](include/esp_mrm_client.h)
- **Key Features:**
  - Multicast communication
  - Master-slave architecture
  - Dynamic role assignment
  - Network time synchronization
  - Automatic audio sync adjustment
- **Use Cases:**
  - Whole-home audio systems
  - Synchronized speaker groups
  - Commercial audio installations
  - Party mode applications

### UPnP Protocol

Universal Plug and Play with DLNA compliance for seamless media sharing and device discovery.
- **Readme:** [ESP_UPnP](docs/en/UPnP_README.md)
- **Headers:** [`esp_ssdp.h`](include/esp_ssdp.h), [`esp_upnp.h`](include/esp_upnp.h), [`esp_upnp_service.h`](include/esp_upnp_service.h), [`esp_upnp_notify.h`](include/esp_upnp_notify.h)
- **Key Features:**
  - HTTP server integration
  - XML-based service descriptions
  - Device discovery via SSDP
  - SOAP-based control actions
  - Event notifications
  - Media server/renderer/controller roles
- **Use Cases:**
  - Media servers and NAS devices
  - Smart TV and streaming devices
  - Home automation systems
  - Media center applications

## Protocol Comparison Matrix

| Protocol | Real-time | Streaming | Control | Discovery | SSL/TLS | Complexity |
|----------|-----------|-----------|---------|-----------|---------|------------|
| SIP      | High      | Yes       | Yes     | Manual    | Yes     | Medium     |
| RTSP     | High      | Yes       | Yes     | Manual    | No      | Medium     |
| RTMP     | Medium    | Yes       | Basic   | Manual    | Yes     | Medium     |
| MRM      | High      | Yes       | Yes     | Auto      | No      | Low        |
| UPnP     | Low       | Yes       | Yes     | Auto      | No      | Medium     |

# Supported chip

The following table shows the support of ESP_MEDIA_PROTOCOLS for Espressif SoCs. The "&#10004;" means supported, and the "&#10006;" means not supported.

|Chip         |      v0.5.0      |
|:-----------:|:----------------:|
|ESP32        |     &#10004;     |
|ESP32-S2     |     &#10004;     |
|ESP32-S3     |     &#10004;     |
|ESP32-C2     |     &#10004;     |
|ESP32-C3     |     &#10004;     |
|ESP32-C5     |     &#10004;     |
|ESP32-C6     |     &#10004;     |
|ESP32-P4     |     &#10004;     |
