
#include <stdlib.h>
#include <string.h>

#include <debug/memory.h>
#include <debug/log.h>
#include <debug/hex.h>

#define TEST_ALLOC
#define TEST_REALLOC

#ifdef TEST_ALLOC
static void *xalloc (size_t count)
{
   void *tmp;

   if ((tmp = mem_alloc (count)) == NULL)
	 {
		log_printf (LOG_ERROR,"alloc failed: %m\n");
		exit (EXIT_FAILURE);
	 }

   return (tmp);
}
#endif	/* #ifdef TEST_ALLOC */

#ifdef TEST_REALLOC
static void *xrealloc (void *ptr,size_t count)
{
   void *tmp;

   if ((tmp = mem_realloc (ptr,count)) == NULL)
	 {
		log_printf (LOG_ERROR,"realloc failed: %m\n");
		mem_free (ptr);
		exit (EXIT_FAILURE);
	 }

   return (tmp);
}
#endif	/* #ifdef TEST_REALLOC */

#ifdef TEST_ALLOC
static void test_alloc ()
{
   void *tmp;
   int i;

   for (i = 1; i < 1024*1024; i += 1025)
	 {
		tmp = xalloc (i);
		memset (tmp,0x55,i);
		memset (tmp,0xaa,i);
		mem_free (tmp);
	 }
}
#endif	/* #ifdef TEST_ALLOC */

#ifdef TEST_REALLOC
static void test_realloc ()
{
   unsigned char *tmp = NULL,pattern = 0x55;
   int i,j,k;

   for (i = 0; i < 16; i++)
	 {
		tmp = xrealloc (tmp,(i + 1) * 1025);
		memset (tmp + i * 1025,pattern,1025);

		for (j = 0; j < i + 1; j++)
		  for (k = 0; k < 1025; k++) if (tmp[k + j * 1025] != pattern)
			{
			   mem_free (tmp);
			   log_printf (LOG_ERROR,"realloc test failed on round %d at position %d\n",i,j);
			   exit (EXIT_FAILURE);
			}

		log_printf (LOG_NORMAL,"round %d succeeded\n",i);
	 }

   mem_free (tmp);
}
#endif	/* #ifdef TEST_REALLOC */

int main ()
{
   mem_open (NULL);
   log_open (NULL,LOG_NOISY,LOG_HAVE_COLORS | LOG_PRINT_FUNCTION);
   atexit (mem_close);
   atexit (log_close);

#ifdef TEST_ALLOC
   test_alloc ();
#endif	/* #ifdef TEST_ALLOC */

#ifdef TEST_REALLOC
   test_realloc ();
#endif	/* #ifdef TEST_REALLOC */

   exit (EXIT_SUCCESS);
}

