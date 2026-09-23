# ESP Ambient LED

- [![Component Registry](https://components.espressif.com/components/espressif/esp_ambient_led/badge.svg)](https://components.espressif.com/components/espressif/esp_ambient_led)

[中文版本](./README_CN.md)

ESP Ambient LED maps camera or framebuffer content onto ambient LED strips. It samples color along the top, right, bottom, and left edges and returns one RGB value per color block.

The implementation builds a vertical partition from eight control points (four corners and four edge midpoints), optional black-border inward shift, and mean or dominant-color reduction.

## Features

- Eight-point geometry: corners and edge midpoints with configurable start corner and winding (CW / CCW)
- Per-edge ROI block counts (`top_bottom`, `right_left`) and per-edge shrink scale
- Dynamic inward ROI shift: brightness threshold (`roi_th`), near-gray R/G and B/G ranges, debounce, recover scan, and per-edge `start_index` / `shift_match_count`
- Optional `joint_shift`: after each edge's own inward shift, the neighbor inset also moves this edge's ROIs toward the center
- Pixel sample filter via `esp_ambient_led_sample_filter_cfg_t` for `cblk_samples`; black-border shift detection always uses the unfiltered mean
- Sample reduction: mean (`ESP_AMBIENT_LED_SAMPLE_MODE_MEAN`) or 4-bit RGB histogram dominant color (`ESP_AMBIENT_LED_SAMPLE_MODE_DOMINANT`)
- Input formats: RGB565 little-endian, RGB565 big-endian, RGB888, packed YUYV (YUV422, even width)
- Runtime APIs: `esp_ambient_led_cfg_get_defaults`, `init` / `deinit` / `process`

## Directory Structure

```text
esp_ambient_led/
├── include/
│   └── esp_ambient_led.h
└── lib/
    ├── idf_v5/               # Prebuilt for ESP-IDF 5.x
    │   ├── esp32s3/
    │   └── esp32p4/
    └── idf_v6/               # Prebuilt for ESP-IDF 6.x
        ├── esp32s3/
        ├── esp32p4/
        └── esp32s31/
```

## Quick Start

### Requirements

- Target chip with a matching prebuilt under `lib/idf_v<IDF_VERSION_MAJOR>/<target>/`

### Recommended input

1280×720 at 15–30 FPS, RGB565, RGB888, or packed YUYV (YUV422, even width). 1920×1080 is acceptable on ESP32-P4 when PSRAM bandwidth allows. `esp_ambient_led_cfg_get_defaults(w, h)` builds a full-frame default for that size (non-positive `w` or `h` falls back to 1280×720 RGB888).

### Calibration and test

1. Obtain eight control points (TL, T, TR, R, BR, B, BL, L) from a host calibration tool or a known geometry.
2. Start from `esp_ambient_led_cfg_get_defaults(w, h)`, override `ctrl_points` if the full-frame geometry is not the calibration, then `init` / `process` each frame.
3. Confirm sample count is `2 * top_bottom + 2 * right_left` and inspect `samples[i].rgb[0/1/2]`.

### Minimal Usage

```c
#include "esp_ambient_led.h"

esp_ambient_led_cfg_t cfg = esp_ambient_led_cfg_get_defaults(1280, 720);
/* Override cfg.pts.ctrl_points[] when not using the full-frame geometry */

esp_ambient_led_handle_t handle = NULL;
esp_ambient_led_err_t err = esp_ambient_led_init(&cfg, &handle);
if (err != ESP_AMBIENT_LED_OK) {
    return;
}

int n = 2 * cfg.roi_number.roi_number.top_bottom
      + 2 * cfg.roi_number.roi_number.right_left;
esp_ambient_led_sample_t *samples = calloc(n, sizeof(esp_ambient_led_sample_t));
err = esp_ambient_led_process(handle, frame_buf, samples);
/* samples[i].rgb[0/1/2] = R/G/B */

esp_ambient_led_deinit(handle);
free(samples);
```

## Notes

- Call `esp_ambient_led_deinit()` on every successful `esp_ambient_led_init()`
- `cblk_samples` length must be `2 * top_bottom + 2 * right_left`
- `src` format and size must match `cfg.image` from init
- Control points use `double` (sub-pixel arcs). ROI rectangles use integer pixels
- Per-edge scalars are `esp_ambient_led_edges_t` (a ratio or threshold per edge, not a pixel rectangle)
- `ctrl_points` length is `ESP_AMBIENT_LED_CTRL_POINT_COUNT` (four corners and four edge midpoints)
- `max_shift` per edge must be in `(0, 0.5]`

## SoC Compatibility

| Chip      | IDF 5.x | IDF 6.x |
|-----------|---------|---------|
| ESP32-S3  | Yes     | Yes     |
| ESP32-P4  | Yes     | Yes     |
| ESP32-S31 | —       | Yes     |

## FAQ

**Q: How many samples does `esp_ambient_led_process` write?**

A: `2 * roi_number.top_bottom + 2 * roi_number.right_left`. Output order follows `start_corner` and `winding` in `esp_ambient_led_roi_num_cfg_t`.

**Q: Does the sample filter affect black-border shift detection?**

A: No. Shift trigger uses the unfiltered ROI mean. The filter applies only when computing user `cblk_samples`.
