/**
 * @file print_utils.c
 * @brief Print formatting helpers for tests, benchmarks, and CLI output.
 * @ingroup print_utils
 */

#include <utils.h>

#if defined(__clang__)
#define IZ_DIAG_PUSH _Pragma("clang diagnostic push")
#define IZ_DIAG_POP _Pragma("clang diagnostic pop")
#define IZ_DIAG_IGNORE_FORMAT_NONLITERAL _Pragma("clang diagnostic ignored \"-Wformat-nonliteral\"")
#elif defined(__GNUC__)
#define IZ_DIAG_PUSH _Pragma("GCC diagnostic push")
#define IZ_DIAG_POP _Pragma("GCC diagnostic pop")
#define IZ_DIAG_IGNORE_FORMAT_NONLITERAL _Pragma("GCC diagnostic ignored \"-Wformat-nonliteral\"")
#else
#define IZ_DIAG_PUSH
#define IZ_DIAG_POP
#define IZ_DIAG_IGNORE_FORMAT_NONLITERAL
#endif

/** @ingroup print_utils
 *  @brief Print a repeated-character horizontal line.
 *  @param length Number of characters to print.
 *  @param fill_char Character to repeat (`'-'` is used when zero).
 */
void print_line(int length, char fill_char)
{
    if (fill_char == '\0')
        fill_char = '-';

    for (int i = 0; i < length; ++i)
        printf("%c", fill_char);
    printf("\n");
}

/** @ingroup print_utils
 *  @brief Print centered text within a line of specified length.
 *  @param text Text to center.
 *  @param line_length Total length of the line.
 *  @param fill_char Character to use for padding (`' '` is used when zero).
 */
void print_centered_text(const char *text, int line_length, char fill_char)
{
    if (text == NULL)
        text = "";

    int text_length = (int)strlen(text);
    if (text_length >= line_length)
    {
        printf("%s\n", text);
        return;
    }

    int left_padding = (line_length - text_length) / 2;
    int right_padding = line_length - text_length - left_padding;

    for (int i = 0; i < left_padding; ++i)
        printf("%c", fill_char);
    printf("%s", text);
    for (int i = 0; i < right_padding; ++i)
        printf("%c", fill_char);
    printf("\n");
}

/** @ingroup print_utils
 * @brief Print the SHA-256 hash in hexadecimal format.
 *
 * @param hash Pointer to the SHA-256 hash array.
 */
void print_sha256_hash(const unsigned char *hash)
{
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        printf("%02x", hash[i]);

    printf("\n");
}

/** @ingroup print_utils
 * @brief Print module-level banner for test output.
 * @param module_name Display name of the tested module.
 */
void print_test_module_header(char *module_name)
{
    print_line(60, '*');
    printf("* %s MODULE TEST SUITE\n", module_name);
    print_line(60, '*');
}

/** @ingroup print_utils
 * @brief Print standard test-table column headers.
 */
void print_test_table_header(void)
{
    print_line(92, '-');
    printf("[%s] %-30s %s %-66s\n", "ID", "Unit Name", "Result", "Details");
    print_line(92, '-');
}

/** @ingroup print_utils
 * @brief Print a single test unit result row with formatted details.
 *
 * @param result 1 for pass, 0 for fail.
 * @param test_id Monotonic case index.
 * @param unit_name Short test unit label.
 * @param format `printf`-style detail format string.
 * @param ... Arguments for the detail format string.
 */
void print_test_module_result(int result, int test_id, const char *unit_name, const char *format, ...)
{
    va_list args;
    va_start(args, format);

    char message[1024];
    IZ_DIAG_PUSH;
    IZ_DIAG_IGNORE_FORMAT_NONLITERAL;
    vsnprintf(message, sizeof(message), format, args);
    IZ_DIAG_POP;
    va_end(args);

    if (result)
    {
        printf("[%02d] %-30s [PASS] %s\n", test_id, unit_name, message);
    }
    else
    {
        printf("[%02d] %-30s [FAIL] %s\n", test_id, unit_name, message);
    }
}

/** @ingroup print_utils
 * @brief Print a summary of test results for a module.
 *
 * @param module_name Name of the tested module.
 * @param passed Number of passing tests.
 * @param failed Number of failing tests.
 * @param verbose If non-zero, prints additional hints about the results.
 */
void print_test_summary(char *module_name, int passed, int failed, int verbose)
{
    (void)verbose;

    print_line(60, '*');
    printf("Results Summary for %s\n", module_name);
    print_line(60, '-');
    printf("%-32s: %d\n", "Total Tests", passed + failed);
    printf("%-32s: %d\n", "Passed", passed);
    printf("%-32s: %d\n", "Failed", failed);
    print_line(60, '-');
    if (failed == 0)
    {
        printf("[SUCCESS] ALL %s TESTS PASSED!\n", module_name);
    }
    else
    {
        printf("[FAILURE] SOME %s TESTS FAILED :\\\n", module_name);
    }
    print_line(60, '*');
}

void print_test_fn_header(char *fn_name)
{
    char header[64];
    snprintf(header, sizeof(header), " Testing %s ", fn_name);

    print_line(60, '*');
    print_centered_text(header, 60, '=');
    print_line(60, '*');
    printf("\n");
}
