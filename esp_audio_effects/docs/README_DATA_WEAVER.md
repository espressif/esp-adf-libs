# DATA WEAVER

`DATA WEAVER` module is used to interleave multiple independently stored channel data together and to split the interleaved data into independent buffers for each channel.

# Features

- Support full range of sample rates and channel
- Support bits per sample: s16, s24, s32

# Performance

Tested with the following system configuration:<br>
|      Chip      | IDF Version  | CPU Frequency | SPI Ram Frequency |
|       --       |     --       |  --           |     --            |  
|   ESP32-S3R8   |    v5.3      | 240MHz        |   80MHz           |

| Bits per sample| Heap Memory(Byte) | CPU loading(%) |
|       --       |  --               |     --         |
|       16       |  0                |    < 0.05      |
|       24       |  0                |    < 0.05      |
|       32       |  0                |    < 0.05      |

Note:
1) The test music is in 8 kHz mono format. The CPU load for music with different sampling rates and channel configurations can be estimated using the formula:
   >CPU Load = (sample_rate / 8000) * channel_count * base_load<br>

   where `base_load` is the CPU load listed in table.
2) The CPU load values in the table represent average measurements.

# Usage

Here is an example of using [DATA WEAVER](../test_app/main/test_data_weaver.c)

# FAQ

1) What's the function of Data Weaver?
   >In audio processing, there are interleaved and non-interleaved data processing modules, mainly due to different application scenarios and requirements. Here are the common application scenarios for each:
   - Interleaved Data Processing:
     - Efficient Data Transmission: Interleaved format is advantageous when efficiency in data transmission is crucial, especially in real-time audio streaming applications or when dealing with large volumes of audio data.
     - Multi-Channel Audio Processing: Interleaved format is commonly used in applications requiring simultaneous processing of multiple audio channels.
     - Compatibility: Interleaved format is widely supported by audio processing tools and libraries, making it suitable for interoperability between different systems and platforms.
   - Non-interleaved Data Processing:
     - Independent Channel Processing: Non-interleaved format is preferred when individual control or processing of each audio channel is necessary, such as in equalization.
     - Modular Audio Systems: In modular audio processing systems where different processing modules handle specific channels independently, non-interleaved format facilitates modular design and processing.

