# MIDI 电子琴 example

- [English](./README.md)

## 例程简介

本例程演示了如何实时处理 MIDI 消息并控制音频播放，实现了一个简单的电子琴功能。使用支持矩阵键盘或 USB 键盘作为电子琴的琴键输入设备，可以通过 menuconfig 进行配置。两种键盘对应音符的映射均可以在代码中进行修改。

若使用 USB 键盘，默认的按键对应音符示意图如下：

```plain
    ┌───┬───┐   ┌───┬───┬───┐                               
    │ 2 │ 3 │   │ 5 │ 6 │ 7 │                               
  ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐                             
  │ Q │ W │ E │ R │ T │ Y │ U │                             
  └───┴───┴───┴───┴───┴───┴───┘ ┌───┬───┐   ┌───┬───┬───┐   
    0 1 2 3 4 ...           11  │ F │ G │   │ J │ K │ L │   
                              ┌─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┬─┴─┐ 
                              │ C │ V │ B │ N │ M │ < │ > │ 
                              └───┴───┴───┴───┴───┴───┴───┘ 
                               12 13 ...                23  
```

若使用 5x5 的矩阵键盘，默认的按键对应音符示意图如下：

```plain
         ┌──┐   ┌──┐          ┌──┐    
         │ 1│   │ 3│          │ 6│    
     ┌───┼──┼───┼──┼───┐  ┌───┼──┼───┐
     │ 0 │  │ 2 │  │ 4 │  │ 5 │  │ 7 │
     └───┘  └───┘  └───┘  └───┘  └───┘
  ┌──┐   ┌──┐          ┌──┐   ┌──┐    
  │ 8│   │10│          │13│   │15│    
  └──┼───┼──┼───┐  ┌───┼──┼───┼──┼───┐
     │ 9 │  │ 11│  │ 12│  │ 14│  │ 16│
     └───┘  └───┘  └───┘  └───┘  └───┘
         ┌──┐   ┌──┐   ┌──┐           
         │18│   │20│   │22│           
     ┌───┼──┼───┼──┼───┼──┼───┐  ┌───┐
     │ 17│  │ 19│  │ 21│  │ 23│  │ 24│
     └───┘  └───┘  └───┘  └───┘  └───┘
```

系统框图如下：

```plain
                                                                                                  
                             ┌──────────────────────────┐                                         
                             │         ESP32-S3         │                                         
                             │                          │                                         
   ┌────────────────────────►│GPIO6               GPIO48├──►                                      
   │                         │                          │                                         
   │   ┌────────────────────►│GPIO10              GPIO1 ├──► power control                        
   │   │                     │                          │                                         
   │   │   ┌────────────────►│GPIO11              GPIO3 ├──►                                      
   │   │   │                 │                          │                                         
   │   │   │   ┌────────────►│GPIO12                    │    ┌───────────┐                        
   │   │   │   │             │                          │    │   codec   │                        
   │   │   │   │   ┌────────►│GPIO13                    │    │           │                        
   │   │   │   │   │         │                    GPIO16├───►│MCLK       │   ┌───────┐  ┌───────┐ 
                             │                          │    │           │   │  PA   │  │  SPK  │ 
   │   │   │   │   │         │                    GPIO45├───►│WS         ├──►│       ├─►│       │ 
 ──┼───┼───┼───┼───┼── ─────►│GPIO39                    │    │           │   │       │  │       │ 
   │   │   │   │   │         │                    GPIO9 ├───►│BCLK       │ ┌►│EN     │  │       │ 
 ──┼───┼───┼───┼───┼── ─────►│GPIO40                    │    │           │ │ │       │  │       │ 
   │   │   │   │   │         │                    GPIO8 ├───►│DIN        │ │ └───────┘  └───────┘ 
 ──┼───┼───┼───┼───┼── ─────►│GPIO41                    │    │           │ │                      
   │   │   │   │   │         │                          │    └───────────┘ │                      
 ──┼───┼───┼───┼───┼── ─────►│GPIO42                    │                  │                      
   │   │   │   │   │         │                    GPIO47│──────────────────┘                      
 ──┼───┼───┼───┼───┼── ─────►│GPIO46                    │    ┌───────────┐                        
   │   │   │   │   │         │                          │    │   knob    │                        
                             │                          │    │           │                        
    matrix keyboard    ┌────►│GPIO35              GPIO37│◄───┤A          │                        
                       │     │                          │    │           │                        
         ┌───────────┐ │ ┌──►│GPIO34              GPIO38│◄───┤B          │                        
         │   knob    │ │ │   │                          │    │           │                        
         │           │ │ │ ┌►│GPIO33              GPIO36│◄───┤D          │                        
         │          A├─┘ │ │ │                          │    │           │                        
         │           │   │ │ └──────────────────────────┘    └───────────┘                        
         │          B├───┘ │                                                                      
         │           │     │                                                                      
         │          D├─────┘                                                                      
         │           │                                                                            
         └───────────┘                                                                            
                                                                                                  
```

本例程默认使用两个旋转编码器作为控制旋钮，可以自定义旋转和按下时执行的操作。默认的旋钮功能如下：

| 按键           | 功能               |
|--------------|------------------|
| 旋钮A - 按住向左滚动 | 音域减少一度（1 个半音）    |
| 旋钮A - 向左滚动   | 音域减少一个八度（12 个半音） |
| 旋钮A - 按住向右滚动 | 音域增加一度（1 个半音）    |
| 旋钮A - 向右滚动   | 音域增加一个八度（12 个半音） |
| 旋钮B - 按住向左滚动 | 切换乐器（序号 -1）      |
| 旋钮B - 向左滚动   | 播放速度 -0.25       |
| 旋钮B - 按住向右滚动 | 切换乐器（序号 +1）      |
| 旋钮B - 向右滚动   | 播放速度 +0.25       |

## 预备知识

播放所需的音频文件存储在 Flash 的 `soundfile` 分区中。

为了简化烧录操作，本例程使用 `merge_wav_tool_keynote.py` 脚本将多个音频文件合并为一个音源文件，并生成对应的索引文件。

### 生成音源文件

1. 将音频文件放入某个文件夹下，如 `resource_path`。

    在本例程中，音频文件名格式为 `instr/drum_name_index_note.wav`，如 `instr_guitar_025_038.wav`。其中每个字段的含义如下：

    | 名称         | 含义      | 类型  | 最小值 | 最大值 |
    |------------|---------|-----|-----|-----|
    | instr/drum | 乐器或打击乐器 | 字符串 | -   | -   |
    | name       | 乐器名称    | 字符串 | -   | -   |
    | index      | 乐器序号    | 整形  | 0   | 127 |
    | note       | 音符序号    | 整形  | 0   | 127 |

    注意：

    - 文件名格式须严格遵循上述格式，否则脚本无法正确解析音频文件。
    - 所有音频文件需为 WAV 格式，且采样率和声道数须一致。本例程使用的音频文件均为 16 位单声道采样率为 16000 的 WAV 格式音频文件。
    - 若修改音频文件名格式，需同时修改 `merge_wav_tool_keynote.py` 脚本和 `main/load_sound_lib/src/esp_midi_flash_loader.c` 中的 `esp_midi_flash_loader_noteon` 函数。

2. 执行 `python3 merge_wav_tool_keynote.py -path resource_path` 指令，脚本将在音频文件夹下生成 `midi_sound_files.bin` 音源文件和 `midi_sound_files.h` 索引文件。
3. 将 `midi_sound_files.bin` 和 `midi_sound_files.h` 文件放入 `main/load_sound_lib` 目录下。

### 烧录音源文件

1. 本例程的音源文件存放位置为 Flash 中的 `soundfile` 分区，分区地址和大小在 `partitions.csv` 分区表中配置如下，用户可以根据自己的项目 flash 分区灵活配置地址和分区大小。本例程默认使用的音频文件大小约 15.4MB，板载 Flash 需要至少 16MB 存放全部固件和数据。

    ```code
    soundfile,data, 0xff,    0x110000, 15296K,
    ```

2. 执行以下指令将 `main/load_sound_lib` 目录下的 `midi_sound_files.bin` 文件烧录到 `soundfile` 分区。

    以 `esp32s3` 芯片，串口 `/dev/ttyACM0` 为例，烧录指令如下：

    ```bash
    esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x110000 ./main/midi_sound_files/midi_sound_files.bin
    ```

## 工程配置

开发板的管脚配置可以在 `main/app_board.h` 中进行修改。

本例程默认使用矩阵键盘作为输入设备，用户可以通过 menuconfig 进行配置。若使用 USB 键盘，需在 menuconfig 中选择 `USB Keyboard` 作为输入设备。

```plain
menuconfig > Keyboard Example Configuration > Input Key Selection > USB Keyboard
```

## 编译和下载

1. 执行以下命令选择使用的芯片型号，例程依赖的组件将自动下载到 `managed_components` 文件夹中。

    ```bash
    idf.py set-target esp32s3
    ```

2. 本例程使用 `esp_board_manager` 管理开发板硬件资源，需要将  `esp_board_manager` 的路径添加进环境变量中。若使用 Ubuntu 或 Mac 系统，请执行以下命令，其他系统请参考 [esp_board_manager 文档](https://github.com/espressif/esp-gmf/blob/main/packages/esp_board_manager/README_CN.md#1-%E8%AE%BE%E7%BD%AE-idf-action-%E6%89%A9%E5%B1%95)：

    ```bash
    export IDF_EXTRA_ACTIONS_PATH=./managed_components/espressif__esp_board_manager
    ```

3. 生成板级配置文件。默认的硬件配置文件存放在 `components/midi_keyboard/` 目录下。执行以下指令后，将自动生成 `gen_bmgr_codes` 配置代码，若修改了硬件配置文件，需重新执行此指令。

    ```bash
    idf.py gen-bmgr-config -b midi_keyboard -c components/midi_keyboard
    ```

4. 编译工程并烧录到开发板上，然后运行 monitor 工具来查看串口输出（替换 PORT 为端口名称）：

    ```bash
    idf.py -p PORT flash monitor
    ```

5. 本例程还需烧录音源文件到 `partitions.csv` 分区表中指定的 `soundfile` 分区，请参考预备知识章节操作

使用 ``Ctrl-]`` 退出调试界面。

有关配置和使用 ESP-IDF 生成项目的完整步骤，请参阅 [《ESP-IDF 编程指南》](https://docs.espressif.com/projects/esp-idf/zh_CN/v5.4.1/esp32s3/index.html)。

## FAQ

1. 我的 Flash 不够大，无法烧录音源文件怎么办？

    若您的 Flash 不足 16MB，则无法烧录本例程预置的音源文件。您可以参考[预备知识](#预备知识)章节和[如何自定义音源文件](#如何自定义音源文件)自行准备音源文件并烧录到 Flash 中。

2. 如何自定义音源文件？<a id="如何自定义音源文件"></a>

    本例程默认将音源文件烧录到 Flash 中的 `soundfile` 分区。音源文件由多个参数相同的 WAV 音频文件合并而成。您可以自定义音源文件，需要修改以下几个文件：

    - `merge_wav_tool_keynote.py`：解析音频文件名，生成音源索引信息，生成合并后的音源文件
    - `main/load_sound_lib/src/esp_midi_flash_loader.c`：加载音源文件
    - `partitions.csv`：配置音源文件的烧录地址和大小

3. 如果获取更多音色库和音频文件？

    您可以访问以下网站获取更多音色库和音频文件：

    - [Polyphone](https://www.polyphone.io/en/soundfonts)
    - [Musical Artifacts](https://musical-artifacts.com/)
