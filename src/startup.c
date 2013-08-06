
#include "includes.h"

void fail (const char *fmt, ...)
{
    va_list ap;
    va_start (ap,fmt);
#ifndef DEBUG_LOG
    if (fmt != NULL) log_vprintf (LOG_ERROR,fmt,ap);
#else   /* #ifndef DEBUG_LOG */
    if (fmt != NULL) {
        vfprintf (stderr,fmt,ap);
        fflush (stderr);
    }
#endif  /* #ifndef DEBUG_LOG */
    va_end (ap);
    exit (EXIT_FAILURE);
}

void startup(void) {
    mem_open(fail);
    if (log_open(NULL, LOG_VERBOSE, /*LOG_TIMESTAMP |*/
        LOG_HAVE_COLORS | LOG_PRINT_FUNCTION |
        LOG_DEBUG_PREFIX_ONLY /*| LOG_DETECT_DUPLICATES*/)) {
        fprintf(stderr, "log_open failed: %m\n");
        exit (EXIT_FAILURE);
    }
//    atexit(mem_close);
//    atexit(log_close);

    return;			     
}
