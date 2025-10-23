/*
 * SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
 * SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT
 *
 * See LICENSE file for details.
 */

#include "media_lib_err.h"
#include "media_lib_adapter.h"

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
    /***    _____ _____ ____ _____   __  __ _____ ____ ___    _      _     ___ ____    ____    _    _
     *     |_   _| ____/ ___|_   _| |  \/  | ____|  _ \_ _|  / \    | |   |_ _| __ )  / ___|  / \  | |
     *       | | |  _| \___ \ | |   | |\/| |  _| | | | | |  / _ \   | |    | ||  _ \  \___ \ / _ \ | |
     *       | | | |___ ___) || |   | |  | | |___| |_| | | / ___ \  | |___ | || |_) |  ___) / ___ \| |___
     *       |_| |_____|____/ |_|   |_|  |_|_____|____/___/_/   \_\ |_____|___|____/  |____/_/   \_\_____|
     *
     */
    printf(" _____ _____ ____ _____   __  __ _____ ____ ___    _      _     ___ ____    ____    _    _ \n");
    printf("|_   _| ____/ ___|_   _| |  \\/  | ____|  _ \\_ _|  / \\    | |   |_ _| __ )  / ___|  / \\  | | \n");
    printf("  | | |  _| \\___ \\ | |   | |\\/| |  _| | | | | |  / _ \\   | |    | ||  _ \\  \\___ \\ / _ \\ | | \n");
    printf("  | | | |___ ___) || |   | |  | | |___| |_| | | / ___ \\  | |___ | || |_) |  ___) / ___ \\| |___ \n");
    printf("  |_| |_____|____/ |_|   |_|  |_|_____|____/___/_/   \\_\\ |_____|___|____/  |____/_/   \\_\\_____| \n");

    esp_err_t ret = media_lib_add_default_adapter();
    TEST_ASSERT_EQUAL(ESP_OK, ret);

    unity_run_menu();
}
