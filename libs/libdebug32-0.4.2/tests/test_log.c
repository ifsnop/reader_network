
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <debug/memory.h>
#include <debug/log.h>

#define TEST_NEWLINES
#define TEST_APPEND
#define TEST_LEVELS
#define TEST_DUPLICATE
#define TEST_FLUSH
/* #define TEST_SYSLOG */

#ifdef TEST_SYSLOG
#undef TEST_FLUSH
#endif	/* #ifdef TEST_SYSLOG */

static void fail (const char *fmt, ...)
{
   va_list ap;
   va_start (ap,fmt);
#ifndef DEBUG_LOG
   if (fmt != NULL) log_vprintf (LOG_ERROR,fmt,ap);
#else	/* #ifndef DEBUG_LOG */
   if (fmt != NULL)
	 {
		vfprintf (stderr,fmt,ap);
		fflush (stderr);
	 }
#endif	/* #ifndef DEBUG_LOG */
   va_end (ap);
   exit (EXIT_FAILURE);
}

#ifdef TEST_DUPLICATE
static void repeat ()
{
   log_printf (LOG_DEBUG,"this is one of the repeated messages\n");
}
#endif	/* TEST_DUPLICATE */

int main ()
{
   mem_open (fail);
#ifdef TEST_SYSLOG
   if (log_open ("test.daemon",LOG_NOISY,LOG_HAVE_COLORS | LOG_PRINT_FUNCTION | LOG_DEBUG_PREFIX_ONLY | LOG_DETECT_DUPLICATES))
#else	/* #ifdef TEST_SYSLOG */
   if (log_open (NULL,LOG_NOISY,LOG_HAVE_COLORS | LOG_PRINT_FUNCTION | LOG_DEBUG_PREFIX_ONLY | LOG_DETECT_DUPLICATES))
#endif	/* #ifdef TEST_SYSLOG */
	 {
		fprintf (stderr,"log_open failed: %m\n");
		exit (EXIT_FAILURE);
	 }
   atexit (mem_close);
   atexit (log_close);

#ifdef TEST_NEWLINES
   log_printf (LOG_NORMAL,"1, 1\n1, 2%c1, 3\n",10);
   log_printf (LOG_NORMAL,"2, 1%c2, 2" "\012" "2, 3%c\n",'\n',0x0a);
#endif	/* TEST_NEWLINES */

#ifdef TEST_APPEND
   log_printf (LOG_NORMAL,"hello ");
   log_printf (LOG_NORMAL,"brave new ");
   log_printf (LOG_NORMAL,"world\n");
#endif	/* TEST_APPEND */

#ifdef TEST_LEVELS
   log_printf (LOG_QUIET,"this shouldn't be printed\n");
   log_printf (LOG_ERROR,"error");
   log_printf (LOG_WARNING,"warning");
   log_printf (LOG_NORMAL,"normal");
   log_printf (LOG_VERBOSE,"verbose\n");
   log_printf (LOG_DEBUG,"debug");
   log_printf (LOG_NOISY,"noisy\n");
#endif	/* TEST_LEVELS */

#ifdef TEST_DUPLICATE
   log_printf (LOG_VERBOSE,"i'm going to repeat 3 identical messages and then quit.\n");
   repeat ();
   repeat ();
   repeat ();
#endif	/* TEST_DUPLICATE */

#ifdef TEST_FLUSH
   log_printf (LOG_NORMAL,"> ");
   log_flush ();
   write (STDOUT_FILENO,"this should be after the '>'",28);
   log_printf (LOG_NORMAL," and this is the trailer\n");
   log_printf (LOG_NORMAL,"newline\n");

   repeat();
   repeat();
   repeat();
   log_flush ();
   repeat();
#endif	/* TEST_FLUSH */

   exit (EXIT_SUCCESS);
}

