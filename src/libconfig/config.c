
/*
 * Copyright (c) Abraham vd Merwe <abz@blio.com>
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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>

#include <libdebug/memory.h>
#include <libdebug/log.h>

#include <libconfig/typedefs.h>
#include <libconfig/parse.h>
#include <libconfig/config.h>

static bool match_section (section_t *s,const char *name)
{
   const char *end = strchr (name,'.');
   size_t n = strlen (name);
   if (end != NULL) n = (size_t) (end - name);
   return (strlen (s->name) == n && !strncmp (s->name,name,n));
}

static section_t *find_section (section_t *s,const char *name)
{
   const char *next = strchr (name,'.');
   section_t *tmp;
   int i;

   if (next == NULL) return (NULL);

   next++;

   for (i = 0; i < s->n; i++)
	 if (match_section (s->child[i],name))
	   {
		  if (strchr (next,'.') == NULL) return (s->child[i]);
		  if ((tmp = find_section (s->child[i],next)) != NULL) return (tmp);
	   }

   return (NULL);
}

static stmt_t *find_statement (stmt_t *s,const char *name)
{
   stmt_t *tmp,*prev = s;

   if (s == NULL) return (NULL);

   do
	 {
		tmp = s->next;
		if (!strcmp (s->name,name)) return (s);
		s = tmp;
	 }
   while (prev != s);

   return (NULL);
}

static stmt_t *find (section_t *s,const char *name)
{
   section_t *tmp;
   if (s == NULL) return (NULL);
   if (strchr (name,'.') == NULL) return (find_statement (s->stmt,name));
   if ((tmp = find_section (s,name)) == NULL) return (NULL);
   return (find_statement (tmp->stmt,strrchr (name,'.') + 1));
}

/************************/

section_t *sections = NULL;

/* open configuration file */
bool cfg_open (const char *filename)
{
   if (sections == NULL)
	 {
		if ((sections = parse (filename)) == NULL)
		  return (false);
	 }
   return (true);
}

/* close configuration file */
void cfg_close (void)
{
   if (sections != NULL) parse_destroy (&sections);
}

/* extract boolean value from configuration file */
bool cfg_get_bool (bool *value,const char *name)
{
   stmt_t *tmp;
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == TOK_BOOLEAN && tmp->n == 1)
	 {
		*value = tmp->args[0].boolean ? true : false;
		return (true);
	 }
   return (false);
}

/* extract integer value from configuration file */
bool cfg_get_int (long int *value,const char *name)
{
   stmt_t *tmp;
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == TOK_INTEGER && tmp->n == 1)
	 {
		*value = tmp->args[0].integer;
		return (true);
	 }
   return (false);
}

#define string(fn,tok)																					\
bool cfg_get_##fn (char **str,const char *name)															\
{																										\
   stmt_t *tmp;																							\
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == tok && tmp->n == 1)		\
	 {																									\
		if ((*str = (char *) mem_alloc ((strlen (tmp->args[0].string) + 1) * sizeof (char))) == NULL)	\
		  {																								\
			 log_printf (LOG_WARNING,"Out of memory\n");												\
			 return (false);																			\
		  }																								\
		strcpy (*str,tmp->args[0].string);																\
		return (true);																					\
	 }																									\
   return (false);																						\
}

/* extract string from configuration file */
string(str,TOK_STRING)

/* extract enumeration from configuration file */
string(enum,TOK_ENUM)

/* extract an array of boolean values from configuration file */
bool cfg_get_bool_array (bool **array,int *n,const char *name)
{
   stmt_t *tmp;
   int i;
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == TOK_BOOLEAN)
	 {
		*n = tmp->n;
		if ((*array = (bool *) mem_alloc (tmp->n * sizeof (bool))) == NULL)
		  {
			 log_printf (LOG_WARNING,"Out of memory\n");
			 return (false);
		  }
		for (i = 0; i < tmp->n; i++)
		  (*array)[i] = tmp->args[i].boolean ? true : false;
		return (true);
	 }
   return (false);
}

/* extract an array of integers from configuration file */
bool cfg_get_int_array (long int **array,int *n,const char *name)
{
   stmt_t *tmp;
   int i;
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == TOK_INTEGER)
	 {
		*n = tmp->n;
		if ((*array = (long int *) mem_alloc (tmp->n * sizeof (long int))) == NULL)
		  {
			 log_printf (LOG_WARNING,"Out of memory\n");
			 return (false);
		  }
		for (i = 0; i < tmp->n; i++)
		  (*array)[i] = tmp->args[i].integer;
		return (true);
	 }
   return (false);
}

#define string_array(fn,tok)																						\
bool cfg_get_##fn##_array (char ***array,int *n,const char *name)													\
{																													\
   stmt_t *tmp;																										\
   int i,j;																											\
																													\
   if (sections != NULL && (tmp = find (sections,name)) != NULL && tmp->type == tok)								\
	 {																												\
		*n = tmp->n;																								\
		if ((*array = (char **) mem_alloc (*n * sizeof (char *))) == NULL)											\
		  goto out_of_memory0;																						\
		for (i = 0; i < tmp->n; i++)																				\
		  {																											\
			 if (((*array)[i] = (char *) mem_alloc ((strlen (tmp->args[i].string) + 1) * sizeof (char))) == NULL)	\
			   goto out_of_memory1;																					\
			 strcpy ((*array)[i],tmp->args[i].string);																\
		  }																											\
		return (true);																								\
	 }																												\
   return (false);																									\
																													\
out_of_memory1:																										\
   for (j = 0; j < i; j++) mem_free ((*array)[j]);																	\
   mem_free (*array);																								\
																													\
out_of_memory0:																										\
   log_printf (LOG_WARNING,"Out of memory\n");																		\
   return (false);																									\
}

/* extract an array of strings from configuration file */
string_array(str,TOK_STRING)

/* extract an array of enumerations from configuration file */
string_array(enum,TOK_ENUM)

