# ESP Ambient LED

- [![Component Registry](https://components.espressif.com/components/espressif/esp_ambient_led/badge.svg)](https://components.espressif.com/components/espressif/esp_ambient_led)

[English](./README.md)

ESP Ambient LED 把相机或帧缓冲内容映射到氛围灯灯带：沿上、右、下、左四边采样颜色，每个色块输出一组 RGB。

实现上根据八个控制点（四角与四边中点）做竖直分区，可选黑边向内平移，以及均值或主色归约。

## 功能特性

- 八点几何：四角与四边中点，可配置起始角与绕向（顺时针 / 逆时针）
- 按边配置 ROI 色块数量（`top_bottom`、`right_left`）与按边收缩比例
- 动态向内 ROI 平移：亮度阈值、近灰 R/G 与 B/G 区间、防抖、恢复扫描，以及按边 `start_index` / `shift_match_count`
- 可选 `joint_shift`：邻边内沿再把本边 ROI 向画面中心推移
- `esp_ambient_led_sample_filter_cfg_t` 只作用于用户 `cblk_samples`；黑边检测始终用未过滤均值
- 归约：均值或 4-bit RGB 直方图主色
- 输入：RGB565 小端 / 大端、RGB888、packed YUYV（YUV422，宽度须为偶数）
- 运行时 API：`esp_ambient_led_cfg_get_defaults`、`init` / `deinit` / `process` 等

## 目录结构

```text
esp_ambient_led/
├── include/
│   └── esp_ambient_led.h
└── lib/
    ├── idf_v5/               # ESP-IDF 5.x 预编译库
    │   ├── esp32s3/
    │   └── esp32p4/
    └── idf_v6/               # ESP-IDF 6.x 预编译库
        ├── esp32s3/
        ├── esp32p4/
        └── esp32s31/
```

## 快速开始

### 环境要求

- 目标芯片在 `lib/idf_v<IDF_VERSION_MAJOR>/<target>/` 下有对应预编译库

### 推荐输入

1280×720、15–30 FPS，RGB565、RGB888 或 packed YUYV（YUV422，宽度须为偶数）。ESP32-P4 在 PSRAM 带宽允许时可用 1920×1080。`esp_ambient_led_cfg_get_defaults(w, h)` 按该尺寸生成满幅默认配置；`w` 或 `h` 非正时回退为 1280×720 RGB888。

### 标定与测试

1. 取得八点控制点（左上、上中、右上、右中、右下、下中、左下、左中）。
2. 调用 `esp_ambient_led_cfg_get_defaults(w, h)`。满幅几何不适用时再覆盖 `ctrl_points`，然后 `init` / `process`。
3. 采样个数为 `2 * top_bottom + 2 * right_left`，颜色在 `samples[i].rgb[0/1/2]`。

### 最简用法

```c
#include "esp_ambient_led.h"

esp_ambient_led_cfg_t cfg = esp_ambient_led_cfg_get_defaults(1280, 720);
/* 非满幅几何时覆盖 cfg.pts.ctrl_points[] */

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

## 注意事项

- 每次成功的 `esp_ambient_led_init()` 都需调用 `esp_ambient_led_deinit()`
- `cblk_samples` 长度须为 `2 * top_bottom + 2 * right_left`
- `src` 的格式与尺寸须与 init 时的 `cfg.image` 一致
- 控制点使用 `double`（亚像素弧）；ROI 矩形使用整数像素
- 四边标量类型为 `esp_ambient_led_edges_t`，表示各边比例或阈值，不是像素矩形
- `ctrl_points` 长度为 `ESP_AMBIENT_LED_CTRL_POINT_COUNT`（四角加四边中点）
- 各边 `max_shift` 须落在 `(0, 0.5]`

## SoC 兼容性

| 芯片      | IDF 5.x | IDF 6.x |
|-----------|---------|---------|
| ESP32-S3  | 支持    | 支持    |
| ESP32-P4  | 支持    | 支持    |
| ESP32-S31 | —       | 支持    |

## FAQ

**Q：`esp_ambient_led_process` 会写入多少个采样？**

A：`2 * roi_number.top_bottom + 2 * roi_number.right_left`。输出顺序由 `start_corner` 与 `winding` 决定。

**Q：采样过滤是否影响黑边平移检测？**

A：不影响。平移触发使用未过滤的 ROI 均值。
