/**
 * @file cli.h
 * @brief Public entrypoint for the iZprime CLI application layer.
 */

#ifndef IZ_CLI_H
#define IZ_CLI_H

/**
 * @brief Run CLI command dispatch.
 * @param argc Argument count from process entry.
 * @param argv Argument vector from process entry.
 * @return Process exit code.
 */
int cli_run(int argc, char **argv);

#endif // IZ_CLI_H
