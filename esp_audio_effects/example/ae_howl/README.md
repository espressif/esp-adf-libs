# ESP Howling Suppression (Karaoke) Demo

- [中文版本](./README_CN.md)
- Difficulty: ⭐⭐

## Example Introduction

This example demonstrates a karaoke processing flow with howling suppression: microphone input is captured, optionally processed by `esp_ae_howl` howling suppression, mixed with background music from the SD card, and finally played through the codec.

- The example uses a single real-time processing path: microphone capture -> optional howling suppression -> mixing -> DAC output.
- It demonstrates how to suppress sustained narrowband howling in a "microphone + speaker" acoustic feedback loop.
- Howling suppression is controlled by buttons: **single-click PLAY** to enable, **single-click SET** to disable, so effects can be compared in real time.

### Signal Flow

```
Microphone (ADC) -> [optional Howling Suppression] -> Mixer <- Music (SD: /sdcard/test_dukou.pcm)
                    ↓                              ↓
               audio_buffer                  music_buffer
                    ↓                              ↓
                    └────────── Mixer ────────────┘
                                    ↓
                            out_buffer -> Codec (DAC) -> Speaker
```

## Environment Setup

### Hardware Requirements

- **Development board**: A board that supports microphone (ADC), speaker (DAC), and SD card (for example, ESP32-S3-Korvo2/Korvo2 V3).
- **Required resources**: SD card, microphone input, speaker output.

### Default IDF Branch

This example supports ESP-IDF `release/v5.4` and later.

### Software Requirements

- Place the PCM file in the SD card root directory: `/sdcard/test_dukou.pcm`
- File format: 16 kHz, mono, 16 bit

## Build and Flash

### Build Preparation

Before building this example, ensure the ESP-IDF environment is configured. If not configured, first run environment installation and export in the ESP-IDF root directory. For full steps, refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html).

```bash
./install.sh
. ./export.sh
```

Abbreviated steps:

- Enter the example project directory:

```bash
cd $YOUR_ESP_AUDIO_EFFECTS_PATH/example/ae_howl
```

- Install `esp-bmgr-assist` on Linux:

```bash
pip install esp-bmgr-assist
pip install --upgrade esp-bmgr-assist
```

- List supported boards:

```bash
idf.py bmgr -l
```

Output example:

```text
ℹ️  Main Boards:
  [1] dual_eyes_board_v1_0
  [2] esp32_c3_lyra
  [3] esp32_c5_spot
  [4] esp32_p4_function_ev
  [5] esp32_s3_korvo2_v3
  [6] esp32_s3_korvo2l
  [7] esp_box_3
  [8] esp_box_lite
  [9] esp_hi
```

- Select a board (replace `number` with an index from the list above):

```bash
idf.py bmgr -b number
```

### Project Configuration

This example configures major audio parameters and howling thresholds in code. If you need to tune them, see `demo_howl_process()` in `main/esp_howl_demo.c`.

### Build and Flash

- Build the example:

```bash
idf.py build
```

- Flash and monitor logs (replace `PORT` with your actual serial port):

```bash
idf.py -p PORT flash monitor
```

- Press `Ctrl-]` to exit the monitor.

## How to Use the Example

### Features and Usage

- After power-on, howling suppression is disabled by default. The system reads microphone and SD card music and mixes them for playback.
- **Single-click PLAY** to enable `esp_ae_howl` howling suppression, and **single-click SET** to disable it, so you can compare processing effects in real time.
- It is recommended to confirm `/sdcard/test_dukou.pcm` exists on the SD card before power-on.

### Howling Suppression Parameters

Default configuration (see `esp_howl_demo.c`):

| Parameter       | Value | Description |
|----------------|-------|-------------|
| sample_rate     | 16000 | Sample rate (Hz) |
| channel         | 1 | Mono |
| bits_per_sample | 16 | Bit depth |
| papr_th         | 8.0  | PAPR threshold (dB), range [-10, 20] |
| phpr_th         | 45.0 | PHPR threshold (dB), range [0, 100] |
| pnpr_th         | 45.0 | PNPR threshold (dB), range [0, 100] |
| imsd_th         | 5.0  | IMSD threshold (effective when do_imsd_check is true), range [0, 20] |
| do_imsd_check   | true | Enabling IMSD reduces false alarms in music scenarios; set false in speech-only scenarios to save memory and CPU |

## Code Structure

```text
main/
├── esp_howl_demo.c    # Main logic: codec initialization, howl + mixer pipeline, button handling
├── CMakeLists.txt     # Build configuration for main component
└── idf_component.yml  # ESP-IDF component dependencies (optional)
```

- **Button handling**: In `simple_button_event_handler`, single-click PLAY enables `do_howl_process`, single-click SET disables `do_howl_process`.
- **Main loop**: Read microphone -> optional `esp_ae_howl_process` -> read music -> `esp_ae_mixer_process` -> write to DAC.

## Troubleshooting

### No Sound or Playback Errors

- Check whether codec and board-level audio peripherals are initialized successfully.
- Confirm the SD card is mounted correctly and `test_dukou.pcm` exists in the root directory with format 16 kHz / mono / 16 bit.

### Howling Is Still Obvious

- Increase the distance between microphone and speaker, or lower speaker volume.
- Confirm howling suppression has been enabled via button.
- Fine-tune `papr_th`, `phpr_th`, `pnpr_th`, and `imsd_th` based on your acoustic environment.

### Button Not Responding

- Confirm the ADC button group is initialized and callback is registered.
- Check hardware mapping with `init_adc_button()` and board-manager-related docs.

## Technical Support

For support, use the following channels:

- Technical discussion: [esp32.com](https://esp32.com/viewforum.php?f=20)
- Issue reporting: [GitHub issue](https://github.com/espressif/esp-adf/issues)
