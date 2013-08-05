
#include <stdlib.h>
#include <stdarg.h>

#include <debug/memory.h>
#include <debug/log.h>

#include <config/config.h>

#define xcfg_get_string(x) if (!cfg_get_string(x)) fail ("couldn't get " #x "\n")
#define xcfg_get_integer(x) if (!cfg_get_integer(x)) fail ("couldn't get " #x "\n")
#define xcfg_get_boolean(x) if (!cfg_get_boolean(x)) fail ("couldn't get " #x "\n")
#define xcfg_get_enumeration(x) if (!cfg_get_enumeration(x)) fail ("couldn't get " #x "\n")
#define xcfg_get_string_array(x,n) if (!cfg_get_string_array(x,n)) fail ("couldn't get " #x "\n")
#define xcfg_get_integer_array(x,n) if (!cfg_get_integer_array(x,n)) fail ("couldn't get " #x "\n")
#define xcfg_get_boolean_array(x,n) if (!cfg_get_boolean_array(x,n)) fail ("couldn't get " #x "\n")
#define xcfg_get_enumeration_array(x,n) if (!cfg_get_enumeration_array(x,n)) fail ("couldn't get " #x "\n")

typedef struct
{
   char *string;
   bool *array;
   long int tt;
   char **abit;
} section_t;

typedef struct
{
   char **array;
   struct
	 {
		bool *g;
		bool h;
	 } f;
} section2_t;

static long int integer,*array,*tt,_int2;
static char *string,**array2,**array4,*fALSE;
static bool TRUE,FALSE,*array3,*a;
static int n[10];
static section_t section;
static section2_t section2;

static void fail (const char *s)
{
   log_puts (LOG_ERROR,s);
   exit (EXIT_FAILURE);
}

static void test_get (void)
{
   xcfg_get_string (string);
   xcfg_get_integer (integer);
   xcfg_get_boolean (TRUE);
   xcfg_get_boolean (FALSE);
   xcfg_get_enumeration (fALSE);
   xcfg_get_integer_array (array,&n[0]);
   xcfg_get_string_array (array2,&n[1]);
   xcfg_get_boolean_array (array3,&n[2]);
   xcfg_get_enumeration_array (array4,&n[3]);
   xcfg_get_integer (_int2);
   xcfg_get_integer_array (tt,&n[4]);
   xcfg_get_string (section.string);
   xcfg_get_boolean_array (section.array,&n[5]);
   xcfg_get_integer (section.tt);
   xcfg_get_enumeration_array (section.abit,&n[6]);
   xcfg_get_boolean_array (a,&n[9]);
   xcfg_get_string_array (section2.array,&n[7]);
   xcfg_get_boolean_array (section2.f.g,&n[8]);
   xcfg_get_boolean (section2.f.h);
}

static void test_display (void)
{
   int i;

   log_printf (LOG_DEBUG,"string = \"%s\"\n",string);
   mem_free (string);

   log_printf (LOG_DEBUG,"integer = %ld\n",integer);
   log_printf (LOG_DEBUG,"TRUE = %d\n",TRUE);
   log_printf (LOG_DEBUG,"FALSE = %d\n",FALSE);

   log_printf (LOG_DEBUG,"fALSE = %s\n",fALSE);
   mem_free (fALSE);

   log_printf (LOG_DEBUG,"array = {");
   for (i = 0; i < n[0]; i++) log_printf (LOG_DEBUG," %ld",array[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (array);

   log_printf (LOG_DEBUG,"array2 = {");
   for (i = 0; i < n[1]; i++)
	 {
		log_printf (LOG_DEBUG," \"%s\"",array2[i]);
		mem_free (array2[i]);
	 }
   log_printf (LOG_DEBUG," }\n");
   mem_free (array2);

   log_printf (LOG_DEBUG,"array3 = {");
   for (i = 0; i < n[2]; i++) log_printf (LOG_DEBUG," %d",array3[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (array3);

   log_printf (LOG_DEBUG,"array4 = {");
   for (i = 0; i < n[3]; i++)
	 {
		log_printf (LOG_DEBUG," %s",array4[i]);
		mem_free (array4[i]);
	 }
   log_printf (LOG_DEBUG," }\n");
   mem_free (array4);

   log_printf (LOG_DEBUG,"_int2 = %ld\n",_int2);

   log_printf (LOG_DEBUG,"tt = {");
   for (i = 0; i < n[4]; i++) log_printf (LOG_DEBUG," %ld",tt[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (tt);

   log_printf (LOG_DEBUG,"section.string = \"%s\"\n",section.string);
   mem_free (section.string);

   log_printf (LOG_DEBUG,"section.array = {");
   for (i = 0; i < n[5]; i++) log_printf (LOG_DEBUG," %d",section.array[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (section.array);

   log_printf (LOG_DEBUG,"section.tt = %ld\n",section.tt);

   log_printf (LOG_DEBUG,"section.abit = {");
   for (i = 0; i < n[6]; i++)
	 {
		log_printf (LOG_DEBUG," %s",section.abit[i]);
		mem_free (section.abit[i]);
	 }
   log_printf (LOG_DEBUG," }\n");
   mem_free (section.abit);

   log_printf (LOG_DEBUG,"a = {");
   for (i = 0; i < n[9]; i++) log_printf (LOG_DEBUG," %d",a[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (a);

   log_printf (LOG_DEBUG,"section2.array = {");
   for (i = 0; i < n[7]; i++)
	 {
		log_printf (LOG_DEBUG," \"%s\"",section2.array[i]);
		mem_free (section2.array[i]);
	 }
   log_printf (LOG_DEBUG," }\n");
   mem_free (section2.array);

   log_printf (LOG_DEBUG,"section2.f.g = {");
   for (i = 0; i < n[8]; i++) log_printf (LOG_DEBUG," %d",section2.f.g[i]);
   log_printf (LOG_DEBUG," }\n");
   mem_free (section2.f.g);

   log_printf (LOG_DEBUG,"section2.f.h = %d\n",section2.f.h);
}

int main (void)
{
   mem_open (NULL);
   log_open (NULL,LOG_NOISY,LOG_HAVE_COLORS | LOG_PRINT_FUNCTION);
   atexit (mem_close);
   atexit (log_close);

   if (!cfg_open ("TEST.conf"))
	 exit (EXIT_FAILURE);

   test_get ();

   cfg_close ();

   test_display ();

   exit (EXIT_SUCCESS);
}

