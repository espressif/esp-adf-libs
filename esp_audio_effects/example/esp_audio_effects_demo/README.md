# ESP Audio Effects Demo

- [中文版本](./README_CN.md)

## Overview

This example demonstrates various audio effects and processing capabilities provided by the ESP Audio Effects library. It showcases how to use different audio effect modules including ALC, Fade, EQ, Sonic, DRC, MBC, Mixer, and Basic Audio Info Convert.

The demo processes embedded audio files and plays them through the audio codec with real-time audio effects applied.

## Supported Demos

The example supports the following audio effect demos:

1. **ALC (Automatic Level Control)** - Controls volume increase and decrease
2. **Fade** - Fade in/out effects
3. **EQ (Equalizer)** - Voice enhancement demo
4. **Sonic** - Speed and pitch adjustment
5. **DRC (Dynamic Range Compression)** - Two sub-demos:
   - Sound Balance Demo
   - Remove Hit Demo
6. **MBC (Multi-Band Compressor)** - Voice enhancement demo
7. **Mixer** - Mix multiple audio streams with fade control
8. **Basic Audio Info Convert** - Convert audio format (sample rate, channels, bit depth)

## Prerequisites

- ESP-IDF release/v5.3 or later
- Supported development boards (e.g., ESP32-S3-Korvo2-V3)

## Build and Flash

Before building this example, make sure the ESP-IDF environment is configured. If already configured, you can skip to the next step. If not configured, you need to run the following scripts in the ESP-IDF root directory to set up the build environment. For complete steps on configuring and using ESP-IDF, please refer to the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32s3/index.html).

```
./install.sh
. ./export.sh
```

Here are the simplified build steps:

1. **Enter the project directory**
   ```bash
   cd $YOUR_ESP_AUDIO_EFFECTS_PATH/example/esp_audio_effects_demo
   ```

2. **Setup Basic Environment**

   Modify the `install.sh` file according to your target chip and development board
   ```bash
   sh ./install.sh
   ```

3. **Configure Example**

   ```bash
   idf.py menuconfig
   ```
   Navigate to `Audio Effects Example` menu:
   - Select the audio effect demo you want to run
   - For DRC demo, you can further select:
     - Sound Balance Demo (default)
     - Remove Hit Demo

4. **Build Project**

   ```bash
   idf.py build
   ```

5. **Flash and Monitor** (Replace `PORT` with your actual serial port)
   ```bash
   idf.py -p PORT flash monitor
   ```

6. Use `Ctrl-]` to exit the serial monitor interface.

## Demo Details

### ALC Demo

**Function**: Controls volume increase and decrease.

**Behavior**:
- Plays audio with ALC processing
- At 4 seconds, sets ALC gain to +6dB to demonstrate gain adjustment

**Audio File**: `voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Fade Demo

**Function**: Applies fade in/out effects to audio.

**Behavior**:
- Starts with fade in effect
- At 4 seconds, switches to fade out effect

**Audio File**: `voice_with_music.pcm` (16kHz, 1ch, 16bit)

### EQ Demo

**Function**: Voice enhancement demo.

**Configuration**:
- 4 filters: High-pass (120Hz), Peak (350Hz, 2200Hz, 4500Hz)
- All filters initially disabled
- At 4 seconds, all filters are enabled

**Audio File**: `voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Sonic Demo

**Function**: Adjusts playback speed and pitch independently.

**Behavior**:
- Plays audio at normal speed and pitch
- At 4 seconds, sets speed to 1.2x and pitch to 0.8x

**Audio File**: `voice_with_music.pcm` (16kHz, 1ch, 16bit)

### DRC Demo

**Function**: Applies dynamic range control to audio.

#### Sound Balance Demo

**Configuration**:
- Compression curve: {0.0, -25.0}, {-50.0, -35.0}, {-100.0, -100.0}
- Attack time: 3ms
- Release time: 50ms

**Audio File**: `voice_flick_up_and_down.pcm` (16kHz, 1ch, 16bit)

#### Remove Hit Demo

**Configuration**:
- Compression curve: {0.0, -20.0}, {-20.0, -20.0}, {-100.0, -100.0}
- Attack time: 1ms
- Release time: 200ms

**Audio File**: `voice_with_hit.pcm` (16kHz, 1ch, 16bit)

**Behavior**:
- First plays original audio
- Then plays DRC processed audio to demonstrate the effect

### MBC Demo

**Function**: Voice enhancement demo.

**Configuration**:
- Frequency bands: 400Hz, 2100Hz, 6000Hz
- All bands initially bypassed
- At 4 seconds, all bands are enabled

**Audio File**: `voice_with_music.pcm` (16kHz, 1ch, 16bit)

### Mixer Demo

**Function**: Mixes two audio streams with fade control.

**Behavior**:
- Stream 1: Background music (`voice_with_music.pcm`)
- Stream 2: Tone signal (`tone.pcm`) - starts after 4 seconds delay via queue
- Applies fade up/down effects to both streams
- Demonstrates queue-based audio streaming

**Audio Files**: 
- `voice_with_music.pcm` (16kHz, 1ch, 16bit)
- `tone.pcm` (16kHz, 1ch, 16bit)

### Basic Audio Info Convert Demo

**Function**: Converts audio format using bit conversion, channel conversion, and rate conversion.

**Conversion Pipeline**:
1. **Channel Conversion**: 2 channels → 1 channel (24bit)
2. **Bit Conversion**: 24bit → 16bit (1 channel)
3. **Rate Conversion**: 48kHz → 16kHz (1 channel, 16bit)

**Input Format**: 48kHz, 2 channels, 24bit
**Output Format**: 16kHz, 1 channel, 16bit

**Audio File**: `manen_48000_2_24_10.pcm` (48kHz, 2ch, 24bit)

**Configuration**:
- Channel conversion: Uses default weight (average)
- Bit conversion: 24bit signed → 16bit signed
- Rate conversion: Complexity 3 (highest quality), Memory optimization mode

## Audio Files

The demo uses embedded audio files that are compiled into the firmware:

- `voice_with_music.pcm` - Default audio file (16kHz, 1ch, 16bit)
- `voice_flick_up_and_down.pcm` - For DRC Sound Balance demo
- `voice_with_hit.pcm` - For DRC Remove Hit demo
- `tone.pcm` - For Mixer demo
- `manen_48000_2_24_10.pcm` - For Basic Audio Info Convert demo (48kHz, 2ch, 24bit)

These files are automatically embedded based on the selected demo configuration.

## Code Structure

```
main/
├── esp_audio_effects_demo.c    # Main demo code
├── CMakeLists.txt              # Build configuration
├── Kconfig.projbuild           # Kconfig options
└── *.pcm                       # Embedded audio files
```

## Configuration Options

### Main Selection (Choice)

- `ESP_AUDIO_EFFECTS_DEMO_SELECT_ALC` - ALC demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_FADE` - Fade demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_EQ` - EQ demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_SONIC` - Sonic demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_DRC` - DRC demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_MBC` - MBC demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_MIXER` - Mixer demo
- `ESP_AUDIO_EFFECTS_DEMO_SELECT_BASIC_AUDIO_INFO_CVT` - Basic Audio Info Convert demo

### DRC Sub-selection (Choice, depends on DRC)

- `ESP_AUDIO_EFFECTS_DEMO_SOUND_BALANCE_DEMO` - Sound Balance Demo (default)
- `ESP_AUDIO_EFFECTS_DEMO_REMOVE_HIT_DEMO` - Remove Hit Demo
