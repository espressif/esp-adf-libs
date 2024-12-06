# ESP_VIDEO_CODEC

`esp_video_codec` is an integrated module designed for video encoding and decoding on Espressif series platforms. It provides a unified API to interact with various video encoders and decoders, abstracting the underlying hardware or software implementation. This module simplifies video codec management and allows for flexible configuration and optimization.

## Key Features

- **Unified API**
  `esp_video_codec` provides a consistent interface to operate with different video encoders and decoders, both hardware and software-based, ensuring easy integration into applications.

- **Dynamic Codec Registration**
  Users can register new video encoders and decoders or overwrite existing ones using the provided API.

- **Optimized Library Size**
  With `menuconfig`, users can select the specific video codec libraries they need, reducing the linked library size of the application and speeding up the build process.

- **Capability Query**
  The module abstracts special requests of video encoders as `esp_video_enc_caps_t` and video decoders as `esp_video_dec_caps_t`. This helps users discover supported features and enables automatic negotiation. For example, if a camera supports RGB565, and the video encoder also supports RGB565, data can be directly fed from the camera to the video encoder without conversion.

- **Buffer Handling**
  Through the capability query, users can easily understand hardware-specific buffer alignment requirements or software-specific requests to speed up processing. The `esp_video_codec_align_alloc` API ensures that provided buffers meet these requirements, either for hardware compatibility or software optimization.

- **Advanced Features**
  Some video encoders and decoders support advanced features, which users can query from the capability information. Using these features can speed up processing by keeping functions centralized within the codec module rather than in separate modules. Advanced capabilities include:
  1. **Rotate**: Rotate video frames during encoding or decoding.
  2. **Resize**: Resize video frames on the fly.
  3. **Crop**: Crop video frames to a specified region of interest.

- **B-frame Handling**
  To process video with B-frame support, we added a `consumed` field in the input frame. This allows users to feed one input frame to get multiple output frames or input multiple frames to get a single output frame. Additionally, we support a unified flush behavior: users can set the `size` of an input frame to 0, prompting the encoder or decoder to flush as needed.

- **Cross-Platform Support**
  The user can register a PC codec and then debug the app code on the PC to accelerate development.

## Supported Video Encoders and Decoders

### Encoders

| Chip         | Hardware H.264   | Software H.264   | Hardware JPEG    | Software JPEG    |
|:------------:|:----------------:|:----------------:|:----------------:|:----------------:|
| ESP32-S2     |      ❌          |       ❌         |       ❌         |       ✅         |
| ESP32-S3     |      ❌          |       ✅         |       ❌         |       ✅         |
| ESP32-P4     |      ✅          |       ✅         |       ✅         |       ✅         |

### Decoders

| Chip         | Software H.264   | Hardware JPEG    | Software JPEG    |
|:------------:|:----------------:|:----------------:|:----------------:|
| ESP32-S2     |       ❌         |       ❌         |       ✅         |
| ESP32-S3     |       ✅         |       ❌         |       ✅         |
| ESP32-P4     |       ✅         |       ✅         |       ✅         |


# Usage

For a quick introduction to how to use the codec functions, you can refer to the sample test application:

- **Sample Test File**: [video_codec_test.c](test_apps/video_codec_test/main/video_codec_test.c)

This file includes both encoder and decoder test cases with basic usage examples to help you understand how the video codec API is intended to be used.

## Video Encoder Test

- **Function**: `single_mjpeg_encode_test`

This function demonstrates how to perform single-frame MJPEG encoding. It will help you understand the typical flow of encoding a raw image or video frame into an MJPEG compressed stream.

## Video Decoder Test

- **Function**: `single_mjpeg_decode_test`

This function demonstrates how to decode an MJPEG compressed frame back into raw video frame. It will help you understand the typical flow of decoding video packet into a raw image.
