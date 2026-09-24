ESP Ambient LED
=======

简介
----------------

ESP Ambient LED 把相机或帧缓冲内容映射到氛围灯灯带：沿上、右、下、左四边采样颜色，每个色块输出一组 RGB。实现上根据八个控制点（四角与四边中点）做竖直分区，可选黑边向内平移，以及均值或主色归约。

功能清单
----------------------

- 八点几何：四角与四边中点，可配置起始角与绕向（顺时针 / 逆时针）
- 按边配置 ROI 色块数量（ ``top_bottom`` 、 ``right_left`` ）与按边收缩比例
- 动态向内 ROI 平移：亮度阈值（ ``roi_th`` ）、近灰 R/G 与 B/G 区间（ ``rg_ratio_min`` / ``rg_ratio_max`` 、 ``bg_ratio_min`` / ``bg_ratio_max`` ）、防抖（ ``delay_frames`` ）、恢复扫描（ ``recover_frames`` ），以及按边 ``start_index`` / ``shift_match_count``
- 可选 ``joint_shift`` ：各边完成本边向内平移后，邻边内沿再把本边 ROI 向画面中心推移
- 通过 ``esp_ambient_led_sample_filter_cfg_t`` 过滤 ``cblk_samples`` 像素（各通道 ``r/g/b_min`` / ``max`` 、反向 R/G × B/G 剔除框、 ``mode`` 、 ``min_ok_percent`` ）；黑边平移检测始终使用未过滤均值
- 采样归约：均值（ ``ESP_AMBIENT_LED_SAMPLE_MODE_MEAN`` ）或 4-bit RGB 直方图主色（ ``ESP_AMBIENT_LED_SAMPLE_MODE_DOMINANT`` ）
- 输入格式：RGB565 小端、RGB565 大端、RGB888、packed YUYV（YUV422，宽度须为偶数）（ ``esp_ambient_led_format_t`` ）
- 运行时 API： ``esp_ambient_led_cfg_get_defaults`` 、 ``init`` / ``deinit`` / ``process``

技术拆解
----------------------

处理流程
^^^^^^^^^^^^^^^^^^^^^^

调用方提供八点标定与图像参数，组件在初始化阶段建立弧线、网格、ROI 缓存与平移表；每帧调用处理接口，按当前平移档位对 ROI 取色，写出色块 RGB 采样，并按近灰检测结果更新平移档位。用户侧 ``cblk_samples`` 经过像素过滤与归约；平移触发始终使用未过滤均值。

.. only:: html

   .. mermaid::

      flowchart LR
          A["八点标定 + 图像配置"] --> B["partition init"]
          B --> C["ROI / 平移表"]
          D["帧缓冲 src"] --> E["partition process"]
          C --> E
          E --> F["cblk_samples RGB"]
          E --> G["更新 shift 档位"]

生命周期
^^^^^^^^^^^^^^^^^^^^^^

``esp_ambient_led_init`` 校验配置后分配句柄； ``esp_ambient_led_process`` 在句柄上逐帧采样； ``esp_ambient_led_deinit`` 释放资源。 ``handle`` 为 ``NULL`` 时 ``deinit`` 为空操作。

返回码 ``esp_ambient_led_err_t`` ：

- ``ESP_AMBIENT_LED_OK`` ：成功
- ``ESP_AMBIENT_LED_ERR_INVALID_ARG`` ：空指针或参数越界
- ``ESP_AMBIENT_LED_ERR_NO_MEM`` ：内存分配失败
- ``ESP_AMBIENT_LED_ERR_DEGENERATE`` ：八点几何退化（共线或近零面积）

.. code:: c

    esp_ambient_led_handle_t handle = NULL;
    esp_ambient_led_err_t err = esp_ambient_led_init(&cfg, &handle);
    if (err != ESP_AMBIENT_LED_OK) {
        return;
    }
    err = esp_ambient_led_process(handle, frame_buf, samples);
    esp_ambient_led_deinit(handle);

八点几何与绕向
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``esp_ambient_led_pts_cfg_t`` 持有 ``ctrl_points[ESP_AMBIENT_LED_CTRL_POINT_COUNT]`` 、起始角 ``start_corner`` 与绕向 ``winding`` 。重排后索引约定：0-1-2 为上边，2-3-4 为右边，4-5-6 为下边，6-7-0 为左边，角点位于 0/2/4/6。

- ``ESP_AMBIENT_LED_CORNER_TL`` / ``TR`` / ``BR`` / ``BL`` ：指定哪一角映射为规范 ``ctrl_points[0]``
- ``ESP_AMBIENT_LED_WIND_CW`` ：屏幕顺时针，从上左起为上→右→下→左
- ``ESP_AMBIENT_LED_WIND_CCW`` ：屏幕逆时针，从上左起为上→左→下→右

默认起始角为 ``ESP_AMBIENT_LED_CORNER_TL`` ，默认绕向为 ``ESP_AMBIENT_LED_WIND_CW`` 。

ROI 布局与输出顺序
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

内部几何始终按上→右→下→左组织； ``esp_ambient_led_roi_num_cfg_t`` 中的 ``start_corner`` 与 ``winding`` 只重映射 ``cblk_samples[]`` 的输出下标顺序。

- ``roi_number.top_bottom`` ：上下边色块数量，有效范围 2 到 255
- ``roi_number.right_left`` ：左右边色块数量
- ``scale`` ：各边 ROI 收缩比例（ ``esp_ambient_led_edges_t`` ）

调用方分配的 ``cblk_samples`` 长度须为 ``2 * top_bottom + 2 * right_left`` 。

动态向内平移
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``esp_ambient_led_shift_cfg_t`` 控制黑边检测与 ROI 向内平移。当 ROI 均值满足近灰条件时，组件在防抖帧数满足后提高平移档位； ``recover_frames`` 控制完整恢复扫描间隔，为 0 时各边保持在 ``start_index`` 。

``joint_shift`` 非 0 时，各边先按本边检测结果向内平移，随后邻边内沿再把本边各 ROI 向画面中心推移。

主要字段：

- ``max_shift`` ：各边最大平移比例，须落在 ``(0, 0.5]``
- ``shift_step`` ：各边每档步进比例
- ``roi_th`` ：各边亮度阈值
- ``delay_frames`` / ``recover_frames`` ：防抖与恢复扫描
- ``rg_ratio_min`` / ``rg_ratio_max`` 、 ``bg_ratio_min`` / ``bg_ratio_max`` ：近灰 R/G、B/G 条件（落在区间内视为近灰）
- ``shift_match_count[4]`` ：各边最少匹配色块数，0 表示该边全部色块
- ``start_index[4]`` ：各边初始档位，恢复时也回到该值；init 时钳位到 ``[0, max_shift_number]``
- ``joint_shift`` ：非 0 时启用邻边内沿对本边 ROI 的向内推移

采样过滤与归约
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

``esp_ambient_led_sample_filter_cfg_t`` 仅作用于用户侧 ``cblk_samples`` 。通道 min/max 采用“任一通道在范围内即保留”：仅当 R、G、B 同时越出各自已启用边界时才剔除该像素。R/G × B/G 为反向剔除框：两个比例同时落在 ``rg_ratio_*`` 与 ``bg_ratio_*`` 区间内则丢掉该像素；四个比例字段均为 0 时关闭比例过滤。任一 max 字段为 0 表示关闭该上限；min 为 0 表示无下限。

通过过滤的像素再按 ``mode`` 归约为一个 RGB 采样：

- ``ESP_AMBIENT_LED_SAMPLE_MODE_MEAN`` ：算术均值
- ``ESP_AMBIENT_LED_SAMPLE_MODE_DOMINANT`` ：4-bit RGB 直方图主色，再对该桶取均值

``min_ok_percent`` 为 ROI 内通过过滤的像素最低占比（1..100）。为 0 时只要有像素通过即采用过滤结果；占比不足时该 ROI 回退未过滤均值。黑边平移检测始终使用未过滤均值。

.. code:: c

    cfg.sample_filter.r_min = 0;
    cfg.sample_filter.r_max = 0;
    cfg.sample_filter.rg_ratio_min = 0.0f;
    cfg.sample_filter.rg_ratio_max = 0.2f;
    cfg.sample_filter.bg_ratio_min = 0.0f;
    cfg.sample_filter.bg_ratio_max = 0.2f;
    cfg.sample_filter.mode = ESP_AMBIENT_LED_SAMPLE_MODE_MEAN;
    cfg.sample_filter.min_ok_percent = 0;

图像输入
^^^^^^^^^^^^^^^^^^^^^^

``esp_ambient_led_image_cfg_t`` 指定宽、高与像素格式：

- ``ESP_AMBIENT_LED_FORMAT_RGB565_LE``
- ``ESP_AMBIENT_LED_FORMAT_RGB565_BE``
- ``ESP_AMBIENT_LED_FORMAT_RGB888``
- ``ESP_AMBIENT_LED_FORMAT_YUYV`` （packed YUV422，行内 ``Y U Y V``，宽度须为偶数；内部转为 RGB）

``esp_ambient_led_process`` 的 ``src`` 须与 init 时的图像配置一致。

快速开始
----------------------

目标芯片须在 ``lib/idf_v5`` / ``lib/idf_v6`` 下有对应预编译库。推荐输入分辨率 1280×720、15–30 FPS。先 ``esp_ambient_led_cfg_get_defaults(w, h)`` ，再按需要覆盖 ``ctrl_points`` 。

.. code:: c

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

注意事项
----------------------

- 每次成功的 ``esp_ambient_led_init`` 都需调用 ``esp_ambient_led_deinit``
- ``cblk_samples`` 长度须为 ``2 * top_bottom + 2 * right_left``
- ``src`` 的格式与尺寸须与 init 时的 ``cfg.image`` 一致
- 近灰平移触发使用 ``R/G`` 、 ``B/G`` 区间；min 为 ``0`` 表示无下限
- 采样过滤的通道 min/max：任一已启用通道仍在范围内则保留该像素；仅当 R、G、B 同时越界才剔除
- 采样过滤的 R/G × B/G 为剔除框：两个比例同时落在区间内则丢掉该像素；四个比例字段均为 ``0`` 时关闭比例过滤
- ``esp_ambient_led_sample_filter_cfg_t`` 中 max 为 ``0`` 表示关闭该上限； ``min_ok_percent`` 为 ``0`` 表示只要有像素通过即采用过滤结果， ``1..100`` 表示通过占比不足时回退未过滤均值
- 各边 ``max_shift`` 须落在 ``(0, 0.5]``
- ``esp_ambient_led_cfg_get_defaults(w, h)`` 的 ``w`` 或 ``h`` 非正时，图像尺寸回退为 1280×720

性能
----------------

应用示例
----------------------

先调用 ``esp_ambient_led_cfg_get_defaults(w, h)`` ，再按需要覆盖 ``ctrl_points`` 。推荐输入为 1280×720、15–30 FPS（RGB565、RGB888 或 packed YUYV）。

SoC 兼容性
-------------------------------

.. list-table::
   :header-rows: 1

   * - 芯片
     - IDF 5.x
     - IDF 6.x
   * - ESP32-S3
     - 支持
     - 支持
   * - ESP32-P4
     - 支持
     - 支持
   * - ESP32-S31
     - —
     - 支持

FAQ
---

**Q：** ``esp_ambient_led_process`` **会写入多少个采样？**

A： ``2 * roi_number.top_bottom + 2 * roi_number.right_left`` 。输出顺序由 ``esp_ambient_led_roi_num_cfg_t`` 中的 ``start_corner`` 与 ``winding`` 决定。

**Q：采样过滤是否影响黑边平移检测？**

A：不影响。平移触发使用未过滤的 ROI 均值。过滤仅作用于用户侧 ``cblk_samples`` 。

**Q：** ``rg_ratio_min`` / ``rg_ratio_max`` **控制什么？**

A：在 ``esp_ambient_led_shift_cfg_t`` 中约束近灰 R/G（以及对应的 B/G）平移条件；在 ``esp_ambient_led_sample_filter_cfg_t`` 中构成反向剔除框：R/G 与 B/G 同时落在区间内的像素不参与归约。平移 max 默认值为 ``ESP_AMBIENT_LED_DEFAULT_RG_RATIO_MAX`` / ``ESP_AMBIENT_LED_DEFAULT_BG_RATIO_MAX`` （ ``0.2f`` ）。

**Q：** ``recover_frames == 0`` **表示什么？**

A：各边平移保持在 ``start_index`` ；不进行完整恢复扫描。

**Q：MEAN 与 DOMINANT 有何区别？**

A： ``ESP_AMBIENT_LED_SAMPLE_MODE_MEAN`` 对通过过滤的像素取算术均值。 ``ESP_AMBIENT_LED_SAMPLE_MODE_DOMINANT`` 在这些像素上取 4-bit RGB 直方图主色，再对该桶取均值。

**Q：** ``min_ok_percent`` **做什么？**

A：表示 ROI 内须通过过滤的像素最低占比（ ``1..100`` ）。 ``0`` 表示只要有像素通过即采用过滤结果。占比不足时该 ROI 使用未过滤均值。

API 参考
----------------------------

公开头文件为 ``include/esp_ambient_led.h`` ，包含类型、配置结构、错误码与分区生命周期接口。将本组件接入 esp-docs 工程后，可用 Doxygen / Breathe 从该头文件生成 API 页面。
