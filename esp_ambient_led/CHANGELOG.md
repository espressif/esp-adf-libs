# Changelog

## v1.0.0

### Features

- Initial version of ESP Ambient LED
- Sample camera or framebuffer edges into per-block RGB for ambient LED strips
- Eight-point partition with optional inward ROI shift and mean or dominant-color reduction
- Packed YUYV (YUV422) input format `ESP_AMBIENT_LED_FORMAT_YUYV`
- `esp_ambient_led_cfg_get_defaults(w, h)` returns `esp_ambient_led_cfg_t` by value
- Control-point count is `ESP_AMBIENT_LED_CTRL_POINT_COUNT`
