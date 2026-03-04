#include <test_api.h>
#include <cli.h>

#include <sys/stat.h>
#include <unistd.h>

#define CLI_MAX_ARGS 16

typedef struct
{
    int exit_code;
    char *stdout_text;
    char *stderr_text;
} CLI_CAPTURE;

typedef struct
{
    const char *name;
    int argc;
    const char *argv[CLI_MAX_ARGS];
    int expected_exit;
    const char *stdout_contains;
    const char *stderr_contains;
    const char *stdout_not_contains;
    const char *file_to_check;
    int expect_file_nonempty;
} CLI_TEST_CASE;

static char *read_stream_all(FILE *stream)
{
    if (stream == NULL)
        return NULL;

    if (fflush(stream) != 0)
        return NULL;

    if (fseek(stream, 0, SEEK_END) != 0)
        return NULL;

    long size = ftell(stream);
    if (size < 0)
        return NULL;

    if (fseek(stream, 0, SEEK_SET) != 0)
        return NULL;

    char *buf = malloc((size_t)size + 1);
    if (buf == NULL)
        return NULL;

    size_t nread = fread(buf, 1, (size_t)size, stream);
    buf[nread] = '\0';
    return buf;
}

static int capture_cli_run(int argc, const char *const *argv_const, CLI_CAPTURE *capture)
{
    if (capture == NULL)
        return 0;

    capture->exit_code = EXIT_FAILURE;
    capture->stdout_text = NULL;
    capture->stderr_text = NULL;

    FILE *tmp_out = tmpfile();
    FILE *tmp_err = tmpfile();
    if (tmp_out == NULL || tmp_err == NULL)
    {
        if (tmp_out)
            fclose(tmp_out);
        if (tmp_err)
            fclose(tmp_err);
        return 0;
    }

    fflush(NULL);

    int saved_stdout = dup(STDOUT_FILENO);
    int saved_stderr = dup(STDERR_FILENO);
    if (saved_stdout < 0 || saved_stderr < 0)
    {
        if (saved_stdout >= 0)
            close(saved_stdout);
        if (saved_stderr >= 0)
            close(saved_stderr);
        fclose(tmp_out);
        fclose(tmp_err);
        return 0;
    }

    if (dup2(fileno(tmp_out), STDOUT_FILENO) < 0 || dup2(fileno(tmp_err), STDERR_FILENO) < 0)
    {
        close(saved_stdout);
        close(saved_stderr);
        fclose(tmp_out);
        fclose(tmp_err);
        return 0;
    }

    char *argv[CLI_MAX_ARGS] = {0};
    for (int i = 0; i < argc; ++i)
        argv[i] = (char *)argv_const[i];

    capture->exit_code = cli_run(argc, argv);

    fflush(stdout);
    fflush(stderr);

    fflush(NULL);
    dup2(saved_stdout, STDOUT_FILENO);
    dup2(saved_stderr, STDERR_FILENO);
    close(saved_stdout);
    close(saved_stderr);

    capture->stdout_text = read_stream_all(tmp_out);
    capture->stderr_text = read_stream_all(tmp_err);

    fclose(tmp_out);
    fclose(tmp_err);

    if (capture->stdout_text == NULL || capture->stderr_text == NULL)
    {
        free(capture->stdout_text);
        free(capture->stderr_text);
        capture->stdout_text = NULL;
        capture->stderr_text = NULL;
        return 0;
    }

    return 1;
}

static int file_exists_and_nonempty(const char *path)
{
    if (path == NULL)
        return 0;

    struct stat st;
    if (stat(path, &st) != 0)
        return 0;

    return st.st_size > 0;
}

static int run_case(const CLI_TEST_CASE *tc, int verbose)
{
    if (tc == NULL)
        return 0;

    if (tc->file_to_check)
        remove(tc->file_to_check);

    CLI_CAPTURE cap;
    if (!capture_cli_run(tc->argc, tc->argv, &cap))
    {
        if (verbose)
            printf("[FAIL   ] %-34s capture failed\n", tc->name);
        return 0;
    }

    int ok = 1;
    if (cap.exit_code != tc->expected_exit)
        ok = 0;
    if (tc->stdout_contains && strstr(cap.stdout_text, tc->stdout_contains) == NULL)
        ok = 0;
    if (tc->stderr_contains && strstr(cap.stderr_text, tc->stderr_contains) == NULL)
        ok = 0;
    if (tc->stdout_not_contains && strstr(cap.stdout_text, tc->stdout_not_contains) != NULL)
        ok = 0;
    if (tc->expect_file_nonempty && !file_exists_and_nonempty(tc->file_to_check))
        ok = 0;

    if (verbose)
    {
        printf("[%-7s] %-34s expected=%d got=%d\n",
               ok ? "PASS" : "FAIL",
               tc->name,
               tc->expected_exit,
               cap.exit_code);
        if (!ok)
        {
            printf("  stdout: %s\n", cap.stdout_text);
            printf("  stderr: %s\n", cap.stderr_text);
        }
    }

    free(cap.stdout_text);
    free(cap.stderr_text);
    return ok;
}

int TEST_CLI(int verbose)
{
    print_test_fn_header("CLI COMMAND TEST SUITE");

    create_dir(DIR_output);
    const char *stream_file = DIR_output "/cli_stream_test.txt";
    const char *bench_file = DIR_output "/cli_bench_test.csv";

    CLI_TEST_CASE cases[] = {
        {.name = "general help", .argc = 1, .argv = {"izprime"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "iZprime CLI"},
        {.name = "help stream_primes", .argc = 3, .argv = {"izprime", "help", "stream_primes"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Usage: izprime stream_primes"},
        {.name = "unknown command", .argc = 2, .argv = {"izprime", "does_not_exist"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Unknown command: does_not_exist"},

        {.name = "stream print", .argc = 5, .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--print"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Prime count in [0, 200] = 46"},
        {.name = "stream alias", .argc = 5, .argv = {"izprime", "sieve", "--range", "[0, 200]", "--print"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Prime count in [0, 200] = 46"},
        {.name = "stream default file", .argc = 4, .argv = {"izprime", "stream_primes", "--range", "[0, 200]"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Streamed primes to:"},
        {.name = "stream explicit file", .argc = 6, .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--stream-to", stream_file}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Streamed primes to:", .file_to_check = stream_file, .expect_file_nonempty = 1},
        {.name = "stream missing range", .argc = 2, .argv = {"izprime", "stream_primes"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Missing required option: --range"},
        {.name = "stream invalid range", .argc = 4, .argv = {"izprime", "stream_primes", "--range", "[10]"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Invalid --range value"},
        {.name = "stream conflicting outputs", .argc = 7, .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--print", "--stream-to", stream_file}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Use either --print or --stream-to"},
        {.name = "stream gaps conflict", .argc = 7, .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--print-gaps", "--stream-to", stream_file}, .expected_exit = EXIT_FAILURE, .stderr_contains = "--print-gaps cannot be combined"},

        {.name = "count primes", .argc = 6, .argv = {"izprime", "count_primes", "--range", "[0, 200]", "--cores", "1"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Prime count in [0, 200] = 46"},
        {.name = "count alias", .argc = 6, .argv = {"izprime", "count", "--range", "[0, 200]", "--cores", "1"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Prime count in [0, 200] = 46"},
        {.name = "count invalid cores", .argc = 6, .argv = {"izprime", "count_primes", "--range", "[0, 200]", "--cores", "0"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Invalid --cores value"},
        {.name = "count precondition", .argc = 4, .argv = {"izprime", "count_primes", "--range", "[0, 99]"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Range size must be > 100"},

        {.name = "next prime", .argc = 4, .argv = {"izprime", "next_prime", "--n", "10^2+1"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Next prime after 101 is 103"},
        {.name = "prev prime", .argc = 4, .argv = {"izprime", "prev_prime", "--n", "10^2+1"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Previous prime before 101 is 97"},
        {.name = "prev alias", .argc = 4, .argv = {"izprime", "prev", "--n", "10^2+1"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Previous prime before 101 is 97"},

        {.name = "is prime yes", .argc = 6, .argv = {"izprime", "is_prime", "--n", "97", "--rounds", "5"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "97 is prime"},
        {.name = "is prime no", .argc = 6, .argv = {"izprime", "is_prime", "--n", "99", "--rounds", "5"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "99 is composite"},
        {.name = "is prime invalid rounds", .argc = 6, .argv = {"izprime", "is_prime", "--n", "97", "--rounds", "0"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Invalid --rounds value"},

        {.name = "doctor", .argc = 2, .argv = {"izprime", "doctor"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "iZprime doctor"},

        {.name = "test cmd", .argc = 4, .argv = {"izprime", "test", "--limit", "1000"}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "All model consistency tests passed"},
        {.name = "benchmark one model", .argc = 10, .argv = {"izprime", "benchmark", "--limit", "1000", "--repeat", "1", "--algo", "SoE", "--save-results", bench_file}, .expected_exit = EXIT_SUCCESS, .stdout_contains = "Saved benchmark results", .file_to_check = bench_file, .expect_file_nonempty = 1},
        {.name = "benchmark bad model", .argc = 8, .argv = {"izprime", "benchmark", "--limit", "1000", "--repeat", "1", "--algo", "BAD"}, .expected_exit = EXIT_FAILURE, .stderr_contains = "Unknown model 'BAD'"},
    };

    int total = (int)(sizeof(cases) / sizeof(cases[0]));
    int passed = 0;

    for (int i = 0; i < total; ++i)
        passed += run_case(&cases[i], verbose);

    int failed = total - passed;

    print_line(60, '-');
    printf("CLI tests total:   %d\n", total);
    printf("CLI tests passed:  %d\n", passed);
    printf("CLI tests failed:  %d\n", failed);
    print_line(60, '-');

    if (failed == 0)
        printf("[SUCCESS] CLI tests passed!\n");
    else
        printf("[FAILURE] CLI tests failed :\\\n");

    return (failed == 0);
}
