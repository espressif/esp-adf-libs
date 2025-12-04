rm -rf sdkconfig sdkconfig.old managed_components dependencies.lock build
idf.py set-target esp32s3
export IDF_EXTRA_ACTIONS_PATH=managed_components/espressif__esp_board_manager
idf.py gen-bmgr-config -b esp32_s3_korvo2_v3