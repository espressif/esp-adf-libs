# format "instr_guitar_025_047.wav", only parse instrument_number and note
import os
from pathlib import Path
import struct
import wave
import argparse
from argparse import RawTextHelpFormatter
from collections import defaultdict

def merge_wav_files(folder_path, output_data_file, output_index_file):
    wav_files = [f for f in os.listdir(folder_path) if f.endswith(".wav")]
    wav_files.sort()

    offset = 0
    index_table = []
    processed_files = 0

    # 统计信息：乐器数量、总音符数量、各乐器音符范围
    instrument_note_map = defaultdict(list)

    with open(output_data_file, "wb") as data_file, open(output_index_file, "w") as index_file:
        index_file.write("/*\n")
        index_file.write(" * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD\n")
        index_file.write(" *\n")
        index_file.write(" * SPDX-License-Identifier: Apache-2.0\n")
        index_file.write(" */\n\n")

        index_file.write("// sound files path: " + str(Path(folder_path).resolve()) + "\n\n")
        index_file.write("#pragma once\n\n")
        index_file.write("#include <stdio.h>\n")
        index_file.write("#include <stdint.h>\n\n")

        index_file.write("typedef struct {\n")
        index_file.write("    unsigned int  instrument;\n")
        index_file.write("    uint8_t       note;\n")
        index_file.write("    unsigned int  offset;\n")
        index_file.write("    unsigned int  size;\n")
        index_file.write("} WavIndex;\n\n")

        index_file.write("static const WavIndex wav_index_table[] = {\n")

        for wav_file in wav_files:
            file_path = os.path.join(folder_path, wav_file)
            try:
                parts = wav_file.replace(".wav", "").split("_")
                instrument = int(parts[-2])
                note = int(parts[-1])
            except (ValueError, IndexError):
                print(f"Skipping file with invalid name format: {wav_file}")
                continue

            with wave.open(file_path, "rb") as wav:
                frames = wav.readframes(wav.getnframes())
                size = len(frames)
                data_file.write(frames)

                index_table.append((instrument, note, offset, size))
                offset += size
                processed_files += 1

                instrument_note_map[instrument].append(note)

                index_file.write(f"    {{{instrument}, {note}, {offset - size}, {size}}},\n")

        index_file.write("};\n\n")

        # 输出 instrument_info_table 和统计信息
        instrument_list = sorted(instrument_note_map.keys())
        total_notes = sum(len(notes) for notes in instrument_note_map.values())

        index_file.write("typedef struct {\n")
        index_file.write("    uint8_t  instrument;\n")
        index_file.write("    uint8_t  min_note;\n")
        index_file.write("    uint8_t  max_note;\n")
        index_file.write("} Instrument_info;\n\n")

        index_file.write("static const Instrument_info instrument_info_table[] = {\n")
        for ins in instrument_list:
            min_note = min(instrument_note_map[ins])
            max_note = max(instrument_note_map[ins])
            index_file.write(f"    {{{ins}, {min_note}, {max_note}}},\n")
        index_file.write("};\n\n")

        index_file.write("#define MIDI_TOTAL_NOTE_NUMBER %d\n" % total_notes)
        index_file.write("#define MIDI_INSTRUMENT_NUMBER %d\n" % len(instrument_list))

    print(f"Data file saved to: {output_data_file}")
    print(f"Index file saved to: {output_index_file}")
    print(f"Successfully processed {processed_files} WAV files.")


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
