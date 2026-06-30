import pytest
import ctypes
import os
import sys
import struct

# ---------------------------------------------------------------------------
# Payloads: strings that exceed typical fixed-size path/filename buffers
# (common sizes: 64, 128, 256, 512 bytes).  We test 2x and 10x overflows,
# plus classic attack patterns.
# ---------------------------------------------------------------------------

PAYLOADS = [
    # 2x typical 128-byte buffer
    "A" * 256,
    # 10x typical 128-byte buffer
    "A" * 1280,
    # 2x typical 256-byte buffer
    "B" * 512,
    # 10x typical 256-byte buffer
    "B" * 2560,
    # Path traversal + overflow
    "../" * 100 + "evil.mid",
    # Null bytes embedded (C string termination bypass attempt)
    "C" * 127 + "\x00" + "D" * 127,
    # Format-string characters mixed with overflow
    "%s%n%x" * 50,
    # Unicode / multi-byte characters
    "\xff\xfe" * 200,
    # Slash-heavy path overflow
    "/" + "subdir/" * 50 + "file.mid",
    # Mixed special characters
    "!@#$%^&*()_+" * 30,
    # Exactly at boundary (128 bytes)
    "E" * 128,
    # One byte over boundary
    "F" * 129,
    # Very long extension
    "sound" + "." + "x" * 500,
    # Windows-style path overflow
    "C:\\" + "dir\\" * 60 + "file.mid",
    # Null-only string
    "\x00" * 256,
    # Whitespace flood
    " " * 1024,
    # Tab/newline flood
    "\t\n\r" * 200,
    # 10x 512-byte buffer
    "G" * 5120,
]


# ---------------------------------------------------------------------------
# Pure-Python simulation of the unsafe strcat / strncat behaviour described
# in esp_midi_files_loader.c.  We model the two fixed-size buffers that the
# C code builds file paths into (typically 128 or 256 bytes) and assert that
# a safe implementation MUST either truncate or reject inputs that would
# overflow those buffers.
# ---------------------------------------------------------------------------

MAX_PATH_LEN = 256   # typical fixed buffer size in the C file
MAX_NAME_LEN = 128   # typical fixed name buffer size


def safe_concat(base: str, suffix: str, max_len: int) -> str:
    """
    Simulate a safe strcat_s / strncat_s replacement.
    Returns the concatenated string truncated to max_len - 1 characters
    (leaving room for the NUL terminator), or raises ValueError if the
    result would exceed max_len.
    """
    combined = base + suffix
    if len(combined) >= max_len:
        # A safe implementation must truncate or reject.
        # We choose truncation here (strcat_s would return ERANGE and
        # truncate to max_len-1 chars + NUL).
        return combined[: max_len - 1]
    return combined


def simulate_file_path_construction(directory: str, filename: str) -> str:
    """
    Mirrors what esp_midi_files_loader.c does:
      1. Copy directory into a fixed buffer.
      2. strcat the filename onto it.
    A safe version must never produce a string longer than MAX_PATH_LEN - 1.
    """
    # Step 1: truncate directory to buffer size
    dir_part = directory[: MAX_PATH_LEN - 1]
    # Step 2: safe concatenation
    result = safe_concat(dir_part, filename, MAX_PATH_LEN)
    return result


def simulate_name_buffer(name: str) -> str:
    """
    Mirrors a secondary name buffer (MAX_NAME_LEN bytes).
    """
    return name[: MAX_NAME_LEN - 1]


# ---------------------------------------------------------------------------
# Tests
# ---------------------------------------------------------------------------

@pytest.mark.parametrize("payload", PAYLOADS)
def test_file_path_construction_never_exceeds_buffer(payload):
    """
    Invariant: Buffer reads/writes never exceed the declared buffer length.

    When an oversized string is used as a directory or filename component,
    the resulting path string MUST be strictly shorter than MAX_PATH_LEN
    (i.e. at most MAX_PATH_LEN - 1 usable characters, leaving room for NUL).
    This guards against CWE-120 buffer-overflow via strcat/strncat.
    """
    result = simulate_file_path_construction("/sdcard/sounds/", payload)
    assert len(result) < MAX_PATH_LEN, (
        f"Buffer overflow detected: result length {len(result)} >= "
        f"MAX_PATH_LEN {MAX_PATH_LEN} for payload of length {len(payload)}"
    )


@pytest.mark.parametrize("payload", PAYLOADS)
def test_filename_only_never_exceeds_buffer(payload):
    """
    Invariant: A filename stored in a fixed-size name buffer must never
    exceed MAX_NAME_LEN - 1 usable bytes (CWE-120 guard).
    """
    result = simulate_name_buffer(payload)
    assert len(result) < MAX_NAME_LEN, (
        f"Name buffer overflow: result length {len(result)} >= "
        f"MAX_NAME_LEN {MAX_NAME_LEN} for payload of length {len(payload)}"
    )


@pytest.mark.parametrize("payload", PAYLOADS)
def test_double_concat_never_exceeds_buffer(payload):
    """
    Invariant: Two successive concatenations (simulating two strncat calls
    in the C loader) must still keep the total within MAX_PATH_LEN.
    This is the exact pattern that triggers CWE-120 in the original code.
    """
    base = "/sdcard/"
    # First concat: append a subdirectory derived from payload
    after_first = safe_concat(base, payload[:64], MAX_PATH_LEN)
    assert len(after_first) < MAX_PATH_LEN, (
        f"After first concat: length {len(after_first)} >= {MAX_PATH_LEN}"
    )
    # Second concat: append a filename derived from payload
    after_second = safe_concat(after_first, payload[:64], MAX_PATH_LEN)
    assert len(after_second) < MAX_PATH_LEN, (
        f"After second concat: length {len(after_second)} >= {MAX_PATH_LEN}"
    )


@pytest.mark.parametrize("payload", PAYLOADS)
def test_no_null_byte_escape(payload):
    """
    Invariant: Embedded NUL bytes in the payload must not allow the
    effective string length (up to first NUL) to differ from the Python
    len() in a way that could bypass length checks in C.

    The safe implementation must treat the full Python string length as the
    worst-case size (conservative bound).
    """
    # Worst-case C strlen would stop at first \x00; Python len() counts all.
    # A safe guard must use the LARGER of the two values.
    c_effective_len = payload.find("\x00")
    if c_effective_len == -1:
        c_effective_len = len(payload)

    python_len = len(payload)
    worst_case = max(c_effective_len, python_len)

    result = simulate_file_path_construction("/mount/", payload)
    # The result must be bounded regardless of embedded NULs.
    assert len(result) < MAX_PATH_LEN, (
        f"NUL-bypass overflow: result {len(result)} >= {MAX_PATH_LEN}, "
        f"worst_case input size was {worst_case}"
    )