# ESP_H264

ESP_H264 is Espressif's lightweight H264 encoder and decoder. It comes from v2.2.0 of [openh264](https://github.com/cisco/openh264), and the memory and CPU loading are optimized to make better use of Espressif chips.

# Features

Encoder:   
    - Support baseline profile (max frame size is 36864 macro-block)      
    - Support a variety of widths and heights     
    - Support quality first rate control
    - Support YUYV and IYUV raw data   
    - Support dynamic changes about bit rate, frame rate   
    - Support more than one slice per frame   
    - Support SPS and PPS encoding  

# Performance

## Test on chip ESP32-S3R8

| Resolution |     Raw Format          | Memory (Byte) | Frame Per Second(fps) |
| --         | --                      |--             | --                    |
| 320 * 192  | ESP_H264_RAW_FMT_I420   |1 M            | 17.48                 |
| 320 * 240  | ESP_H264_RAW_FMT_YUV422 |1 M            | 10.86                 |

# Supported chip

| ESP_H264 Version     | Supported Chip |
| --                   | --             |
| v0.1.0               | ESP32-S3       |


# Usage

Example of function call.
```c
 void app_main(void)
 {
     int one_image_size = 0;
     esp_h264_err_t ret = ESP_H264_ERR_OK;
     esp_h264_enc_t handle = NULL;
     esp_h264_enc_cfg_t cfg = DEFAULT_H264_ENCODER_CONFIG();
     esp_h264_enc_frame_t out_frame = { 0 };
     esp_h264_raw_frame_t in_frame = { 0 };
     int frame_count = 0;
     int ret_w = 0;
     FILE *out = fopen("/sdcard/h264/t160_96.h264", "wb");
     if (out == NULL) {
         printf("Output file cann't open \r\n");
         return;
     }
     FILE *in = fopen("/sdcard/h264/160x96.yuv", "rb");
     if (in == NULL) {
         printf("Input file cann't open \r\n");
         goto h264_example_exit;
     }
     cfg.fps = 30;
     cfg.height = 96;
     cfg.width = 160;
     cfg.pic_type = ESP_H264_RAW_FMT_I420;
     one_image_size = cfg.height * cfg.width * 1.5; // 1.5 : Pixel is 1.5 on ESP_H264_RAW_FMT_I420.
     in_frame.raw_data.buffer = (uint8_t *)heap_caps_aligned_alloc(16, one_image_size, MALLOC_CAP_8BIT | MALLOC_CAP_SPIRAM);
     if (in_frame.raw_data.buffer == NULL) {
         printf("Allcation memory failed \r\n");
         goto h264_example_exit;
     }
     ret = esp_h264_enc_open(&cfg, &handle);
     if (ret != ESP_H264_ERR_OK) {
         printf("Open failed. ret %d, handle %p \r\n", ret, handle);
         goto h264_example_exit;
     }
     while(1) {
         ret_w = fread(in_frame.raw_data.buffer, 1, one_image_size, in);
         if (ret_w != one_image_size) {
             printf("Encoder finished, ret %d \r\n", ret_w);
             goto h264_example_exit;
         }
         in_frame.pts = frame_count * (1000 / cfg.fps);
         ret = esp_h264_enc_process(handle, &in_frame, &out_frame);
         if (ret != ESP_H264_ERR_OK) {
             printf("Process failed. ret %d \r\n", ret);
             goto h264_example_exit;
         }
         for (size_t layer = 0; layer < out_frame.layer_num; layer++) {
             ret_w = fwrite(out_frame.layer_data[layer].buffer, 1, out_frame.layer_data[layer].len, out);
             if (ret_w != out_frame.layer_data[layer].len) {
                 printf("fwrite happened error, ret %d \r\n", ret_w);
                 goto h264_example_exit;
             }
         }
         frame_count++;
     }
 h264_example_exit:
     if (in) {
         fclose(in);
     }
     if (out) {
         fclose(out);
     }
     if (in_frame.raw_data.buffer) {
         heap_caps_free(in_frame.raw_data.buffer);
         in_frame.raw_data.buffer = NULL;
     }
     esp_h264_enc_close(handle);
     return;
 }
```
# Change log

## Version 0.1.0
- Added H264 encoder
