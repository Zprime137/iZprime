/**
 * @file logger.c
 * @brief Logging module.
 *
 * This module provides functions for logging messages at different levels.
 * @ingroup iz_logger
 */
#ifdef ENABLE_LOGGING

#include "logger.h"
#include <stdio.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>
#include <sys/stat.h>
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

// Initialize the mutex for thread-safe logging
static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static LogLevel current_log_level = LOG_DEBUG; // Default log level

/**
 * @brief Returns a string representation of the log level.
 *
 * @param level The log level to convert.
 * @return A string corresponding to the log level.
 */
const char *log_level_to_string(LogLevel level)
{
    switch (level)
    {
    case LOG_DEBUG:
        return "DEBUG";
    case LOG_INFO:
        return "INFO";
    case LOG_WARNING:
        return "WARNING";
    case LOG_ERROR:
        return "ERROR";
    case LOG_FATAL:
        return "FATAL";
    default:
        return "UNKNOWN";
    }
}

/**
 * @brief Get the current timestamp formatted as a string.
 *
 * @param buffer A buffer to store the timestamp.
 * @param size The size of the buffer.
 */
void get_current_timestamp(char *buffer, size_t size)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", t);
}

/**
 * @brief Rotates the log files if the current log exceeds the maximum size.
 *
 * Rotates the current log file by renaming it to "logfile.1". Older logs
 * are renamed (logfile.2, logfile.3, etc.), up to 5 versions. If the oldest
 * log exists, it is removed.
 *
 * @param log_file The path to the log file.
 * @param max_size The maximum allowed log file size in bytes.
 */
void log_rotate(const char *log_file, size_t max_size)
{
    struct stat st;
    if (stat(log_file, &st) == 0 && st.st_size >= (off_t)max_size) // Cast max_size to off_t to match types
    {
        char old_log_file[256];
        for (int i = 5; i > 0; --i)
        {
            snprintf(old_log_file, sizeof(old_log_file), "%s.%d", log_file, i);
            if (i == 5)
            {
                remove(old_log_file); // Remove the oldest log file
            }
            else
            {
                char prev_log_file[256];
                snprintf(prev_log_file, sizeof(prev_log_file), "%s.%d", log_file, i + 1);
                rename(old_log_file, prev_log_file);
            }
        }

        char new_log_file[256];
        snprintf(new_log_file, sizeof(new_log_file), "%s.1", log_file);
        rename(log_file, new_log_file);
    }
}

/**
 * @brief Initializes the logging system.
 *
 * Creates the log directory if it doesn't exist and rotates logs if necessary.
 *
 * @param log_file The path to the log file.
 */
void log_init(const char *log_file)
{
    struct stat st = {0};
    if (stat(LOG_DIR, &st) == -1)
    {
        mkdir(LOG_DIR, 0700); // Create log directory if it doesn't exist
    }

    log_rotate(log_file, LOG_MAX_SIZE); // Rotate logs if needed
}

/**
 * @brief Shuts down the logging system and cleans up resources.
 */
void log_shutdown(void)
{
    pthread_mutex_destroy(&log_mutex);
}

/**
 * @brief Sets the current log level. Messages below this level will not be logged.
 *
 * @param level The log level to set.
 */
void log_set_log_level(LogLevel level)
{
    current_log_level = level;
}

/**
 * @brief Internal function to log messages using va_list.
 *
 * This function handles the core logging logic, including writing to the log
 * file in a thread-safe manner using a mutex.
 *
 * @param level The log level for the message.
 * @param format The format string for the log message.
 * @param args The va_list of arguments.
 */
static void log_message_va(LogLevel level, const char *format, va_list args)
{
    if (level < current_log_level)
        return; // Don't log messages below the current log level

    char message[1024];
    // Format string comes from a variadic function parameter - suppress warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(message, sizeof(message), format, args);
#pragma clang diagnostic pop

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    FILE *file = fopen(LOG_FILE, "a");
    if (!file)
    {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex); // Ensure mutex is unlocked
        return;
    }

    fprintf(file, "[%s] [%s] %s\n", timestamp, log_level_to_string(level), message);
    fclose(file);

    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Core function to log messages with a formatted string.
 *
 * This function handles the core logging logic, including writing to the log
 * file in a thread-safe manner using a mutex.
 *
 * @param level The log level for the message.
 * @param format The format string for the log message.
 */
void log_message(LogLevel level, const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(level, format, args);
    va_end(args);
}

/**
 * @brief Log a message with extended information (file, line number).
 *
 * @param level The log level for the message.
 * @param file_name The source file where the log was generated.
 * @param line_number The line number in the source file.
 * @param format The format string for the log message.
 */
void log_message_extended(LogLevel level, const char *file_name, int line_number, const char *format, ...)
{
    if (level < current_log_level)
        return; // Don't log messages below the current log level

    va_list args;
    va_start(args, format);
    char message[1024];
    // Format string comes from a variadic function parameter - suppress warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(message, sizeof(message), format, args);
#pragma clang diagnostic pop
    va_end(args);

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    pthread_mutex_lock(&log_mutex);

    FILE *file = fopen(LOG_FILE, "a");
    if (!file)
    {
        perror("Failed to open log file");
        pthread_mutex_unlock(&log_mutex);
        return;
    }

    fprintf(file, "[%s] [%s] %s (File: %s, Line: %d)\n",
            timestamp, log_level_to_string(level), message, file_name, line_number);
    fclose(file);

    pthread_mutex_unlock(&log_mutex);
}

/**
 * @brief Logs a formatted message to the console only, without requiring a log level.
 *
 * This can be used for debugging or tracking specific points in the code.
 *
 * @param format The format string for the log message.
 */
void log_console(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    char message[1024];
    // Format string comes from a variadic function parameter - suppress warning
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wformat-nonliteral"
    vsnprintf(message, sizeof(message), format, args);
#pragma clang diagnostic pop
    va_end(args);

    char timestamp[20];
    get_current_timestamp(timestamp, sizeof(timestamp));

    // Print the message with a timestamp to the console
    printf("[%s] %s\n", timestamp, message);
}

/**
 * @brief Convenience function for logging debug messages.
 *
 * @param format The format string for the log message.
 */
void log_debug(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(LOG_DEBUG, format, args);
    va_end(args);
}

/**
 * @brief Convenience function for logging info messages.
 *
 * @param format The format string for the log message.
 */
void log_info(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(LOG_INFO, format, args);
    va_end(args);
}

/**
 * @brief Convenience function for logging warning messages.
 *
 * @param format The format string for the log message.
 */
void log_warn(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(LOG_WARNING, format, args);
    va_end(args);
}

/**
 * @brief Convenience function for logging error messages.
 *
 * @param format The format string for the log message.
 */
void log_error(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(LOG_ERROR, format, args);
    va_end(args);
}

/**
 * @brief Convenience function for logging fatal messages.
 *
 * @param format The format string for the log message.
 */
void log_fatal(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    log_message_va(LOG_FATAL, format, args);
    va_end(args);
}

#endif // ENABLE_LOGGING
