import os
import struct
import wave
import argparse
from argparse import RawTextHelpFormatter


def merge_wav_files(folder_path, output_data_file, output_index_file):
    # 获取文件夹中所有符合条件的 WAV 文件
    wav_files = [f for f in os.listdir(folder_path) if f.endswith(".wav")]
    wav_files.sort()  # 按文件名排序，确保顺序一致

    # 初始化偏移和索引表
    offset = 0
    index_table = []
    processed_files = 0

    # 打开输出数据文件和索引文件
    with open(output_data_file, "wb") as data_file, open(output_index_file, "w") as index_file:
        index_file.write("/*\n")
        index_file.write(" * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD\n")
        index_file.write(" *\n")
        index_file.write(" * SPDX-License-Identifier: Apache-2.0\n")
        index_file.write(" */\n\n")

        index_file.write("// sound files path: " + folder_path + "\n\n")
        index_file.write("#pragma once\n\n")
        index_file.write("#include <stdio.h>\n")
        index_file.write("#include <stdint.h>\n\n")

        index_file.write("typedef struct {\n")
        index_file.write("    uint8_t       index;\n")
        index_file.write("    uint8_t       note;\n")
        index_file.write("    uint8_t       minval;\n")
        index_file.write("    uint8_t       maxval;\n")
        index_file.write("    unsigned int  offset;\n")
        index_file.write("    unsigned int  size;\n")
        index_file.write("} WavIndex;\n\n")

        index_file.write("WavIndex wav_index_table[] = {\n")

        # 遍历所有 WAV 文件
        for wav_file in wav_files:
            file_path = os.path.join(folder_path, wav_file)

            # 解析文件名，提取 index, note, minval, 和 maxval
            try:
                parts = wav_file.replace(".wav", "").split("_")
                file_index = int(parts[0])
                note = int(parts[1])
                minval = int(parts[-2])
                maxval = int(parts[-1])
            except (ValueError, IndexError):
                print(f"Skipping file with invalid name format: {wav_file}")
                continue

            # 读取 WAV 文件数据
            with wave.open(file_path, "rb") as wav:
                frames = wav.readframes(wav.getnframes())
                size = len(frames)

                # 写入数据到大文件
                data_file.write(frames)

                # 添加到索引表
                index_table.append((file_index, note, minval, maxval, offset, size))
                offset += size
                processed_files += 1

                # 写入索引到索引文件
                index_file.write(f"    {{{file_index}, {note}, {minval}, {maxval}, {offset - size}, {size}}},\n")

        index_file.write("};\n")

    print(f"Data file saved to: {output_data_file}")
    print(f"Index file saved to: {output_index_file}")
    print(f"Successfully processed {processed_files} WAV files.")


# 使用示例
if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Merge wav files", add_help=False,
                                     formatter_class=RawTextHelpFormatter)
    parser.add_argument('-path', type=str, help='wav file folder')
    args = parser.parse_args()

    folder_path = args.path
    if folder_path is None:
        raise Exception("Folder path error.")

    output_data_file = os.path.join(folder_path, 'midi_sound_files.bin')
    output_index_file = os.path.join(folder_path, 'midi_sound_files.h')

    merge_wav_files(folder_path, output_data_file, output_index_file)
