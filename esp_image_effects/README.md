# ESP_IMAGE_EFFECTS

ESP Image Effects (ESP_IMGFX) is an image processing engine that integrates basic functions such as rotation, color space conversion, scaling, cropping, and more. It serves as one of the core components of Espressif's audio and video development platform. The ESP Image Effects module undergoes deep reconstruction of underlying algorithms, combined with efficient memory management and hardware acceleration, achieving the triple characteristics of high performance, low power consumption, and low memory usage. Additionally, each image processing function adopts a consistent API architecture design, reducing the learning cost for users and facilitating rapid development. This engine is widely applicable in various fields, including the Internet of Things, smart cameras, industrial vision, and more.

# FEATURES

## IMAGE COLOR CONVERT

   - Supported arbitrary input resolutions
   - Supported bypass mode for identical input/output formats
   - Supports BT.601/BT.709/BT.2020 color space standards
   - Supported faster color convert algorithm for formats and resolutions
   - Comprehensive format support matrix:
     | Input Format               | Supported Output Formats                                  |
     |----------------------------|-----------------------------------------------------------|
     | RGB/BGR565_LE/BE  RGB/BGR888 | RGB565_LE/BGR/RGB565_LE/BE RGB/BGR888 YUV_PLANAR/PACKET YUYV/UYVY  O_UYY_E_VYY/I420   |                                               
     | ARGB/BGR888                  | RGB565_LE/BGR/RGB565_LE/BE RGB/BGR888 YUV_PLANAR O_UYY_E_VYY/I420    |
     | YUV_PACKET/UYVY/YUYV         | RGB565_LE/BGR/RGB565_LE/BE RGB/BGR888 O_UYY_E_VYY/I420     |
     | O_UYY_E_VYY/I420             | RGB565_LE/BGR/RGB565_LE/BE RGB/BGR888 O_UYY_E_VYY     |

## IMAGE ROTATE

- Supported bypass
- Supported arbitrary input resolutions
- Supported clockwise rotation at any degree
- Supported ESP_IMG_PIXEL_FMT_Y/RGB565/BGR565/RGB888/BGR888/YUV_PACKET
- Supported faster clockwise rotation algorithm for specific degrees formats resolutions

## IMAGE SCALE

- Supported bypass
- Supported arbitrary input resolutions
- Supported both upscaling and downscaling operations
- Supported ESP_IMG_PIXEL_FMT_RGB565/BGR565/RGB888/BGR888/YUV_PACKET
- Supported multiple filter algorithms: optimized downsampling and bilinear interpolation

## IMAGE CROP

- Supported bypass
- Supported arbitrary input resolutions
- Supported flexible region selection
- Supported ESP_IMG_PIXEL_FMT_Y/RGB565/BGR565/RGB888/BGR888/YUV_PACKET

# PERFORMANCE

The performance on esp32p4 :`./doc/PERFORMANCE_ESP32P4.md`

# USAGE
For more details on API usage, please refer to the source files in `test_app/main`

# SUPPORTED CHIP
| chip     | C code  | ASM code |
| -------- | --------| ---------|
| ESP32    | √ |  x  |
| ESP32-S2 | √ |  x  |
| ESP32-S3 | √ |  x  |
| ESP32-C3 | √ |  x  |
| ESP32-C5 | √ |  x  |
| ESP32-C6 | √ |  x  |
| ESP32-P4 | √ |  √  |

# FAQ

---
### 1. Why is image data size alignment and address alignment recommended?
   - Image data size alignment and address alignment are recommended for hardware compatibility and performance optimization.
   - Alignment improves cache efficiency, reducing memory access latency and enhancing overall performance.
   - SIMD instruction sets, such as those requiring data size alignment to 16 bytes, benefit from alignment.
---

### 2. Why Concurrent Access to `esp_imgfx_*_set_cfg` and `esp_imgfx_*_process` is Prohibited？
1. **Reason**
   - Modifying configuration parameters with `esp_imgfx_*_set_cfg`  directly impacts `esp_imgfx_*_process`, making concurrent access unsafe in multi-threaded environments.
2. **Solution**
   - Ensure atomicity between configuration updates and processing in multi-threaded scenarios by using mutexes or atomic operations.
---

### 3. Reasons for `esp_imgfx_*_process` Returning `ESP_IMGFX_ERR_DATA_LACK`
1. **Reason**
   - Input image buffer size is smaller than configured requirements, causing data lack errors (e.g., mismatch between configured 1920x1080 and input buffer size of 1920x720).
2. **Solution**
   - Obtain required input dimensions with `esp_imgfx_get_image_size` and allocate sufficient data buffers accordingly.

---

### 4. Reasons for `esp_imgfx_*_process` Returning `ESP_IMGFX_ERR_BUFF_NOT_ENOUGH`
1. **Reason**
   - Output image buffer size is smaller than configured requirements, leading to buffer not enough errors (e.g., configured for 1920x1080 but output buffer allocated for 1920x720).
2. **Solution**
   - Pre-calculate output dimensions with `esp_imgfx_get_image_size` to match requirements and adjust buffer sizes accordingly.

---

### 5. Can the configuration parameters be reset?
   Yes, users can reset the configuration parameters by calling esp_imgfx_*_set_cfg. After that, esp_imgfx_*_process should be executed with the updated configuration.

---

### 6. What's reason of the build error `If use esp32p4, please use idf v5.3.`?
1. **Reason**
   "Currently, compilers above version 5.3 have a compatibility issue, so P4 must be compiled with version 5.3. This will be fixed in a future update."
2. **Solution**
   - Use idf5.3
---
