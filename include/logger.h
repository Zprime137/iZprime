/**
 * @file logger.h
 * @brief Header file for the logging system.
 *
 * @details
 * This file contains function declarations and macros for a logging system
 * that provides various logging levels (DEBUG, INFO, WARNING, ERROR, FATAL).
 * The logging system supports thread-safe logging, log rotation, and
 * console output. It also includes convenience functions for logging
 * messages with extended information (file name, line number).
 *
 * @note
 * This module is part of a larger project that includes various
 * components requiring logging functionality. It is designed to be
 * flexible and easy to use, allowing developers to log messages
 * at different levels and with different formats.
 *
 * API:
 * - log_init: Initializes the logging system.
 * - log_shutdown: Shuts down the logging system.
 * - log_set_log_level: Sets the current log level.
 * - log_message: Logs a formatted message at the given log level.
 * - log_message_extended: Logs a formatted message with extended information.
 * - log_console: Logs a formatted message to the console.
 * - log_debug: Logs a debug message.
 * - log_info: Logs an info message.
 * - log_warn: Logs a warning message.
 * - log_error: Logs an error message.
 * - log_fatal: Logs a fatal message.
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/** @defgroup iz_logger Logging
 *  @brief Thread-safe runtime logging helpers.
 *  @{ */

/**
 * @def LOGGER_FORMAT_PRINTF(fmt_idx, arg_idx)
 * @brief Compiler attribute wrapper for printf-style format checking.
 */
#if defined(__GNUC__) || defined(__clang__)
#define LOGGER_FORMAT_PRINTF(fmt_idx, arg_idx) __attribute__((format(printf, fmt_idx, arg_idx)))
#else
#define LOGGER_FORMAT_PRINTF(fmt_idx, arg_idx)
#endif

// Log directory and file configuration
#define LOG_DIR "logs/"              ///< Directory where logs are stored
#define LOG_FILE LOG_DIR "log.txt"   ///< Default log file
#define LOG_MAX_SIZE 1024 * 1024 * 5 ///< Maximum log file size (5 MB)

/**
 * @brief Enumeration of log levels.
 *
 * This enumeration defines the different log levels that can be used
 * in the logging system. Each level corresponds to a severity of importance.
 *
 */
typedef enum
{
    LOG_DEBUG,   ///< Debug level
    LOG_INFO,    ///< Info level
    LOG_WARNING, ///< Warning level
    LOG_ERROR,   ///< Error level
    LOG_FATAL    ///< Fatal level
} LogLevel;

// Function declarations

/**
 * @brief Returns a string representation of the log level.
 *
 * @param level The log level.
 * @return A string corresponding to the log level.
 */
const char *log_level_to_string(LogLevel level);

/**
 * @brief Initializes the logging system.
 *
 * Creates the log directory if it doesn't exist and rotates logs if necessary.
 *
 * @param log_file The path to the log file.
 */
void log_init(const char *log_file);

/**
 * @brief Shuts down the logging system and cleans up resources.
 */
void log_shutdown(void);

/**
 * @brief Sets the current log level. Messages below this level will not be logged.
 *
 * @param level The log level to set.
 */
void log_set_log_level(LogLevel level);

/**
 * @brief Logs a formatted message at the given log level.
 *
 * This function logs messages to the log file in a thread-safe manner.
 *
 * @param level The log level for the message.
 * @param format The format string for the log message.
 */
void log_message(LogLevel level, const char *format, ...) LOGGER_FORMAT_PRINTF(2, 3);

/**
 * @brief Logs a formatted message with extended information (file name, line number).
 *
 * This function logs messages to the log file in a thread-safe manner.
 *
 * @param level The log level for the message.
 * @param file_name The source file where the log was generated.
 * @param line_number The line number in the source file.
 * @param format The format string for the log message.
 */
void log_message_extended(LogLevel level, const char *file_name, int line_number, const char *format, ...) LOGGER_FORMAT_PRINTF(4, 5);

/**
 * @brief Logs a timestamped message to the console only, without requiring a log level.
 *
 * This can be used for debugging or tracking specific points in the code.
 *
 * @param format The format string for the log message.
 */
void log_console(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

// Convenience logging functions for different log levels
/**
 * @brief Logs a debug message.
 *
 * @param format The format string for the log message.
 */
void log_debug(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

/**
 * @brief Logs an info message.
 *
 * @param format The format string for the log message.
 */
void log_info(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

/**
 * @brief Logs a warning message.
 *
 * @param format The format string for the log message.
 */
void log_warn(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

/**
 * @brief Logs an error message.
 *
 * @param format The format string for the log message.
 */
void log_error(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

/**
 * @brief Logs a fatal message.
 *
 * @param format The format string for the log message.
 */
void log_fatal(const char *format, ...) LOGGER_FORMAT_PRINTF(1, 2);

/** @} */

#endif // LOGGER_H
