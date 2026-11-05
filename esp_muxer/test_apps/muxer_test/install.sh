rm -rf sdkconfig sdkconfig.old managed_components dependencies.lock build
idf.py gen-bmgr-config -b esp32_s3_korvo_2_3
