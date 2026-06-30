#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

// Include the actual production header
#include "esp_hls_stream/src/hls_io.h"

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    // Invariant: Buffer reads never exceed the declared length
    const char *payloads[] = {
        "normal",                    // Valid input
        "A" * 255,                   // Boundary: 255 chars (assuming 256 buffer)
        "EXPLOIT" * 100,            // Oversized: 700 chars
        "\x00\x01\x02\x03\x04",     // Binary data
        "A" * 1024                  // Large overflow
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        // Create a test file with the payload
        FILE *test_file = tmpfile();
        if (!test_file) {
            ck_abort_msg("Failed to create temp file");
        }
        fwrite(payloads[i], strlen(payloads[i]), 1, test_file);
        rewind(test_file);
        
        // Test with a fixed-size buffer
        char buffer[256] = {0};
        size_t bytes_read = hls_io_read_chunk(test_file, buffer, sizeof(buffer) - 1);
        
        // Security invariant: bytes_read must not exceed buffer size
        ck_assert_msg(bytes_read <= sizeof(buffer) - 1, 
                     "Buffer read exceeded declared length. Read %zu bytes into %zu buffer", 
                     bytes_read, sizeof(buffer) - 1);
        
        // Additional check: buffer must be null-terminated if bytes_read > 0
        if (bytes_read > 0) {
            ck_assert_msg(buffer[bytes_read] == '\0', 
                         "Buffer not properly null-terminated after read");
        }
        
        fclose(test_file);
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("Core");

    tcase_add_test(tc_core, test_buffer_reads_never_exceed_declared_length);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}