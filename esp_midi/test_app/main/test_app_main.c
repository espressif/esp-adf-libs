/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "unity.h"
#include "unity_test_runner.h"
#include "unity_test_utils_memory.h"

#define TEST_MEMORY_LEAK_THRESHOLD (500)

void setUp(void)
{
    unity_utils_record_free_mem();
}

void tearDown(void)
{
    unity_utils_evaluate_leaks_direct(TEST_MEMORY_LEAK_THRESHOLD);
}

void app_main()
{
    /***
     *      _____ _____ ____ _____   __   __   _   ______    _ 
     *     |_   _| ____/ ___|_   _| |  \_/  | | | |  ___ \  | |
     *       | | |  _| \___ \ | |   | |\_/| | | | | |   \ | | |
     *       | | | |___ ___) || |   | |   | | | | | |___/ | | |
     *       |_| |_____|____/ |_|   |_|   |_| |_| |______/  |_|
     *
     */

    printf(" _____ _____ ____ _____   __   __   _   ______    _ \n");
    printf("|_   _| ____/ ___|_   _| |  \\_/  | | | |  ___ \\  | |\n");
    printf("  | | |  _| \\___ \\ | |   | |\\_/| | | | | |   \\ | | |\n");
    printf("  | | | |___ ___) || |   | |   | | | | | |___/ | | |\n");
    printf("  |_| |_____|____/ |_|   |_|   |_| |_| |______/  |_|\n");

    unity_run_menu();
}
