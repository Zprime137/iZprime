#include <test_api.h>
#include <cli.h>

typedef struct
{
    const char *name;
    int argc;
    char *argv[10];
    int expected_exit;
} CLI_TEST_CASE;

static int run_case(const CLI_TEST_CASE *test_case, int verbose)
{
    int actual_exit = cli_run(test_case->argc, (char **)test_case->argv);
    int ok = (actual_exit == test_case->expected_exit);

    if (verbose)
    {
        printf("[%-7s] %-28s expected=%d got=%d\n",
               ok ? "PASS" : "FAIL",
               test_case->name,
               test_case->expected_exit,
               actual_exit);
    }

    return ok;
}

int TEST_CLI(int verbose)
{
    print_test_fn_header("CLI COMMAND SMOKE TESTS");

    CLI_TEST_CASE cases[] = {
        {
            .name = "general help",
            .argc = 1,
            .argv = {"izprime"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "help stream_primes",
            .argc = 3,
            .argv = {"izprime", "help", "stream_primes"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "unknown command",
            .argc = 2,
            .argv = {"izprime", "does_not_exist"},
            .expected_exit = EXIT_FAILURE,
        },
        {
            .name = "stream primes print",
            .argc = 5,
            .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--print"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "stream primes gaps",
            .argc = 5,
            .argv = {"izprime", "stream_primes", "--range", "[0, 200]", "--print-gaps"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "count primes cores",
            .argc = 6,
            .argv = {"izprime", "count_primes", "--range", "[0, 200]", "--cores", "1"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "count primes cores max",
            .argc = 6,
            .argv = {"izprime", "count_primes", "--range", "[0, 200]", "--cores", "max"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "count primes cores alias",
            .argc = 6,
            .argv = {"izprime", "count_primes", "--range", "[0, 200]", "--cores-number", "1"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "next prime expression",
            .argc = 4,
            .argv = {"izprime", "next_prime", "--n", "10^2+1"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "prev prime expression",
            .argc = 4,
            .argv = {"izprime", "prev_prime", "--n", "10^2+1"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "is prime",
            .argc = 6,
            .argv = {"izprime", "is_prime", "--n", "97", "--rounds", "5"},
            .expected_exit = EXIT_SUCCESS,
        },
        {
            .name = "is prime invalid rounds",
            .argc = 6,
            .argv = {"izprime", "is_prime", "--n", "97", "--rounds", "0"},
            .expected_exit = EXIT_FAILURE,
        },
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
        printf("[SUCCESS] CLI smoke tests passed!\n");
    else
        printf("[FAILURE] CLI smoke tests failed :\\\n");

    return (failed == 0);
}
