# SPDX-FileCopyrightText: 2022-2025 Espressif Systems (Shanghai) CO LTD
# SPDX-License-Identifier: CC0-1.0

import pytest
from pytest_embedded import Dut

def test_esp_system(dut: Dut) -> None:
    dut.run_all_single_board_cases()
