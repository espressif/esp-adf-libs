# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: Apache-2.0

import pytest
from pytest_embedded import Dut

def test_esp_media_protocols(dut: Dut) -> None:
    dut.run_all_single_board_cases()
