# ESP_MIDI

ESP_MIDI (Musical Instrument Digital Interface) is a comprehensive software library developed by Espressif for efficient MIDI file parsing and real-time audio synthesis.

ESP_MIDI supports SoundFont libraries and custom audio libraries, delivering high-fidelity, distortion-free audio output. By combining the compact size of MIDI files with the rich resources of sound libraries, ESP_MIDI achieves an excellent balance between superior sound quality and high performance, providing developers with a complete MIDI processing solution.

## Features

- Support for Standard MIDI Files (SMF) format parsing
- Real-time MIDI event encoding and decoding, including Note On, Note Off, Program Change, Control Change, Pitch Bend, Channel Pressure, and Meta Events (Tempo, Time Signature, etc.)
- Parse single MIDI events with state tracking and support for incremental parsing
- MIDI encoding: encode MIDI headers, track headers, tempo meta events, and MIDI events to packets
- Support BPM (Beats Per Minute) setting and tempo change with dynamic adjustment
- High-quality audio synthesis from MIDI events using SoundFont samples
- Full SoundFont 2 (SF2) file parsing and playback
- Support for user-defined sound libraries
- Callback-based audio output interface for flexible integration with different audio backends
- Playback control: pause, resume, and time division (ticks per quarter note) configuration

## Architecture

The ESP_MIDI processing flow is as follows:

```text
    MIDI File (SMF)
         │
         ▼
    MIDI Parser ──► MIDI Events ──► SoundFont ──► Synthesizer ──► Audio Output
    (midi_parse)    (processing)    (sf_parse)    (synth)        (callback)
```

## Quick Start

### Adding the Component

Add this component to your ESP-IDF project:

```bash
cd your_project
idf.py add-dependency "espressif/esp_midi"
```

Alternatively, you can add this repository directly to your project's `components` directory.

Add the dependency in your project's `CMakeLists.txt`:

```cmake
idf_component_register(
    ...
    REQUIRES esp_midi
)
```

## Example

For more details on API usage, please refer to the `example` and `test_apps/test_case.c` file.

## Supported Chips


| Chip     | v1.0.0   |
|----------|----------|
| ESP32    | &#10004; |
| ESP32-S2 | &#10004; |
| ESP32-S3 | &#10004; |
| ESP32-P4 | &#10004; |
| ESP32-C2 | &#10004; |
| ESP32-C3 | &#10004; |
| ESP32-C5 | &#10004; |
| ESP32-C6 | &#10004; |
