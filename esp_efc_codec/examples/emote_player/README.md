# LVGL EFC Player

- [中文版本](./README_CN.md)
- Regular Example: ⭐⭐

## Example Brief

This example shows how to play emotes with `esp_efc_codec`. It lists every clip in `/sdcard/totest` whose name ends in `.efc` (plain `.efc` and zlib `.efc_z`), plays the first one, and lets you switch, restart, pause, and resume from the screen. The stage scales the clip to fit. The label under it is the frames per second decode and the panel are actually reaching, with no target frame rate.

`lv_efc` is the LVGL image widget that owns the clip. It decodes each frame in place into RGB565, and adds an alpha plane when the file carries a color key.

### Resources

- LVGL task stack: 20 KB, where each frame is decoded
- Main task stack: 12 KB, used for board bring-up
- PSRAM holds the clip and the frame buffer


### Prerequisites

- A working [ESP-IDF v5.5](https://docs.espressif.com/projects/esp-idf/en/v5.5/esp32s3/get-started/index.html) install, with `IDF_PATH` set
- Clips already encoded to EFC. See [Convert a GIF to EFC](#convert-a-gif-to-efc). Copy the `.efc` files into a `totest` directory on the card. The player opens `/sdcard/totest`.


### Folder contents

```
├── main
│   ├── CMakeLists.txt
│   ├── idf_component.yml       Board manager, touch, and the LVGL adapter
│   └── efc_player_main.c       File list, buttons, and the lv_efc stage
├── CMakeLists.txt              Project file; the codec component is two levels up
├── partitions.csv              Optional 6 MB factory partition
├── sdkconfig.defaults          Widget on, long file names, 12 KB main stack
├── sdkconfig.defaults.esp32s3  240 MHz and octal PSRAM
├── sdkconfig.defaults.esp32p4  PSRAM defaults if you retarget the example
└── README.md                   This is the file you are currently reading
```


## Convert a GIF to EFC

The encoder for end users is the JavaScript one in `pc/js`. It runs in a browser or under Node.js. Either way you get an `.efc` this player can open.

Leave the tile at **4**. This example is built to play fixed 4×4 clips. zlib is on by default and is stored inside the file, so the download can be named `smile.efc` or `smile.efc_z`. Both show up in the on-screen list. Copy the file into `totest` on the card.

### In the browser

Open `pc/js/dist/encode.html` in the browser. Double-click the file, or pass its path to the browser. The page includes all dependencies, so no network connection or local server is required.

1. Choose a GIF. The page shows it and loops the preview.
2. Leave **Tile** at `4 (device decoder)` and leave **zlib** checked.
3. Press **Encode**.
4. Press **Download**. The file name is the GIF name with `.efc` on the end.

Encoding stays in the browser. The status line shows the GIF size, the `.efc` size, and how long the encode took.

### With Node.js

Node.js 18 or later:

```bash
cd pc/js/dist
node encode_gif.mjs smile.gif smile.efc
```

The released script includes its dependencies, so `npm install` is not required. It prints the size, frame count, and byte sizes of the GIF and `.efc`.

Pass `raw` to leave zlib off:

```bash
node encode_gif.mjs smile.gif smile.efc raw
```

Several GIFs at once:

```bash
mkdir -p out
for g in *.gif; do
  node encode_gif.mjs "$g" "out/${g%.gif}.efc"
done
```

Copy `out/` onto the card as `totest`.


## Environment Setup

### Hardware Required

- Any board supported by `esp_board_manager` that has a display and a microSD slot. This README uses ESP32-S3-Korvo-2 v3, board name `esp32_s3_korvo_2_3`, as the example. `gen-bmgr-config -b <board>` brings up that board's LCD, touch, backlight, and SD slot
- A microSD card, FAT, with the clips in `totest`

### Additional Requirements

- At least one `.efc` or `.efc_z` file in that folder. An empty folder still boots: the list says `no .efc* files`
- `CONFIG_ESP_EFC_LVGL_WIDGET` must stay enabled. That is what compiles `lv_efc` and pulls in LVGL 9. It is already set in `sdkconfig` and `sdkconfig.defaults`

## Build and Flash

### Default IDF Branch

This example supports ESP-IDF release/v5.5 and later. By default, it runs on ESP-IDF release/v5.5.

### Configuration

Generate the board-manager configuration for your board, then optionally tune:

```text
ESP-EFC codec > LVGL EFC player widget (lv_efc)
Component config > FAT Filesystem support > Long filename support
```

The widget option is required. It is already set in `sdkconfig.defaults`, together with long file names, PSRAM, and a 12 KB main stack. Long file names matter because clip names are longer than the 8.3 limit. The 240 MHz CPU clock and octal PSRAM come from `sdkconfig.defaults.esp32s3`.

Pick the board name from `esp_board_manager`. The example below is `esp32_s3_korvo_2_3`. `idf.py gen-bmgr-config -b <board>` writes that board's LCD, touch, backlight, and SD-card devices into `components/gen_bmgr_codes`.

The default partition table is the single-app table. If the link step reports that the app image does not fit, switch to the custom table in this directory:

```text
Partition Table > Custom partition table CSV > partitions.csv
```

`partitions.csv` gives the factory app 6 MB, which fits this 16 MB flash.

### Build and Flash

```bash
idf.py set-target esp32s3
idf.py gen-bmgr-config -b esp32_s3_korvo_2_3
idf.py build flash monitor
```

For a different board, pass its name to `-b` and set the target to match that board before generating the config.

For full steps to configure and build an ESP-IDF project, see the [ESP-IDF Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/index.html).

## How to Use the Example

### Example Functionality

After boot the left column is the file list and the right side is the clip. The first file starts on its own.

- **Tap a name** to load that clip and play it from frame 0
- **Play** restarts the current clip at frame 0 and keeps going
- **Pause** holds the frame that is on screen
- **Resume** continues from that frame
- The label is `name` plus the FPS measured over the last half second. The player does not pace itself to a target rate; the number is how fast decode and the panel can go

The clip loops until you pick another file or press Pause.

### References

- `lv_efc_create()` and `lv_efc_set_src()` in `include/lv_efc.h` are the calls this screen is built on. `lv_efc_set_src()` takes a path such as `/sdcard/totest/smile.efc`
- Turn a GIF into an `.efc` in the browser or with Node.js, in [Convert a GIF to EFC](#convert-a-gif-to-efc)


## Troubleshooting

- **`cannot open /sdcard/totest`.** The card did not mount, or the `totest` directory is missing. The slot is the one `esp_board_manager` selected for the board, and the files must sit in that folder, not in the card root
- **`no .efc* files`.** Nothing in the folder has an extension that starts with `.efc`. `.gif` files are not listed
- **`file too large or missing`.** That clip is over 5 MB. Re-encode it, or split the source
- **`open failed`.** The file is not a playable EFC clip, or the widget was built without `CONFIG_ESP_EFC_LVGL_WIDGET`
- **The picture is frozen and the FPS line stays at 0.** Pause is held down, or the first half second has not elapsed yet. Press Resume and wait for the next update
- **Taps do nothing.** Touch init logged a warning. Playback of the first clip does not need touch


## Technical Support and Feedback

Please use the following feedback channels:

- For technical queries, go to the [esp32.com](https://esp32.com/viewforum.php?f=20) forum
- For a feature request or bug report, open an issue on the `esp_efc_codec` project

We will get back to you as soon as possible.
