#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

/* Forward declaration of the vulnerable function from the actual source file */
extern void load_midi_files(const char *filename[], int count);

START_TEST(test_buffer_reads_never_exceed_declared_length)
{
    /* Invariant: Buffer reads never exceed the declared length */
    const char *payloads[] = {
        "valid.mid",                            /* Valid input */
        "A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789.mid",  /* 100 chars (exact boundary) */
        "A123456789B123456789C123456789D123456789E123456789F123456789G123456789H123456789I123456789J123456789K.mid", /* 101 chars (overflow by 1) */
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA.mid",  /* 100 chars (alternate) */
        "X"  /* Minimal valid */
    };
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);

    for (int i = 0; i < num_payloads; i++) {
        /* Create a temporary file to avoid fopen failure */
        FILE *tmp = fopen(payloads[i], "w");
        if (tmp) {
            fclose(tmp);
        }

        const char *filename_array[] = {payloads[i]};
        
        /* This should not crash or cause buffer overflow */
        load_midi_files(filename_array, 1);
        
        /* Clean up */
        unlink(payloads[i]);
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