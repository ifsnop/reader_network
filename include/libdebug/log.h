#ifndef _DEBUG_LOG_H
#define _DEBUG_LOG_H

/*
 * Copyright (c) 2002-2004  Abraham vd Merwe <abz@blio.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *	  notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *	  notice, this list of conditions and the following disclaimer in the
 *	  documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of other contributors
 *	  may be used to endorse or promote products derived from this software
 *	  without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdarg.h>
#include <sys/syslog.h>

enum
{
   _LOG_QUIET   = 0,
   _LOG_ERROR   = 1,
   _LOG_WARNING = 4,
   _LOG_NORMAL  = 5,
   _LOG_VERBOSE = 6,
   _LOG_DEBUG   = 7,
   _LOG_NOISY   = 8
};

/*
 * This unfortunate hack is necessary in order to include
 * syslog.h in the library itself. The syslog header defines
 * conflicting symbols (LOG_DEBUG, LOG_WARNING, etc), so we
 * have to use different names inside the library.
 */
#ifndef _DEBUG_LOG_C

#define LOG_VERBOSE	_LOG_VERBOSE
#define LOG_ERROR	_LOG_ERROR
#define LOG_NORMAL	_LOG_NORMAL

#if defined(__linux)
#define LOG_QUIET	_LOG_QUIET
//#define LOG_WARNING	_LOG_WARNING
//#define LOG_DEBUG	_LOG_DEBUG
#define LOG_NOISY	_LOG_NOISY
#endif	/* #if defined(__linux)  */

#endif	/* #ifndef _DEBUG_LOG_C */

#define LOG_LEVELS	(_LOG_NOISY - _LOG_QUIET + 1)

/*
 * The LOG_HAVE_LOGFILE and LOG_USE_SYSLOG flags are appended
 * automatically by log_open() if the specified logfile is not
 * NULL (otherwise, the flags are removed).
 */

#define LOG_HAVE_LOGFILE		0x01
#define LOG_HAVE_COLORS			0x02
#define LOG_PRINT_FUNCTION		0x04
#define LOG_DEBUG_PREFIX_ONLY	0x08
#define LOG_DETECT_DUPLICATES	0x10
#define LOG_DETECT_FLOODING		0x20
#define LOG_USE_SYSLOG			0x40

/*
 * Initialize the log system.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
extern int log_open (const char *logfile,int loglevel,int flags);

/*
 * Close the log system. Any calls to the print routines after this call
 * is undefined.
 */
extern void log_close (void);

/*
 * Close and reopen the log file if necessary. This function may fail
 * and if it does, the log system is uninitialized.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_reset() log_reset_stub(__FILE__,__LINE__,__FUNCTION__)
extern int log_reset_stub (const char *filename,int line,const char *function);

/*
 * Print all the data that is currently pending.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_flush() log_flush_stub (__FILE__,__LINE__,__FUNCTION__)
extern int log_flush_stub (const char *filename,int line,const char *function);

/*
 * printf() replacement.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_printf(level,format,args...) log_printf_stub(__FILE__,__LINE__,__FUNCTION__,level,format,## args)
extern int log_printf_stub (const char *filename,int line,const char *function,int level,const char *format,...)
  __attribute__ ((format (printf,5,6)));

/*
 * vprintf() replacement.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_vprintf(level,format,ap) log_vprintf_stub(__FILE__,__LINE__,__FUNCTION__,level,format,ap)
extern int log_vprintf_stub (const char *filename,int line,const char *function,int level,const char *format,va_list ap);

/*
 * putc() replacement.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_putc(level,c) log_putc_stub(__FILE__,__LINE__,__FUNCTION__,level,c)
extern int log_putc_stub (const char *filename,int line,const char *function,int level,int c);

/*
 * puts() equivalent.
 *
 * Returns 0 if successful, -1 otherwise. Check errno to see what
 * error occurred.
 */
#define log_puts(level,str) log_puts_stub(__FILE__,__LINE__,__FUNCTION__,level,str)
extern int log_puts_stub (const char *filename,int line,const char *function,int level,const char *str);

#define MARKER() log_printf_stub(__FILE__,__LINE__,__FUNCTION__,LOG_DEBUG,"MARKER\n")

#if (defined _DEBUG_LOG_C && defined __sun)

#define LOG_MAKEPRI(fac, pri)   (((fac) << 3) | (pri))

#define INTERNAL_NOPRI  0x10    /* the "no priority" priority */
                                /* mark "facility" */
#define INTERNAL_MARK   LOG_MAKEPRI(LOG_NFACILITIES, 0)

typedef struct _code {
        char    *c_name;
        int     c_val;
} CODE;

CODE prioritynames[] =
  {
    { "alert", LOG_ALERT },
    { "crit", LOG_CRIT },
    { "debug", LOG_DEBUG },
    { "emerg", LOG_EMERG },
    { "err", LOG_ERR },
    { "error", LOG_ERR },               /* DEPRECATED */
    { "info", LOG_INFO },
    { "none", INTERNAL_NOPRI },         /* INTERNAL */
    { "notice", LOG_NOTICE },
    { "panic", LOG_EMERG },             /* DEPRECATED */
    { "warn", LOG_WARNING },            /* DEPRECATED */
    { "warning", LOG_WARNING },
    { NULL, -1 }
  };

#define LOG_KERN        (0<<3)  /* kernel messages */
#define LOG_USER        (1<<3)  /* random user-level messages */
#define LOG_MAIL        (2<<3)  /* mail system */
#define LOG_DAEMON      (3<<3)  /* system daemons */
#define LOG_AUTH        (4<<3)  /* security/authorization messages */
#define LOG_SYSLOG      (5<<3)  /* messages generated internally by syslogd */
#define LOG_LPR         (6<<3)  /* line printer subsystem */
#define LOG_NEWS        (7<<3)  /* network news subsystem */
#define LOG_UUCP        (8<<3)  /* UUCP subsystem */
//#define LOG_CRON        (9<<3)  /* clock daemon */
#define LOG_AUTHPRIV    (10<<3) /* security/authorization messages (private) */
#define LOG_FTP         (11<<3) /* ftp daemon */

#define LOG_LOCAL0      (16<<3) /* reserved for local use */
#define LOG_LOCAL1      (17<<3) /* reserved for local use */
#define LOG_LOCAL2      (18<<3) /* reserved for local use */
#define LOG_LOCAL3      (19<<3) /* reserved for local use */
#define LOG_LOCAL4      (20<<3) /* reserved for local use */
#define LOG_LOCAL5      (21<<3) /* reserved for local use */
#define LOG_LOCAL6      (22<<3) /* reserved for local use */
#define LOG_LOCAL7      (23<<3) /* reserved for local use */

#define LOG_NFACILITIES 24      /* current number of facilities */
#define LOG_FACMASK     0x03f8  /* mask to extract facility part */
                                /* facility of pri */
#define LOG_FAC(p)      (((p) & LOG_FACMASK) >> 3)

CODE facilitynames[] =
  {
    { "auth", LOG_AUTH },
    { "authpriv", LOG_AUTHPRIV },
    { "cron", LOG_CRON },
    { "daemon", LOG_DAEMON },
    { "ftp", LOG_FTP },
    { "kern", LOG_KERN },
    { "lpr", LOG_LPR },
    { "mail", LOG_MAIL },
    { "mark", INTERNAL_MARK },          /* INTERNAL */
    { "news", LOG_NEWS },
    { "security", LOG_AUTH },           /* DEPRECATED */
    { "syslog", LOG_SYSLOG },
    { "user", LOG_USER },
    { "uucp", LOG_UUCP },
    { "local0", LOG_LOCAL0 },
    { "local1", LOG_LOCAL1 },
    { "local2", LOG_LOCAL2 },
    { "local3", LOG_LOCAL3 },
    { "local4", LOG_LOCAL4 },
    { "local5", LOG_LOCAL5 },
    { "local6", LOG_LOCAL6 },
    { "local7", LOG_LOCAL7 },
    { NULL, -1 }
  };

#endif  /* #if (defined _DEBUG_LOG_C && defined __sun) */

#endif	/* #ifndef _DEBUG_LOG_H */

