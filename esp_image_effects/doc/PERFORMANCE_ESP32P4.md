# Test on esp32p4

## IMAGE COLOR CONVERT

| Width | Height | Input format                     | Output format                    | OPT Frame Per Second(fps) | C Frame Per Second(fps) |
|-------|--------|----------------------------------|----------------------------------|---------------------------|-------------------------|
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_I420           | ESP_IMG_PIXEL_FMT_I420           | 34.8958                   | 26.9247                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_I420           | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | 30.1942                   | 19.815                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_I420           | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 18.1374                   | 4.5326                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_I420           | ESP_IMG_PIXEL_FMT_RGB/BGR888     | 17.5294                   | 4.4743                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 18.8097                   | 4.7104                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | ESP_IMG_PIXEL_FMT_RGB/BGR888     | 17.9523                   | 5.6457                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_I420           | 20.1829                   | 2.4178                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | 20.4147                   | 2.4455                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 26.3158                   | 20.2532                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | 18.3592                   | 2.2755                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_RGB/BGR888     | 17.9272                   | 7.6518                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_YUV_PLANNER    | 13.8708                   | 2.7918                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | ESP_IMG_PIXEL_FMT_YUV_PACKET     | 14.7262                   | 3.0223                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | ESP_IMG_PIXEL_FMT_I420           | 26.936                    | 12.8928                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | 35.8744                   | 13.256                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 18.8679                   | 3.805                   |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | ESP_IMG_PIXEL_FMT_RGB/BGR888     | 15.1515                   | 4.6893                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_I420           | 17.4292                   | 2.6161                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | 17.094                    | 2.6729                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 18.5185                   | 7.3733                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_YUYV/UYVY      | 15.6863                   | 2.5707                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_YUV_PLANNER    | 11.7302                   | 3.0361                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR888     | ESP_IMG_PIXEL_FMT_YUV_PACKET     | 15.4143                   | 3.1008                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUV_PACKET     | ESP_IMG_PIXEL_FMT_I420           | 21.2766                   | 10.8844                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUV_PACKET     | ESP_IMG_PIXEL_FMT_O_UYY_E_VYY    | 25.9109                   | 11.7045                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUV_PACKET     | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 14.7194                   | 2.9701                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_YUV_PACKET     | ESP_IMG_PIXEL_FMT_RGB/BGR888     | 13.0826                   | 3.043                   |


## IMAGE ROTATE

| Width | Height | Format                        | degree | OPT Frame Per Second(fps) | C Frame Per Second(fps) |
|-------|--------|-------------------------------|--------|---------------------------|-------------------------|
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 0      |                           | 20.9249                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 90     |                           | 14.4444                 |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 180    |                           | 4.2105                  |
| 1920  | 1080   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 270    |                           | 15.3181                 |
| 1920  | 1088   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 0      | 20.7632                   |                         |
| 1920  | 1088   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 90     | 19.4413                   |                         |
| 1920  | 1088   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 180    | 23.679                    |                         |
| 1920  | 1088   | ESP_IMG_PIXEL_FMT_RGB/BGR565_LE/BE | 270    | 19.4525                   |                         |

## IMAGE SCALE

| Input Width | Input Height | Format                      | Scale Width | Scale height | Filter type                              | C Frame Per Second(fps) |
|-------------|--------------|-----------------------------|-------------|--------------|------------------------------------------|-------------------------|
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 240         | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 44.3213                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 240         | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 22.2531                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 480         | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 26.0586                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 480         | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 13.0719                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 720         | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 18.0383                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 720         | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 9.06                    |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 960         | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 14.1218                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 960         | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 7.1365                  |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1200        | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 11.6959                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1200        | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 5.848                   |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1440        | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 9.6502                  |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1440        | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 4.7933                  |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1680        | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 8.8154                  |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1680        | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 4.374                   |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1920        | 540          | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 8.0808                  |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1920        | 1080         | ESP_IMG_SCALE_FILTER_TYPE_DOWN_RESAMPLE | 4.0414                  |

## IMAGE CROP
note: The starting coordinate is (0, 0)

| Input Width | Input Height | Format                      | Clip Width | Clip height | C Frame Per Second(fps) |
|-------------|--------------|-----------------------------|------------|-------------|-------------------------|
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 240        | 540         | 306.2201                |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 240        | 1080        | 158.6121                |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 480        | 540         | 161.8205                |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 480        | 1080        | 83.1709                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 720        | 540         | 110.4400                |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 720        | 1080        | 56.3628                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 960        | 540         | 85.0498                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 960        | 1080        | 43.1412                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1200       | 540         | 68.4126                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1200       | 1080        | 34.5852                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1440       | 540         | 57.2707                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1440       | 1080        | 28.9462                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1680       | 540         | 48.8550                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1680       | 1080        | 24.6438                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1920       | 540         | 42.5249                 |
| 1920        | 1080         | ESP_IMG_PIXEL_FMT_RGB565_LE | 1920       | 1080        | 21.2625                 |
