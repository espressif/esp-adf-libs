# SPDX-FileCopyrightText: 2026 Espressif Systems (Shanghai) CO., LTD
# SPDX-License-Identifier: Apache-2.0

"""Hardware CI test for esp_efc_codec embedded-asset performance bench."""

from __future__ import annotations

import json
import os
import re
from pathlib import Path
from typing import Any

import pytest
from pytest_embedded_idf.dut import IdfDut

CASE_TIMEOUT_S = 180

ROUND_RE = re.compile(
    r'--- round (?P<round>\d+): (?P<frames>\d+) frames (?P<width>\d+)x(?P<height>\d+) ---'
)
PLAIN_RE = re.compile(
    r'plain: open (?P<open_us>-?\d+) us\s+decode (?P<decode_us_pf>[-+]?\d+(?:\.\d+)?) us/frame\s+'
    r'ctx heap (?P<ctx_heap>\d+) B'
)
Z_RE = re.compile(
    r'z\s*: open (?P<open_us>-?\d+) us\s+decode (?P<decode_us_pf>[-+]?\d+(?:\.\d+)?) us/frame\s+'
    r'ctx heap (?P<ctx_heap>\d+) B'
)
DROP_RE = re.compile(
    r'drop : decode (?P<decode_delta_pct>[-+]?\d+(?:\.\d+)?)% per frame \(every playthrough\)\s+'
    r'\(size (?P<size_delta_pct>[-+]?\d+(?:\.\d+)?)%\)'
)
MEM_RE = re.compile(
    r'mem\s+: z streaming work-set (?P<z_workset>\d+) B vs inflate-all (?P<inflate_all>\d+) B\s+'
    r'-> saves (?P<saves>-?\d+) B'
)
VERIFY_RE = re.compile(
    r'verify: plain csum=(?P<plain_csum>0x[0-9a-fA-F]+)  z csum=(?P<z_csum>0x[0-9a-fA-F]+) -> (?P<status>.+)'
)
ASSETS_RE = re.compile(
    r'assets: plain=(?P<plain_sz>\d+) B  z=(?P<z_sz>\d+) B \((?P<ratio>\d+(?:\.\d+)?)x smaller\)\s+'
    r'free heap (?P<free_heap>\d+) B'
)
CONFIG_RE = re.compile(r'config: 4x4_only=(?P<only4x4>\d+)\s+SIMD=(?P<simd>\S+)')
PREVIEW_SUFFIX = '_preview'


def real_target_name(build_target: str) -> str:
    """Map CI matrix entry (e.g. esp32s31_preview) to the real chip name."""
    if build_target.endswith(PREVIEW_SUFFIX):
        return build_target[: -len(PREVIEW_SUFFIX)]
    return build_target


def _decode(value: Any) -> str:
    if isinstance(value, bytes):
        return value.decode('utf-8', errors='replace')
    return str(value)


def _expect_group(dut: IdfDut, pattern: re.Pattern[str], timeout: int) -> re.Match[str]:
    match = dut.expect(pattern, timeout=timeout)
    assert match is not None
    return match


def _collect_metrics(dut: IdfDut, target: str) -> dict[str, Any]:
    dut.expect_exact('===== EFC codec perf test (plain vs DEFLATE `z`) =====', timeout=60)

    config_match = _expect_group(dut, CONFIG_RE, timeout=30)
    assets_match = _expect_group(dut, ASSETS_RE, timeout=30)
    round_match = _expect_group(dut, ROUND_RE, timeout=CASE_TIMEOUT_S)
    plain_match = _expect_group(dut, PLAIN_RE, timeout=30)
    z_match = _expect_group(dut, Z_RE, timeout=30)
    drop_match = _expect_group(dut, DROP_RE, timeout=30)
    mem_match = _expect_group(dut, MEM_RE, timeout=30)
    verify_match = _expect_group(dut, VERIFY_RE, timeout=30)

    status = _decode(verify_match.group('status')).strip()
    assert 'MATCH' in status, f'checksum mismatch on {target}: {status}'

    metrics: dict[str, Any] = {
        'target': target,
        'config': {
            'decode_4x4_only': int(_decode(config_match.group('only4x4'))),
            'simd': _decode(config_match.group('simd')),
        },
        'assets': {
            'plain_bytes': int(_decode(assets_match.group('plain_sz'))),
            'z_bytes': int(_decode(assets_match.group('z_sz'))),
            'size_ratio': float(_decode(assets_match.group('ratio'))),
            'free_heap_bytes': int(_decode(assets_match.group('free_heap'))),
        },
        'round': int(_decode(round_match.group('round'))),
        'frames': int(_decode(round_match.group('frames'))),
        'width': int(_decode(round_match.group('width'))),
        'height': int(_decode(round_match.group('height'))),
        'plain': {
            'open_us': int(_decode(plain_match.group('open_us'))),
            'decode_us_per_frame': float(_decode(plain_match.group('decode_us_pf'))),
            'ctx_heap_bytes': int(_decode(plain_match.group('ctx_heap'))),
        },
        'z': {
            'open_us': int(_decode(z_match.group('open_us'))),
            'decode_us_per_frame': float(_decode(z_match.group('decode_us_pf'))),
            'ctx_heap_bytes': int(_decode(z_match.group('ctx_heap'))),
        },
        'drop': {
            'decode_delta_pct': float(_decode(drop_match.group('decode_delta_pct'))),
            'size_delta_pct': float(_decode(drop_match.group('size_delta_pct'))),
        },
        'mem': {
            'z_streaming_workset_bytes': int(_decode(mem_match.group('z_workset'))),
            'inflate_all_bytes': int(_decode(mem_match.group('inflate_all'))),
            'saves_bytes': int(_decode(mem_match.group('saves'))),
        },
        'verify': {
            'plain_csum': _decode(verify_match.group('plain_csum')),
            'z_csum': _decode(verify_match.group('z_csum')),
            'status': status,
            'match': True,
        },
    }
    return metrics


def _write_performance_json(metrics: dict[str, Any], target: str) -> Path:
    out_dir = Path(os.environ.get('CI_PROJECT_DIR', Path.cwd()))
    out_path = out_dir / f'performance-{target}.json'
    out_path.write_text(json.dumps(metrics, indent=2, sort_keys=True) + '\n', encoding='utf-8')
    return out_path


def _run_case(dut: IdfDut, build_target: str) -> None:
    target = real_target_name(build_target)
    metrics = _collect_metrics(dut, target)
    metrics['build_target'] = build_target
    out_path = _write_performance_json(metrics, target)
    print(f'[efc-ci] wrote {out_path}')
    print(
        f"[efc-ci] build_target={build_target} target={target}: "
        f"plain={metrics['plain']['decode_us_per_frame']:.1f} us/f "
        f"z={metrics['z']['decode_us_per_frame']:.1f} us/f verify={metrics['verify']['status']}"
    )


@pytest.mark.esp32s3
def test_efc_codec_esp32s3(dut: IdfDut) -> None:
    _run_case(dut, os.environ.get('BUILD_TARGET', 'esp32s3'))


@pytest.mark.esp32p4
def test_efc_codec_esp32p4(dut: IdfDut) -> None:
    _run_case(dut, os.environ.get('BUILD_TARGET', 'esp32p4'))


@pytest.mark.esp32s31
def test_efc_codec_esp32s31(dut: IdfDut) -> None:
    _run_case(dut, os.environ.get('BUILD_TARGET', 'esp32s31_preview'))
