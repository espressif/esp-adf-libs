# LVGL EFC 播放器

- [English Version](./README.md)
- 例程难度：⭐⭐

## 例程简介

本例程演示如何用 `esp_efc_codec` 播放表情动画。它列出 `/sdcard/totest` 中所有扩展名以 `.efc` 开头的片段（普通 `.efc` 和 zlib 的 `.efc_z`），自动播放第一个，并可以在界面上切换、重头播放、暂停和继续。画面会把片段缩放到区域内。下方标签显示解码和屏幕实际达到的帧率，播放器不会去追一个目标帧率。

`lv_efc` 是持有这段动画的 LVGL 图像控件。它把每一帧原地解码成 RGB565；文件带色键时再附上一层 alpha。

### 资源列表

- LVGL 任务栈：20 KB，每一帧在这里解码
- 主任务栈：12 KB，用于板级初始化
- 片段和帧缓冲放在 PSRAM


### 预备知识

- 已安装可用的 [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.5/esp32s3/get-started/index.html)，并设置了 `IDF_PATH`
- 片段已经编码成 EFC。见 [将 GIF 转换为 EFC](#将-gif-转换为-efc)。把 `.efc` 文件拷到卡上的 `totest` 目录。播放器打开的路径是 `/sdcard/totest`。


### 文件结构

```
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml       Board manager、触摸和 LVGL 适配器
│   └── efc_player_main.c       文件列表、按键和 lv_efc 画面
├── CMakeLists.txt              工程文件；codec 组件在上两级目录
├── partitions.csv              可选的 6 MB factory 分区
├── sdkconfig.defaults          打开控件、长文件名、12 KB 主任务栈
├── sdkconfig.defaults.esp32s3  240 MHz 和八线 PSRAM
├── sdkconfig.defaults.esp32p4  改目标芯片时的 PSRAM 默认值
└── README.md                   英文说明
```


## 将 GIF 转换为 EFC

面向最终用户的编码器是 `pc/js` 里的 JavaScript 版本，可以在浏览器里运行，也可以用 Node.js。两种方式都会得到本播放器能打开的 `.efc`。

**Tile 保持 4。** 本例程按固定 4×4 块播放。zlib 默认开启，并写在文件内部，所以下载的文件可以叫 `smile.efc`，也可以叫 `smile.efc_z`。两种都会出现在屏幕上的列表里。把文件拷进卡上的 `totest`。

### 在浏览器中

用浏览器打开 `pc/js/dist/encode.html`。双击该文件，或把路径交给浏览器即可。页面已包含所有依赖，不需要网络连接或本地服务器。

1. 选择一个 GIF。页面会显示它并循环预览。
2. **Tile** 保持 `4 (device decoder)`，**zlib** 保持勾选。
3. 点击 **Encode**。
4. 点击 **Download**。文件名是 GIF 的名字加上 `.efc`。

编码在浏览器本地完成。状态行会显示 GIF 大小、`.efc` 大小和编码耗时。

### 使用 Node.js

需要 Node.js 18 或更高版本：

```bash
cd pc/js/dist
node encode_gif.mjs smile.gif smile.efc
```

发布脚本已包含所有依赖，不需要执行 `npm install`。脚本会打印尺寸、帧数，以及 GIF 和 `.efc` 的字节大小。

传入 `raw` 可关闭 zlib：

```bash
node encode_gif.mjs smile.gif smile.efc raw
```

一次转换多个 GIF：

```bash
mkdir -p out
for g in *.gif; do
  node encode_gif.mjs "$g" "out/${g%.gif}.efc"
done
```

把 `out/` 拷到卡上，目录名为 `totest`。


## 环境配置

### 硬件要求

- `esp_board_manager` 支持的、带显示屏和 microSD 卡槽的开发板都可以。本文以 ESP32-S3-Korvo-2 v3 为例，板名是 `esp32_s3_korvo_2_3`。`gen-bmgr-config -b <board>` 会拉起该板的 LCD、触摸、背光和 SD 卡槽
- 一张 FAT 格式的 microSD 卡，片段放在 `totest` 里

### 其他要求

- 该目录里至少有一个 `.efc` 或 `.efc_z`。空目录也能启动，列表会显示 `no .efc* files`
- `CONFIG_ESP_EFC_LVGL_WIDGET` 必须保持开启。它负责编译 `lv_efc` 并引入 LVGL 9。`sdkconfig` 和 `sdkconfig.defaults` 里已经打开

## 编译和下载

### 默认 IDF 分支

本例程支持 ESP-IDF release/v5.5 及以后版本。默认在 ESP-IDF release/v5.5 上运行。

### 配置

先为所用开发板生成 board manager 配置，再按需要调整：

```text
ESP-EFC codec > LVGL EFC player widget (lv_efc)
Component config > FAT Filesystem support > Long filename support
```

必须打开控件选项。`sdkconfig.defaults` 里已经打开，同时包含长文件名、PSRAM 和 12 KB 主任务栈。片段文件名通常超过 8.3 限制，所以需要长文件名。240 MHz 主频和八线 PSRAM 来自 `sdkconfig.defaults.esp32s3`。

板名从 `esp_board_manager` 里选。下面的例子是 `esp32_s3_korvo_2_3`。`idf.py gen-bmgr-config -b <board>` 会把该板的 LCD、触摸、背光和 SD 卡设备写到 `components/gen_bmgr_codes`。

默认分区表是 single-app。如果链接阶段提示应用程序镜像放不下，改用本目录里的自定义分区表：

```text
Partition Table > Custom partition table CSV > partitions.csv
```

`partitions.csv` 给 factory 应用 6 MB，适合这块 16 MB Flash。

### 编译和烧录

```bash
idf.py set-target esp32s3
idf.py gen-bmgr-config -b esp32_s3_korvo_2_3
idf.py build flash monitor
```

换一块板时，把板名传给 `-b`，并在生成配置之前把 target 设成与该板一致。

配置和编译 ESP-IDF 工程的完整步骤见 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/latest/esp32/get-started/index.html)。

## 如何使用例程

### 功能和用法

启动后，左边一列是文件列表，右边是正在播放的片段。第一个文件会自己开始。

- **点一个名字**：加载该片段，从第 0 帧播放
- **Play**：当前片段从第 0 帧重新开始并继续
- **Pause**：停在屏幕上的这一帧
- **Resume**：从这一帧继续
- 标签是 `名字` 加上最近半秒测到的 FPS。播放器不会按目标帧率限速，这个数字就是解码和屏幕能跑到的速度

片段会循环，直到另选一个文件或按下 Pause。

### 参考

- 这个界面基于 `include/lv_efc.h` 里的 `lv_efc_create()` 和 `lv_efc_set_src()`。`lv_efc_set_src()` 接受路径，例如 `/sdcard/totest/smile.efc`
- 在浏览器或 Node.js 里把 GIF 转成 `.efc`，见 [将 GIF 转换为 EFC](#将-gif-转换为-efc)


## 故障排除

- **`cannot open /sdcard/totest`。** 卡没有挂载，或没有 `totest` 目录。卡槽是 `esp_board_manager` 为该板选中的那个，文件要放在这个目录里，不要放在卡的根目录
- **`no .efc* files`。** 目录里没有扩展名以 `.efc` 开头的文件。`.gif` 不会出现在列表里
- **`file too large or missing`。** 该片段超过 5 MB。重新编码，或把源文件拆开
- **`open failed`。** 文件不是可播放的 EFC 片段，或编译控件时没有打开 `CONFIG_ESP_EFC_LVGL_WIDGET`
- **画面不动，FPS 一直是 0。** Pause 还按着，或者第一个半秒还没到。按 Resume，等下一次刷新
- **点了没反应。** 触摸初始化打出了警告。播放第一个片段不依赖触摸


## 技术支持与反馈

请通过以下渠道反馈：

- 技术问题请到 [esp32.com](https://esp32.com/viewforum.php?f=20) 论坛
- 功能请求或缺陷请在 `esp_efc_codec` 项目中提交 issue

我们会尽快回复。
