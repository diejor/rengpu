#pragma once

#include <cstdio>
#include <cstdarg>
#include <cstring>

// ---------------------------------------------------------------------
// Log Levels
// ---------------------------------------------------------------------
enum LogLevel {
    LL_NONE = 0,
    LL_ERROR,
    LL_WARN,
    LL_INFO,
    LL_TRACE
};

// Human-readable strings for log levels (must match enum order).
static const char *LOG_LEVEL_STR[] = {
    "NONE",
    "ERROR",
    "WARN",
    "INFO",
    "TRACE"
};

// ---------------------------------------------------------------------
// Logger struct (holds output stream & minimum level)
// ---------------------------------------------------------------------
struct Logger {
    FILE *stream;  // e.g., stdout or stderr
    int  level;    // e.g., LL_INFO, LL_WARN, etc.
};

// Default logger instance: writes to stdout at LL_TRACE level.
static Logger DEFAULT_LOGGER = { stdout, LL_TRACE };

// Helper: set the level of the default logger at runtime.
inline void setLogLevel(int level) {
    DEFAULT_LOGGER.level = level;
}

// ---------------------------------------------------------------------
// Optional ANSI color codes (customize as you like)
// ---------------------------------------------------------------------
static const char *LOG_COLORS[] = {
    "\033[0m",     // LL_NONE  - no color
    "\033[0;31m",  // LL_ERROR - red
    "\033[0;33m",  // LL_WARN  - yellow
    "\033[0;37m",  // LL_INFO  - white
    "\033[0;90m",  // LL_TRACE - gray
};
static const char *RESET_COLOR = "\033[0m";

// ---------------------------------------------------------------------
// Macros for Logging
// ---------------------------------------------------------------------
#ifdef NO_LOG

// If NO_LOG is defined, strip out all logging calls at compile time.
#define LOG(logger, level, fmt, ...) (void)0
#define LOG_ERROR(fmt, ...)         (void)0
#define LOG_WARN(fmt, ...)          (void)0
#define LOG_INFO(fmt, ...)          (void)0
#define LOG_TRACE(fmt, ...)         (void)0

// Also disable the error macro.
#define ERR(cond, fmt, ...)         \
    do {                            \
        if (cond) {               \
            return;               \
        }                         \
    } while (0)

#else

#define LOG(logger, logLevel, fmt, ...)                                           \
    do {                                                                        \
        if ((logLevel) <= (logger).level) {                                     \
            constexpr int BUF_SIZE = 1024;                                      \
            char _log_buf[BUF_SIZE];                                            \
            std::snprintf(_log_buf, BUF_SIZE, fmt __VA_OPT__(, __VA_ARGS__));     \
            TracyMessage(_log_buf, strlen(_log_buf));  /* Send message to Tracy */\
            std::fprintf((logger).stream,                                     \
                         "%s[%s:%d %s()][%s]%s %s\n",                         \
                         LOG_COLORS[logLevel], __FILE__, __LINE__, __func__,    \
                         LOG_LEVEL_STR[logLevel], RESET_COLOR, _log_buf);        \
        }                                                                       \
    } while (0)


#define LOG_ERROR(fmt, ...)  LOG(DEFAULT_LOGGER, LL_ERROR, fmt __VA_OPT__(, __VA_ARGS__))
#define LOG_WARN(fmt, ...)   LOG(DEFAULT_LOGGER, LL_WARN,  fmt __VA_OPT__(, __VA_ARGS__))
#define LOG_INFO(fmt, ...)   LOG(DEFAULT_LOGGER, LL_INFO,  fmt __VA_OPT__(, __VA_ARGS__))
#define LOG_TRACE(fmt, ...)  LOG(DEFAULT_LOGGER, LL_TRACE, fmt __VA_OPT__(, __VA_ARGS__))

/**
 * ERR:
 *
 * If the condition is true, logs an error message at LL_ERROR level (including the
 * stringized condition) and returns from the function.
 *
 * Usage:
 *     ERR(val < 0, "Invalid negative value: %d", val);
 */
#define ERR(cond, fmt, ...)                                                     \
    do {                                                                        \
        if ((cond)) {                                                           \
            LOG(DEFAULT_LOGGER, LL_ERROR, "Condition \"%s\" is true. " fmt __VA_OPT__(, __VA_ARGS__), #cond); \
            return;                                                           \
        }                                                                       \
    } while (0)

/**
 * WARN_COND:
 *
 * If the condition is true, logs a warning message at LL_WARN level (including the
 * stringized condition) but does not return.
 */
#define WARN_COND(cond, fmt, ...)                                               \
    do {                                                                        \
        if ((cond)) {                                                           \
            LOG(DEFAULT_LOGGER, LL_WARN, "Condition \"%s\" is true. " fmt __VA_OPT__(, __VA_ARGS__), #cond); \
        }                                                                       \
    } while (0)

#endif // NO_LOG

