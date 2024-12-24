# MIDI Keyboard Example

- [中文版](./README_CN.md)

## Example Overview

This example demonstrates how to process MIDI messages in real-time and control audio playback, implementing a simple keyboard function. It uses a matrix keyboard or USB keyboard as the keyboard input device, which can be configured through menuconfig. The key-to-note mappings for both keyboard types can be modified in the code.

If using a USB keyboard, the default key-to-note mapping diagram is as follows:

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

If using a 5x5 matrix keyboard, the default key-to-note mapping diagram is as follows:

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

The system block diagram is as follows:

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

This example uses two rotary encoders as control knobs by default, and you can customize the operations performed when rotating and pressing. The default knob functions are as follows:

| Button | Function |
|--------|----------|
| Knob A - Hold and rotate left | Decrease pitch range by one semitone (1 semitone) |
| Knob A - Rotate left | Decrease pitch range by one octave (12 semitones) |
| Knob A - Hold and rotate right | Increase pitch range by one semitone (1 semitone) |
| Knob A - Rotate right | Increase pitch range by one octave (12 semitones) |
| Knob B - Hold and rotate left | Switch instrument (index -1) |
| Knob B - Rotate left | Decrease playback speed by 0.25 |
| Knob B - Hold and rotate right | Switch instrument (index +1) |
| Knob B - Rotate right | Increase playback speed by 0.25 |

## Prerequisites

The audio files required for playback are stored in the `soundfile` partition of Flash.

To simplify the flashing operation, this example uses the `merge_wav_tool_keynote.py` script to merge multiple audio files into a single sound source file and generate the corresponding index file.

### Generating Sound Files

1. Place audio files in a folder, such as `resource_path`.

    In this example, the audio file name format is `instr/drum_name_index_note.wav`, such as `instr_guitar_025_038.wav`. The meaning of each field is as follows:

    | Name | Meaning | Type | Min | Max |
    |------|---------|------|-----|-----|
    | instr/drum | Instrument or percussion | String | - | - |
    | name | Instrument name | String | - | - |
    | index | Instrument number | Integer | 0 | 127 |
    | note | Note number | Integer | 0 | 127 |

    Notes:

    - The file name format must strictly follow the above format, otherwise the script cannot correctly parse the audio files.
    - All audio files must be in WAV format, and the sample rate and number of channels must be consistent. The audio files used in this example are all 16-bit mono WAV format audio files with a sample rate of 16000.
    - If you modify the audio file name format, you must also modify the `merge_wav_tool_keynote.py` script and the `esp_midi_flash_loader_noteon` function in `main/load_sound_lib/src/esp_midi_flash_loader.c`.

2. Execute the command `python3 merge_wav_tool_keynote.py -path resource_path`. The script will generate the `midi_sound_files.bin` sound source file and `midi_sound_files.h` index file in the audio folder.
3. Place the `midi_sound_files.bin` and `midi_sound_files.h` files in the `main/load_sound_lib` directory.

### Flashing Sound Files

1. The sound source files for this example are stored in the `soundfile` partition of Flash. The partition address and size are configured in the `partitions.csv` partition table as follows. Users can flexibly configure the address and partition size according to their project's flash partition. The default audio files used in this example are approximately 15.4MB in size, and the onboard Flash needs at least 16MB to store all firmware and data.

    ```code
    soundfile,data, 0xff,    0x110000, 15296K,
    ```

2. Execute the following command to flash the `midi_sound_files.bin` file from the `main/load_sound_lib` directory to the `soundfile` partition.

    Taking the `esp32s3` chip and serial port `/dev/ttyACM0` as an example, the flashing command is as follows:

    ```bash
    esptool.py --chip esp32s3 --port /dev/ttyACM0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x110000 ./main/midi_sound_files/midi_sound_files.bin
    ```

## Project Configuration

The pin configuration of the development board can be modified in `main/app_board.h`.

This example uses a matrix keyboard as the input device by default, which can be configured through menuconfig. If using a USB keyboard, you need to select `USB Keyboard` as the input device in menuconfig.

```plain
menuconfig > Keyboard Example Configuration > Input Key Selection > USB Keyboard
```

## Build and Flash

1. Execute the following command to select the chip model to use. The components that the example depends on will be automatically downloaded to the `managed_components` folder.

    ```bash
    idf.py set-target esp32s3
    ```

2. This example uses `esp_board_manager` to manage development board hardware resources. You need to add the path of `esp_board_manager` to the environment variables. If using Ubuntu or Mac systems, please execute the following command. For other systems, please refer to the [esp_board_manager documentation](https://github.com/espressif/esp-gmf/blob/main/packages/esp_board_manager/README.md#1-setup-idf-action-extension):

    ```bash
    export IDF_EXTRA_ACTIONS_PATH=./managed_components/espressif__esp_board_manager
    ```

3. Generate the board-level configuration file. The default hardware configuration files are stored in the `components/midi_keyboard/` directory. After executing the following command, the `gen_bmgr_codes` configuration code will be automatically generated. If you modify the hardware configuration files, you need to re-execute this command.

    ```bash
    idf.py gen-bmgr-config -b midi_keyboard -c components/midi_keyboard
    ```

4. Compile the project and flash it to the development board, then run the monitor tool to view serial output (replace PORT with your port name):

    ```bash
    idf.py -p PORT flash monitor
    ```

5. This example also requires flashing the sound source files to the `soundfile` partition specified in the `partitions.csv` partition table. Please refer to the Prerequisites section for operations.

Use ``Ctrl-]`` to exit the debug interface.

For complete steps on configuring and using ESP-IDF to generate projects, please refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html).

## FAQ

1. What should I do if my Flash is not large enough to flash the sound source files?

    If your Flash is less than 16MB, you cannot flash the preset sound source files for this example. You can refer to the [Prerequisites](#prerequisites) section and [How to Customize Sound Source Files](#how-to-customize-sound-source-files) to prepare your own sound source files and flash them to Flash.

2. How to customize sound source files?<a id="how-to-customize-sound-source-files"></a>

    This example flashes sound source files to the `soundfile` partition in Flash by default. Sound source files are formed by merging multiple WAV audio files with the same parameters. You can customize sound source files by modifying the following files:

    - `merge_wav_tool_keynote.py`: Parse audio file names, generate sound source index information, and generate merged sound source files
    - `main/load_sound_lib/src/esp_midi_flash_loader.c`: Load sound source files
    - `partitions.csv`: Configure the flashing address and size of sound source files

3. How to get more sound libraries and audio files?

    You can visit the following websites to get more sound libraries and audio files:

    - [Polyphone](https://www.polyphone.io/en/soundfonts)
    - [Musical Artifacts](https://musical-artifacts.com/)
