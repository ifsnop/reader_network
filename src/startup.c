/*
reader_network - A package of utilities to record and work with
multicast radar data in ASTERIX format. (radar as in air navigation
surveillance).

Copyright (C) 2002-2012 Diego Torres <diego dot torres at gmail dot com>

This file is part of the reader_network utils.

reader_network is free software: you can redistribute it and/or modify
it under the terms of the Lesser GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

reader_network is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with reader_network. If not, see <http://www.gnu.org/licenses/>.
*/


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
