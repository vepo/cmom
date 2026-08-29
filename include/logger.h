#ifndef LOGGER_H
#define LOGGER_H
#include <config.h>
#include <stdio.h>
#include <stdlib.h>


/* -------------------------------------------------------------------------
 * Compile-time switch: define ENABLE_LOGGING to turn logging ON.
 * If not defined, all logging macros expand to nothing.
 *
 * To enable:  add -DENABLE_LOGGING to your CFLAGS in Makefile.am
 * To disable: just omit it (or add -UDISABLE_LOGGING)
 * ------------------------------------------------------------------------- */
#ifdef ENABLE_LOGGING

    /* If log4c is available (HAVE_LOG4C comes from config.h), use it */
    #ifdef HAVE_LOG4C
        #include <log4c.h>

        /* Global category – you can use different categories per module */
        extern log4c_category_t* g_log_category;

        /* One‑time initialisation – call this in main() */
        #define LOGGER_INIT() \
            do { \
                if (log4c_init() != 0) { \
                    fprintf(stderr, "log4c_init() failed\n"); \
                    exit(EXIT_FAILURE); \
                } \
                g_log_category = log4c_category_get("cmom"); \
            } while (0)

        #define LOGGER_CLEANUP() log4c_fini()

        /* Logging macros – map directly to log4c */
        #define LOG_DEBUG(fmt, ...) \
            log4c_category_log(g_log_category, LOG4C_PRIORITY_DEBUG, fmt, ##__VA_ARGS__)
        #define LOG_INFO(fmt, ...) \
            log4c_category_log(g_log_category, LOG4C_PRIORITY_INFO,  fmt, ##__VA_ARGS__)
        #define LOG_WARN(fmt, ...) \
            log4c_category_log(g_log_category, LOG4C_PRIORITY_WARN,  fmt, ##__VA_ARGS__)
        #define LOG_ERROR(fmt, ...) \
            log4c_category_log(g_log_category, LOG4C_PRIORITY_ERROR, fmt, ##__VA_ARGS__)
        #define LOG_FATAL(fmt, ...) \
            log4c_category_log(g_log_category, LOG4C_PRIORITY_FATAL, fmt, ##__VA_ARGS__)

    #else /* no log4c → fallback to stderr */

        #define LOGGER_INIT() \
            setlinebuf(stdout); \
            setlinebuf(stderr)
        #define LOGGER_CLEANUP()  /* nothing */

        #define LOG_DEBUG(fmt, ...) \
            fprintf(stdout, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
        #define LOG_INFO(fmt, ...) \
            fprintf(stdout, "[INFO]  " fmt "\n", ##__VA_ARGS__)
        #define LOG_WARN(fmt, ...) \
            fprintf(stderr, "[WARN]  " fmt "\n", ##__VA_ARGS__)
        #define LOG_ERROR(fmt, ...) \
            fprintf(stderr, "[ERROR] " fmt "\n", ##__VA_ARGS__)
        #define LOG_FATAL(fmt, ...) \
            fprintf(stderr, "[FATAL] " fmt "\n", ##__VA_ARGS__)

    #endif /* HAVE_LOG4C */

#else /* ENABLE_LOGGING not defined → everything is removed at compile time */

    #define LOGGER_INIT()     /* nothing */
    #define LOGGER_CLEANUP()  /* nothing */
    #define LOG_DEBUG(...)    ((void)0)
    #define LOG_INFO(...)     ((void)0)
    #define LOG_WARN(...)     ((void)0)
    #define LOG_ERROR(...)    ((void)0)
    #define LOG_FATAL(...)    ((void)0)

#endif /* ENABLE_LOGGING */

#endif /* LOGGER_H */