# MIDI 播放 example

- [English](./README.md)

## 例程简介

本例程演示了如何解码播放 SD 卡上存储的 MIDI 文件，其音源文件存储在 Flash 的 `soundfile` 分区中。

本例程默认使用 ESP32_S3_KORVO_2 开发板，通过按键控制播放不同的 MIDI 文件，并支持音量控制，切换 BPM 等功能，按键功能如下：

| 按键    | 功能              |
|-------|-----------------|
| VOL\+ | 音量增大            |
| VOL\- | 音量减小            |
| SET   | 切换待播的 MIDI 文件   |
| PLAY  | 播放当前选中的 MIDI 文件 |
| MUTE  | 循环切换 BPM        |
| REC   | 结束播放            |

## 预备知识

播放所需的音频文件存储在 Flash 的 `soundfile` 分区中。

为了简化烧录操作，本例程使用 `merge_wav_tool.py` 脚本将多个音频文件合并为一个音源文件，并生成对应的索引文件。

### 生成音源文件

1. 将音频文件放入某个文件夹下，如 `resource_path`。

    在本例程中，音频文件名格式为 `index_note_instrument_name_minval_maxval.wav`，如 `1_22_Pedal_Hihat_0_40.wav`。其中每个字段的含义如下：

    | 名称         | 含义      | 类型  | 最小值 | 最大值 |
    |------------|---------|-----|-----|-----|
    | index      | 音源索引    | 整形  | 0   | 127 |
    | note       | 音符序号    | 整形  | 0   | 127 |
    | instrument | 乐器编号    | 整形  | 0   | 127 |
    | name       | 乐器名称    | 字符串 | -   | -   |
    | minval     | 力度范围最小值 | 整形  | 0   | 127 |
    | maxval     | 力度范围最大值 | 整形  | 0   | 127 |

    注意：

    - 文件名格式须严格遵循上述格式，否则脚本无法正确解析音频文件。
    - 所有音频文件需为 WAV 格式，且采样率和声道数须一致。本例程使用的音频文件均为 16 位单声道采样率为 44100 的 WAV 格式音频文件。
    - 若修改音频文件名格式，需同时修改 `merge_wav_tool.py` 脚本和 `main/load_sound_lib/src/esp_midi_flash_loader.c` 中的 `esp_midi_flash_loader_noteon` 函数。

2. 执行 `python3 merge_wav_tool.py -path resource_path` 指令，脚本将在音频文件夹下生成 `midi_sound_files.bin` 音源文件和 `midi_sound_files.h` 索引文件。
3. 将 `midi_sound_files.bin` 和 `midi_sound_files.h` 文件放入 `main/load_sound_lib` 目录下。

### 烧录音源文件

1. 本例程的音源文件存放位置为 Flash 中的 `soundfile` 分区，分区地址和大小在 `partitions.csv` 分区表中配置如下，用户可以根据自己的项目 flash 分区灵活配置地址和分区大小。本例程默认使用的音频文件大小约 8.5MB，板载 Flash 需要至少 16MB 存放全部固件和数据。

    ```code
    soundfile,data, 0xff,    0x110000, 10M,
    ```

2. 执行以下指令将 `main/load_sound_lib` 目录下的 `midi_sound_files.bin` 文件烧录到 `soundfile` 分区。

    以 `esp32s3` 芯片，串口 `/dev/ttyUSB0` 为例，烧录指令如下：

    ```bash
    esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x110000 ./main/midi_sound_files/midi_sound_files.bin
    ```

## 工程配置

将 `midi_samples` 文件夹中的示例 midi 文件拷贝到 SD 卡根目录，用户可以修改 `./main/midi_play.c` 文件中的 `MIDI_FILE_NUM` 和 `midi_file_list` 数组来修改待播放 MIDI 文件的数量和路径。

## 编译和下载

1. 请先编译版本并烧录到开发板上，然后运行 monitor 工具来查看串口输出（替换 PORT 为端口名称）：

    ```bash
    idf.py -p PORT flash monitor
    ```

2. 本例程还需烧录音源文件到 `partitions.csv` 的 `soundfile` 分区，请参考预备知识章节操作

使用 ``Ctrl-]`` 退出调试界面。

有关配置和使用 ESP-IDF 生成项目的完整步骤，请参阅 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.4.1/esp32s3/index.html)。

## FAQ

1. 我的 Flash 不够大，无法烧录音源文件怎么办？

    若您的 Flash 不足 16MB，则无法烧录本例程预置的音源文件。您可以参考[预备知识](#预备知识)章节和[如何自定义音源文件](#如何自定义音源文件)自行准备音源文件并烧录到 Flash 中。

2. 如何自定义音源文件？<a id="如何自定义音源文件"></a>

    本例程默认将音源文件烧录到 Flash 中的 `soundfile` 分区。音源文件由多个参数相同的 WAV 音频文件合并而成。您可以自定义音源文件，需要修改以下几个文件：

    - `merge_wav_tool.py`：解析音频文件名，生成音源索引信息，生成合并后的音源文件
    - `main/load_sound_lib/src/esp_midi_flash_loader.c`：加载音源文件
    - `partitions.csv`：配置音源文件的烧录地址和大小

3. 如果获取更多音色库和音频文件？

    您可以访问以下网站获取更多音色库和音频文件：

    - [Polyphone](https://www.polyphone.io/en/soundfonts)
    - [Musical Artifacts](https://musical-artifacts.com/)
