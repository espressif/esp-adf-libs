# SPDX-FileCopyrightText: 2025 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: LicenseRef-Espressif-Modified-MIT

import pytest
from pytest_embedded import Dut

def test_esp_system(dut: Dut) -> None:
    dut.run_all_single_board_cases()
