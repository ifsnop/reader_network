
#include <stdlib.h>
#include <stdarg.h>

#include <debug/memory.h>
#include <debug/log.h>

#include <config/parse.h>

void space (int level,int n)
{
   int i;
   for (i = 0; i < n * 4; i++) log_printf (level," ");
}

static void dump_argument (int type,arg_t *arg)
{
   switch (type)
	 {
	  case TOK_INTEGER:
		log_printf (LOG_DEBUG,"%ld",arg->integer);
		break;
	  case TOK_STRING:
		log_printf (LOG_DEBUG,"\"%s\"",arg->string);
		break;
	  case TOK_BOOLEAN:
		log_printf (LOG_DEBUG,"%s",arg->boolean ? "true" : "false");
		break;
	  case TOK_ENUM:
		log_printf (LOG_DEBUG,"%s",arg->string);
		break;
	  default:
		log_printf (LOG_DEBUG,"(unknown argument type: 0x%.4x)",type);
	 }
}

static void dump_statement (int n,stmt_t *s)
{
   space (LOG_DEBUG,n);
   log_printf (LOG_DEBUG,"%s = ",s->name);
   if (s->n != 1) log_printf (LOG_DEBUG,"{ ");
   dump_argument (s->type,&s->args[0]);
   if (s->n != 1)
	 {
		int i;
		for (i = 1; i < s->n; i++)
		  {
			 log_printf (LOG_DEBUG,", ");
			 dump_argument (s->type,&s->args[i]);
		  }
		log_printf (LOG_DEBUG," }");
	 }
   log_printf (LOG_DEBUG,"\n");
}

static void dump_statements (int n,stmt_t *s)
{
   stmt_t *tmp,*prev = s;
   if (s != NULL)
	 do
	   {
		  tmp = s->next;
		  dump_statement (n,s);
		  s = tmp;
	   }
     while (prev != s);
}

static void dump_sections (int n,section_t *s)
{
   int i;

   if (n)
	 {
		space (LOG_DEBUG,n - 1);
		log_printf (LOG_DEBUG,"%s = {\n",s->name);
	 }

   dump_statements (n,s->stmt);

   for (i = 0; i < s->n; i++) dump_sections (n + 1,s->child[i]);

   if (n)
	 {
		space (LOG_DEBUG,n - 1);
		log_printf (LOG_DEBUG,"}\n");
	 }
}

int main ()
{
   section_t *section;

   mem_open (NULL);
   log_open (NULL,LOG_NOISY,LOG_HAVE_COLORS | LOG_PRINT_FUNCTION);
   atexit (mem_close);
   atexit (log_close);

   if ((section = parse ("TEST.conf")) == NULL)
	 exit (EXIT_FAILURE);

   dump_sections (0,section);
   parse_destroy (&section);

   exit (EXIT_SUCCESS);
}

