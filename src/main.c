/**
 * @file main.c
 * @brief Thin process entrypoint that delegates to the CLI module.
 */

#include <cli.h>

int main(int argc, char **argv)
{
    return cli_run(argc, argv);
}
