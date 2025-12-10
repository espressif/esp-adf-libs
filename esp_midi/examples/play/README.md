# MIDI Playback Example

- [中文版](./README_CN.md)

## Example Overview

This example demonstrates how to decode and play MIDI files stored on an SD card, with sound source files stored in the `soundfile` partition of Flash.

This example uses the ESP32_S3_KORVO_2 development board by default, controlling playback of different MIDI files through buttons, and supports volume control, BPM switching, and other functions. The button functions are as follows:

| Button | Function |
|--------|----------|
| VOL\+ | Increase volume |
| VOL\- | Decrease volume |
| SET   | Switch to next MIDI file to play |
| PLAY  | Play the currently selected MIDI file |
| MUTE  | Cycle through BPM settings |
| REC   | Stop playback |

## Prerequisites

The audio files required for playback are stored in the `soundfile` partition of Flash.

To simplify the flashing operation, this example uses the `merge_wav_tool.py` script to merge multiple audio files into a single sound source file and generate the corresponding index file.

### Generating Sound Files

1. Place audio files in a folder, such as `resource_path`.

    In this example, the audio file name format is `index_note_instrument_name_minval_maxval.wav`, such as `1_22_Pedal_Hihat_0_40.wav`. The meaning of each field is as follows:

    | Name | Meaning | Type | Min | Max |
    |------|---------|------|-----|-----|
    | index | Sound source index | Integer | 0 | 127 |
    | note | Note number | Integer | 0 | 127 |
    | instrument | Instrument number | Integer | 0 | 127 |
    | name | Instrument name | String | - | - |
    | minval | Minimum velocity range | Integer | 0 | 127 |
    | maxval | Maximum velocity range | Integer | 0 | 127 |

    Notes:

    - The file name format must strictly follow the above format, otherwise the script cannot correctly parse the audio files.
    - All audio files must be in WAV format, and the sample rate and number of channels must be consistent. The audio files used in this example are all 16-bit mono WAV format audio files with a sample rate of 44100.
    - If you modify the audio file name format, you must also modify the `merge_wav_tool.py` script and the `esp_midi_flash_loader_noteon` function in `main/load_sound_lib/src/esp_midi_flash_loader.c`.

2. Execute the command `python3 merge_wav_tool.py -path resource_path`. The script will generate the `midi_sound_files.bin` sound source file and `midi_sound_files.h` index file in the audio folder.
3. Place the `midi_sound_files.bin` and `midi_sound_files.h` files in the `main/load_sound_lib` directory.

### Flashing Sound Files

1. The sound source files for this example are stored in the `soundfile` partition of Flash. The partition address and size are configured in the `partitions.csv` partition table as follows. Users can flexibly configure the address and partition size according to their project's flash partition. The default audio files used in this example are approximately 8.5MB in size, and the onboard Flash needs at least 16MB to store all firmware and data.

    ```code
    soundfile,data, 0xff,    0x110000, 10M,
    ```

2. Execute the following command to flash the `midi_sound_files.bin` file from the `main/load_sound_lib` directory to the `soundfile` partition.

    Taking the `esp32s3` chip and serial port `/dev/ttyUSB0` as an example, the flashing command is as follows:

    ```bash
    esptool.py --chip esp32s3 --port /dev/ttyUSB0 --baud 921600 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect 0x110000 ./main/midi_sound_files/midi_sound_files.bin
    ```

## Project Configuration

Copy the sample MIDI files from the `midi_samples` folder to the SD card root directory. Users can modify the `MIDI_FILE_NUM` and `midi_file_list` array in the `./main/midi_play.c` file to change the number and paths of MIDI files to be played.

## Build and Flash

1. First compile and flash the firmware to the development board, then run the monitor tool to view serial output (replace PORT with your port name):

    ```bash
    idf.py -p PORT flash monitor
    ```

2. This example also requires flashing the sound source files to the `soundfile` partition in `partitions.csv`. Please refer to the Prerequisites section for operations.

Use ``Ctrl-]`` to exit the debug interface.

For complete steps on configuring and using ESP-IDF to generate projects, please refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html).

## FAQ

1. What should I do if my Flash is not large enough to flash the sound source files?

    If your Flash is less than 16MB, you cannot flash the preset sound source files for this example. You can refer to the [Prerequisites](#prerequisites) section and [How to Customize Sound Source Files](#how-to-customize-sound-source-files) to prepare your own sound source files and flash them to Flash.

2. How to customize sound source files?<a id="how-to-customize-sound-source-files"></a>

    This example flashes sound source files to the `soundfile` partition in Flash by default. Sound source files are formed by merging multiple WAV audio files with the same parameters. You can customize sound source files by modifying the following files:

    - `merge_wav_tool.py`: Parse audio file names, generate sound source index information, and generate merged sound source files
    - `main/load_sound_lib/src/esp_midi_flash_loader.c`: Load sound source files
    - `partitions.csv`: Configure the flashing address and size of sound source files

3. How to get more sound libraries and audio files?

    You can visit the following websites to get more sound libraries and audio files:

    - [Polyphone](https://www.polyphone.io/en/soundfonts)
    - [Musical Artifacts](https://musical-artifacts.com/)
